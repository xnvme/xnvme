// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <errno.h>
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <xnvme_be_nosys.h>
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_upcie_hip.h>
#include <xnvme_be_upcie_hip_cqmirror.h>

int
xnvme_be_upcie_hip_queue_init(struct xnvme_queue *queue, int opts)
{
	struct xnvme_queue_upcie *upcie_queue = (void *)queue;
	struct xnvme_be_upcie_state *state = (void *)queue->base.dev->be.state;
	struct dmamem *host = &g_upcie_rte.mem.dmem;
	uint16_t depth = queue->base.capacity + 1;
	size_t nbytes = (size_t)depth * sizeof(struct nvme_completion);
	uint64_t cq_iova;
	int err;

	if (!(opts & XNVME_QUEUE_P2P_CQ_MIRROR)) {
		return xnvme_be_upcie_queue_init(queue, opts);
	}
	if (g_upcie_rte.connection.alive) {
		XNVME_DEBUG("FAILED: XNVME_QUEUE_P2P_CQ_MIRROR on a served controller");
		return -ENOTSUP;
	}

	upcie_queue->cq_gpu = xnvme_be_upcie_hip_cqmirror_cq_alloc();
	if (!upcie_queue->cq_gpu) {
		XNVME_DEBUG("FAILED: allocating GPU memory for the CQ; errno(%d)", errno);
		return -errno;
	}

	err = xnvme_be_upcie_dmamem_map(state->dmem, upcie_queue->cq_gpu, nbytes, &cq_iova);
	if (err) {
		XNVME_DEBUG("FAILED: the CQ has no address the controller can reach; err(%d)",
			    err);
		goto release_cq;
	}

	err = nvme_controller_create_io_qpair_dmamem_cq_iova(
		state->ctrlr->ctrl, &upcie_queue->qpair, depth, &g_upcie_rte.mem.heap,
		&upcie_queue->offsets.sq, &upcie_queue->offsets.cq, &upcie_queue->offsets.prp,
		cq_iova);
	if (err) {
		XNVME_DEBUG("FAILED: creating a queue of %u; err(%d)", depth, err);
		goto unmap_cq;
	}

	err = xnvme_be_upcie_hip_cqmirror_attach(host->cpu_va, host->size, upcie_queue->cq_gpu,
						 upcie_queue->qpair.cq, depth);
	if (err < 0) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_hip_cqmirror_attach(); err(%d)", err);
		goto delete_qpair;
	}
	upcie_queue->cqmirror_slot = err;

	return 0;

delete_qpair:
	nvme_controller_delete_io_qpair_dmamem(state->ctrlr->ctrl, &upcie_queue->qpair,
					       &g_upcie_rte.mem.heap, upcie_queue->offsets.sq,
					       upcie_queue->offsets.cq, upcie_queue->offsets.prp);
unmap_cq:
	xnvme_be_upcie_dmamem_unmap(state->dmem, upcie_queue->cq_gpu);
release_cq:
	xnvme_be_upcie_hip_cqmirror_cq_release(upcie_queue->cq_gpu);
	upcie_queue->cq_gpu = NULL;

	return err;
}

int
xnvme_be_upcie_hip_queue_term(struct xnvme_queue *queue)
{
	struct xnvme_queue_upcie *upcie_queue = (void *)queue;
	struct xnvme_be_upcie_state *state = (void *)queue->base.dev->be.state;
	int err;

	if (!upcie_queue->cq_gpu) {
		return xnvme_be_upcie_queue_term(queue);
	}

	/* The wavefront first, so nothing reads the CQ after the controller lets go; the memory
	 * last, since a failed delete keeps it */
	xnvme_be_upcie_hip_cqmirror_detach(upcie_queue->cqmirror_slot);
	err = nvme_controller_delete_io_qpair_dmamem(
		state->ctrlr->ctrl, &upcie_queue->qpair, &g_upcie_rte.mem.heap,
		upcie_queue->offsets.sq, upcie_queue->offsets.cq, upcie_queue->offsets.prp);
	if (err) {
		XNVME_DEBUG("FAILED: deleting qid %u; err(%d), keeping its GPU memory too",
			    upcie_queue->qpair.qid, err);
		return 0;
	}
	xnvme_be_upcie_dmamem_unmap(state->dmem, upcie_queue->cq_gpu);
	xnvme_be_upcie_hip_cqmirror_cq_release(upcie_queue->cq_gpu);
	upcie_queue->cq_gpu = NULL;

	return 0;
}

struct xnvme_be_async g_xnvme_be_upcie_hip_async = {
	.id = "upcie-hip",
	.cmd_io = xnvme_be_upcie_async_cmd_io,
	.cmd_iov = xnvme_be_upcie_async_cmd_iov,
	.poke = xnvme_be_upcie_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_upcie_hip_queue_init,
	.term = xnvme_be_upcie_hip_queue_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
};

#endif
