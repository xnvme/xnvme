// SPDX-FileCopyrightText: Simon A. F. Lund
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_DMAMEM_ENABLED
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_dmamem.h>

int
xnvme_be_dmamem_queue_init(struct xnvme_queue *queue, int XNVME_UNUSED(opts))
{
	struct xnvme_queue_dmamem *dq = (void *)queue;
	struct xnvme_be_dmamem_state *state = (void *)queue->base.dev->be.state;
	int err;

	err = nvme_controller_create_io_qpair_dmamem(
		state->ctrlr->ctrl, &dq->qpair, queue->base.capacity + 1, &g_dmamem_rte.heap,
		&dq->sq_offset, &dq->cq_offset, &dq->prp_offset);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_create_io_qpair_dmamem(); err(%d)", err);
		return err;
	}

	return 0;
}

int
xnvme_be_dmamem_queue_term(struct xnvme_queue *queue)
{
	struct xnvme_queue_dmamem *dq = (void *)queue;
	struct xnvme_be_dmamem_state *state = (void *)queue->base.dev->be.state;

	nvme_controller_delete_io_qpair_dmamem(state->ctrlr->ctrl, &dq->qpair, &g_dmamem_rte.heap,
					       dq->sq_offset, dq->cq_offset, dq->prp_offset);

	return 0;
}

int
xnvme_be_dmamem_queue_poke(struct xnvme_queue *queue, uint32_t max)
{
	struct xnvme_queue_dmamem *dq = (struct xnvme_queue_dmamem *)queue;
	struct nvme_qpair *qp = &dq->qpair;
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
	}

	return reaped;
}

int
xnvme_be_dmamem_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			     size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_queue_dmamem *dq = (struct xnvme_queue_dmamem *)ctx->async.queue;
	struct nvme_command *cmd = (struct nvme_command *)&ctx->cmd;
	struct nvme_request *req;
	int err;

	if (dq->base.outstanding == ctx->async.queue->base.capacity) {
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

	req = nvme_request_alloc(dq->qpair.rpool);
	if (!req) {
		XNVME_DEBUG("FAILED: nvme_request_alloc(); errno(%d)", errno);
		return -errno;
	}

	req->user = ctx;
	cmd->cid = req->cid;

	if (dbuf) {
		nvme_request_prep_command_prps_contig_dmamem(req, &g_dmamem_rte.heap, dbuf,
							     dbuf_nbytes, cmd);
	}
	if (mbuf) {
		cmd->mptr = dmamem_va_to_iova(&g_dmamem_rte.dmem, mbuf);
	}

	err = nvme_qpair_enqueue(&dq->qpair, cmd);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_enqueue(); err(%d)", err);
		nvme_request_free(dq->qpair.rpool, req->cid);
		return err;
	}

	dq->base.outstanding += 1;
	return 0;
}

int
xnvme_be_dmamem_async_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			      size_t XNVME_UNUSED(dvec_nbytes), void *mbuf,
			      size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_queue_dmamem *dq = (struct xnvme_queue_dmamem *)ctx->async.queue;
	struct nvme_command *cmd = (struct nvme_command *)&ctx->cmd;
	struct nvme_request *req;
	int err;

	if (dq->base.outstanding == ctx->async.queue->base.capacity) {
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

	req = nvme_request_alloc(dq->qpair.rpool);
	if (!req) {
		XNVME_DEBUG("FAILED: nvme_request_alloc(); errno(%d)", errno);
		return -errno;
	}

	req->user = ctx;
	cmd->cid = req->cid;

	if (dvec) {
		nvme_request_prep_command_prps_iov_dmamem(req, &g_dmamem_rte.heap, dvec, dvec_cnt,
							  cmd);
	}
	if (mbuf) {
		cmd->mptr = dmamem_va_to_iova(&g_dmamem_rte.dmem, mbuf);
	}

	err = nvme_qpair_enqueue(&dq->qpair, cmd);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_enqueue(); err(%d)", err);
		nvme_request_free(dq->qpair.rpool, req->cid);
		return err;
	}

	dq->base.outstanding += 1;
	return 0;
}

#endif

struct xnvme_be_async g_xnvme_be_dmamem_async = {
	.id = "dmamem",
#ifdef XNVME_BE_DMAMEM_ENABLED
	.cmd_io = xnvme_be_dmamem_async_cmd_io,
	.cmd_iov = xnvme_be_dmamem_async_cmd_iov,
	.poke = xnvme_be_dmamem_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_dmamem_queue_init,
	.term = xnvme_be_dmamem_queue_term,
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
