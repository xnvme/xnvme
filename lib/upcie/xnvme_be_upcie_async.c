// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_upcie.h>

/**
 * Pokes reaping nothing before the server's socket is looked at
 *
 * Large enough that the recv() stays off the fast path, small enough that a
 * dead server is noticed within a moment of the queue going quiet.
 */
#define XNVME_BE_UPCIE_POKES_IDLE_MAX 1024

int
xnvme_be_upcie_queue_init(struct xnvme_queue *queue, int XNVME_UNUSED(opts))
{
	struct xnvme_queue_upcie *upcie_queue = (void *)queue;
	struct xnvme_be_upcie_state *state = (void *)queue->base.dev->be.state;
	int err;

	if (g_upcie_rte.connection.alive) {
		/* The controller is another process's, so the queue is asked
		 * for rather than created, and comes back as offsets this side
		 * resolves for itself. */
		err = xnvme_be_upcie_cplane_alloc_qpair(state->ctrlr, &upcie_queue->qpair,
							queue->base.capacity + 1);
	} else {
		err = nvme_controller_create_io_qpair_dmamem(
			state->ctrlr->ctrl, &upcie_queue->qpair, queue->base.capacity + 1,
			&g_upcie_rte.mem.heap, &upcie_queue->offsets.sq, &upcie_queue->offsets.cq,
			&upcie_queue->offsets.prp);
	}
	if (err) {
		XNVME_DEBUG("FAILED: creating a queue of %u; err(%d)", queue->base.capacity + 1,
			    err);
		return err;
	}

	return 0;
}

int
xnvme_be_upcie_queue_term(struct xnvme_queue *queue)
{
	struct xnvme_queue_upcie *upcie_queue = (void *)queue;
	struct xnvme_be_upcie_state *state = (void *)queue->base.dev->be.state;

	if (g_upcie_rte.connection.alive) {
		xnvme_be_upcie_cplane_free_qpair(state->ctrlr, &upcie_queue->qpair);
	} else {
		nvme_controller_delete_io_qpair_dmamem(
			state->ctrlr->ctrl, &upcie_queue->qpair, &g_upcie_rte.mem.heap,
			upcie_queue->offsets.sq, upcie_queue->offsets.cq,
			upcie_queue->offsets.prp);
	}

	return 0;
}

/**
 * Whether whoever served this controller is still behind its socket
 *
 * The server never sends unsolicited, so anything but "nothing yet" says it
 * left: an orderly close reads as end-of-stream, an unclean one as an error.
 */
static int
server_gone(int sock)
{
	char byte;
	ssize_t nbytes;

	if (sock < 0) {
		return 1;
	}

	nbytes = recv(sock, &byte, 1, MSG_PEEK | MSG_DONTWAIT);
	if (nbytes < 0) {
		return (errno != EAGAIN) && (errno != EWOULDBLOCK) && (errno != EINTR);
	}

	return nbytes == 0;
}

/**
 * Complete everything in flight on a queue its server deleted
 *
 * The status is the one the controller would have posted had anybody been
 * left to observe it: aborted because the submission queue was deleted.
 *
 * @return How many commands were completed
 */
static int
queue_fail_inflight(struct xnvme_queue_upcie *upcie_queue)
{
	struct nvme_request_pool *rpool = upcie_queue->qpair.rpool;
	uint8_t inflight[NVME_REQUEST_POOL_LEN];
	int failed = 0;

	memset(inflight, 1, sizeof(inflight));
	for (size_t i = 0; i < rpool->top; ++i) {
		inflight[rpool->stack[i]] = 0;
	}

	for (uint16_t cid = 0; cid < NVME_REQUEST_POOL_LEN; ++cid) {
		struct xnvme_cmd_ctx *ctx;

		if (!inflight[cid]) {
			continue;
		}

		ctx = rpool->reqs[cid].user;
		nvme_request_free(rpool, cid);
		upcie_queue->base.outstanding -= 1;

		memset(&ctx->cpl, 0, sizeof(ctx->cpl));
		ctx->cpl.cid = cid;
		ctx->cpl.status.sc = 0x08; ///< Command Aborted due to SQ Deletion
		ctx->cpl.status.dnr = 1;

		ctx->async.cb(ctx, ctx->async.cb_arg);
		failed++;
	}

	return failed;
}

