// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_upcie.h>

/**
 * Command Queue for asynchronous command submission and completion
 */
int
xnvme_be_upcie_queue_init(struct xnvme_queue *queue, int XNVME_UNUSED(opts))
{
	struct xnvme_queue_upcie *upcie_queue = (void *)(queue);
	struct xnvme_be_upcie_state *state = (void *)queue->base.dev->be.state;
	int err;

	// The spec says that for systems where memory ordering is not guaranteed, then one should
	// leave room in the queue to avoid races. Thus, we do so here, by allocating one more than
	// what is needed.
	err = nvme_controller_create_io_qpair(state->ctrlr->ctrl, &upcie_queue->qpair,
					      queue->base.capacity + 1);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_create_io_qpair()");
		return err;
	}

	return 0;
}

int
xnvme_be_upcie_queue_term(struct xnvme_queue *queue)
{
	struct xnvme_queue_upcie *upcie_queue = (void *)queue;

	nvme_qpair_term(&upcie_queue->qpair);

	return 0;
}

int
xnvme_be_upcie_queue_poke(struct xnvme_queue *queue, uint32_t max)
{
	struct xnvme_queue_upcie *upcie_queue = (struct xnvme_queue_upcie *)queue;
	struct nvme_qpair *qp = &upcie_queue->qpair;
	struct nvme_completion *cq = upcie_queue->qpair.cq;
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

		// Process the completion...
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
	}

	return reaped;
}

int
xnvme_be_upcie_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			    size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_queue_upcie *upcie_queue = (struct xnvme_queue_upcie *)ctx->async.queue;
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
		nvme_request_prep_command_prps_contig(req, &g_upcie_rte.heap, dbuf, dbuf_nbytes,
						      cmd);
	}

	if (mbuf) {
		cmd->mptr = hostmem_dma_v2p(&g_upcie_rte.heap, mbuf);
	}

	err = nvme_qpair_enqueue(&upcie_queue->qpair, cmd);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_enqueue();");
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
		nvme_request_prep_command_prps_iov(req, &g_upcie_rte.heap, dvec, dvec_cnt, cmd);
	}

	if (mbuf) {
		cmd->mptr = hostmem_dma_v2p(&g_upcie_rte.heap, mbuf);
	}

	err = nvme_qpair_enqueue(&upcie_queue->qpair, cmd);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_enqueue();");
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
