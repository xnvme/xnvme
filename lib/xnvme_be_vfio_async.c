// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Michael Bang <mi.bang@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
#include <libxnvme_spec_fs.h>
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_vfio.h>

struct xnvme_queue_vfio {
	struct xnvme_queue_base base;

	struct nvme_sq *sq;
	struct nvme_cq *cq;
	uint id;

	uint8_t _rsvd[208];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_vfio) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

int
xnvme_be_vfio_queue_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	struct xnvme_queue_vfio *queue = (struct xnvme_queue_vfio *)q;
	struct xnvme_be_vfio_state *state = (void *)queue->base.dev->be.state;
	int qpid;

	qpid = _xnvme_be_vfio_create_ioqpair(state, queue->base.capacity, 0x0);
	if (qpid < 0) {
		XNVME_DEBUG("FAILED: nvme_create_ioqpair(); err: %d", qpid);
		return qpid;
	}
	queue->sq = &state->ctrl->sq[qpid];
	queue->cq = &state->ctrl->cq[qpid];
	queue->id = qpid;

	return 0;
}

int
xnvme_be_vfio_queue_term(struct xnvme_queue *q)
{
	struct xnvme_queue_vfio *queue = (struct xnvme_queue_vfio *)q;
	struct xnvme_be_vfio_state *state = (void *)queue->base.dev->be.state;
	return _xnvme_be_vfio_delete_ioqpair(state, queue->id);
}

int
xnvme_be_vfio_queue_poke(struct xnvme_queue *queue, uint32_t max)
{
	struct xnvme_queue_vfio *q = (void *)queue;
	unsigned int reaped = 0;

	if (!max) {
		max = queue->base.outstanding;
	}

	nvme_sq_run(q->sq);

	do {
		struct xnvme_cmd_ctx *ctx;

		struct nvme_rq *rq;
		struct nvme_cqe *cqe;

		cqe = nvme_cq_get_cqe(q->cq);
		if (!cqe) {
			break;
		}

		reaped++;

		rq = __nvme_rq_from_cqe(q->sq, cqe);

		ctx = (struct xnvme_cmd_ctx *)rq->opaque;
		memcpy(&ctx->cpl, cqe, sizeof(ctx->cpl));
		ctx->async.cb(ctx, ctx->async.cb_arg);

		nvme_rq_release(rq);
	} while (reaped < max);

	queue->base.outstanding -= reaped;

	if (reaped) {
		nvme_cq_update_head(q->cq);
	}

	return reaped;
}

int
xnvme_be_vfio_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_queue_vfio *q = (struct xnvme_queue_vfio *)ctx->async.queue;
	struct xnvme_be_vfio_state *state = (void *)q->base.dev->be.state;
	struct nvme_rq *rq;
	uint64_t iova;

	if (q->base.outstanding == q->base.capacity) {
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

	rq = nvme_rq_acquire(q->sq);
	rq->opaque = ctx;

	if (dbuf) {
		if (!vfio_iommu_vaddr_to_iova(&state->ctrl->pci.vfio.iommu, dbuf, &iova)) {
			XNVME_DEBUG("FAILED: vfio_iommu_vaddr_to_iova()");
			goto err;
		}

		nvme_rq_map_prp(rq, (union nvme_cmd *)&ctx->cmd, iova, dbuf_nbytes);
	}

	if (mbuf) {
		if (!vfio_iommu_vaddr_to_iova(&state->ctrl->pci.vfio.iommu, mbuf, &iova)) {
			XNVME_DEBUG("FAILED: vfio_iommu_vaddr_to_iova()");
			goto err;
		}

		ctx->cmd.common.mptr = iova;
	}

	nvme_rq_exec(rq, (union nvme_cmd *)&ctx->cmd);

	q->base.outstanding += 1;

	return 0;

err:
	nvme_rq_release(rq);
	return -EINVAL;
}

#endif

struct xnvme_be_async g_xnvme_be_vfio_async = {
	.id = "nvme",
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
	.cmd_io = xnvme_be_vfio_async_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_vfio_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_vfio_queue_init,
	.term = xnvme_be_vfio_queue_term,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
#endif
};