/**
 * What a poke reaping nothing on a served queue does before giving up
 *
 * A server shutting down deletes the queues its clients hold, and what was in
 * flight on them is dropped without a completion, so a caller draining the
 * queue would otherwise spin forever. The socket is what says the server left,
 * and the command timeout after that is what lets a controller the server
 * merely abandoned, killed rather than shut down, land what it still holds.
 */
static int
queue_poke_idle(struct xnvme_queue_upcie *upcie_queue)
{
	struct xnvme_be_upcie_state *state = (void *)upcie_queue->base.dev->be.state;
	struct timespec ts;
	uint64_t now_ns;

	if (++upcie_queue->pokes_idle < XNVME_BE_UPCIE_POKES_IDLE_MAX) {
		return 0;
	}
	upcie_queue->pokes_idle = 0;

	if (!upcie_queue->served_gone_ns) {
		if (!server_gone(g_upcie_rte.connection.sock)) {
			return 0;
		}

		clock_gettime(CLOCK_MONOTONIC, &ts);
		upcie_queue->served_gone_ns =
			(uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
		return 0;
	}

	clock_gettime(CLOCK_MONOTONIC, &ts);
	now_ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
	if ((now_ns - upcie_queue->served_gone_ns) <
	    ((uint64_t)state->ctrlr->ctrl->timeout_ms * 1000000ULL)) {
		return 0;
	}

	return queue_fail_inflight(upcie_queue);
}

int
xnvme_be_upcie_queue_poke(struct xnvme_queue *queue, uint32_t max)
{
	struct xnvme_queue_upcie *upcie_queue = (struct xnvme_queue_upcie *)queue;
	struct nvme_qpair *qp = &upcie_queue->qpair;
	struct nvme_completion *cq = qp->cq;
	unsigned int reaped = 0;

	if (!max) {
		max = queue->base.outstanding;
	}

	wmb();
	nvme_qpair_sqdb_update(qp);

	do {
		struct nvme_completion *cqe;

		cqe = &cq[qp->head];
		if (((*(const volatile uint16_t *)&(cqe->status)) & 0x1) != qp->phase) {
			break;
		}

		dma_rmb();

		if (++qp->head == qp->depth) {
			qp->head = 0;
			qp->phase ^= 1;
		}

		reaped++;

		{
			struct xnvme_cmd_ctx *ctx;
			struct nvme_request *req;

			req = nvme_request_get(qp->rpool, cqe->cid);
			if (!req) {
				XNVME_DEBUG("FAILED: nvme_request_get()");
				return -EIO;
			}

			ctx = req->user;
			memcpy(&ctx->cpl, cqe, sizeof(ctx->cpl));
			nvme_request_free(qp->rpool, req->cid);

			queue->base.outstanding -= 1;

			ctx->async.cb(ctx, ctx->async.cb_arg);
		}
	} while (reaped < max);

	if (reaped) {
		mmio_write32(qp->cqdb, 0, qp->head);
		upcie_queue->pokes_idle = 0;
		upcie_queue->served_gone_ns = 0;
		return reaped;
	}

	if (!g_upcie_rte.connection.alive) {
		return 0;
	}

	return queue_poke_idle(upcie_queue);
}

int
xnvme_be_upcie_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			    size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_queue_upcie *upcie_queue = (struct xnvme_queue_upcie *)ctx->async.queue;
	struct xnvme_be_upcie_state *state = (void *)ctx->dev->be.state;
	struct nvme_command *cmd = (struct nvme_command *)&ctx->cmd;
	struct nvme_request *req;
	int err;

	if (upcie_queue->base.outstanding == ctx->async.queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_FS_OPC_READ:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
		break;
	}

	req = nvme_request_alloc(upcie_queue->qpair.rpool);
	if (!req) {
		XNVME_DEBUG("FAILED: nvme_request_alloc(); errno(%d)", errno);
		return -errno;
	}

	req->user = ctx;
	cmd->cid = req->cid;

	if (dbuf) {
		err = nvme_request_prep_command_prps_contig_dmamem(req, state->dmem, dbuf,
								   dbuf_nbytes, cmd);
		if (err) {
			XNVME_DEBUG("FAILED: prps_contig_dmamem(); err(%d)", err);
			nvme_request_free(upcie_queue->qpair.rpool, req->cid);
			return err;
		}
	}
	if (mbuf) {
		cmd->mptr = dmamem_va_to_iova(state->dmem, mbuf);
		if ((DMAMEM_XLATE_LUT == state->dmem->translator) && !cmd->mptr) {
			XNVME_DEBUG("FAILED: mbuf(%p) is not in a registered region", mbuf);
			nvme_request_free(upcie_queue->qpair.rpool, req->cid);
			return -EINVAL;
		}
	}

	err = nvme_qpair_enqueue(&upcie_queue->qpair, cmd);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_enqueue(); err(%d)", err);
		nvme_request_free(upcie_queue->qpair.rpool, req->cid);
		return err;
	}

	upcie_queue->base.outstanding += 1;
	return 0;
}

int
xnvme_be_upcie_async_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			     size_t XNVME_UNUSED(dvec_nbytes), void *mbuf,
			     size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_queue_upcie *upcie_queue = (struct xnvme_queue_upcie *)ctx->async.queue;
	struct xnvme_be_upcie_state *state = (void *)ctx->dev->be.state;
	struct nvme_command *cmd = (struct nvme_command *)&ctx->cmd;
	struct nvme_request *req;
	int err;

	if (upcie_queue->base.outstanding == ctx->async.queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_FS_OPC_READ:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
		break;
	}

	req = nvme_request_alloc(upcie_queue->qpair.rpool);
	if (!req) {
		XNVME_DEBUG("FAILED: nvme_request_alloc(); errno(%d)", errno);
		return -errno;
	}

	req->user = ctx;
	cmd->cid = req->cid;

	if (dvec) {
		err = nvme_request_prep_command_prps_iov_dmamem(req, state->dmem, dvec, dvec_cnt,
								cmd);
		if (err) {
			XNVME_DEBUG("FAILED: prps_iov_dmamem(); err(%d)", err);
			nvme_request_free(upcie_queue->qpair.rpool, req->cid);
			return err;
		}
	}
	if (mbuf) {
		cmd->mptr = dmamem_va_to_iova(state->dmem, mbuf);
		if ((DMAMEM_XLATE_LUT == state->dmem->translator) && !cmd->mptr) {
			XNVME_DEBUG("FAILED: mbuf(%p) is not in a registered region", mbuf);
			nvme_request_free(upcie_queue->qpair.rpool, req->cid);
			return -EINVAL;
		}
	}

	err = nvme_qpair_enqueue(&upcie_queue->qpair, cmd);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_enqueue(); err(%d)", err);
		nvme_request_free(upcie_queue->qpair.rpool, req->cid);
		return err;
	}

	upcie_queue->base.outstanding += 1;
	return 0;
}

#endif

struct xnvme_be_async g_xnvme_be_upcie_async = {
	.id = "upcie",
#ifdef XNVME_BE_UPCIE_ENABLED
	.cmd_io = xnvme_be_upcie_async_cmd_io,
	.cmd_iov = xnvme_be_upcie_async_cmd_iov,
	.poke = xnvme_be_upcie_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_upcie_queue_init,
	.term = xnvme_be_upcie_queue_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
#endif
};
