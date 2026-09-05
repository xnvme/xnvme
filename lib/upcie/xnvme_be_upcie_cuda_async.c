// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Queue setup for `upcie-cuda`: XNVME_QUEUE_CQ_GPU
 *
 * A queue without the flag is `upcie`'s. With it, the controller completes
 * into a CQ in the GPU heap, next to the data, and a resident warp keeps the
 * queue's dmamem CQ a copy of it, so submission and completion handling are
 * `upcie`'s unchanged. Only a controller this process drives itself can have
 * it; a secondary's queue is the primary's to place.
 */
#include <libxnvme.h>
#include <errno.h>
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_upcie_cuda.h>
#include <xnvme_be_upcie_cuda_cqmirror.h>

int
xnvme_be_upcie_cuda_queue_init(struct xnvme_queue *queue, int opts)
{
	struct xnvme_queue_upcie *upcie_queue = (void *)queue;
	struct xnvme_be_upcie_state *state = (void *)queue->base.dev->be.state;
	struct dmamem *host = &g_upcie_rte.mem.dmem;
	uint16_t depth = queue->base.capacity + 1;
	size_t nbytes;
	uint64_t cq_iova;
	CUcontext prev;
	CUresult res;
	int err;

	if (!(opts & XNVME_QUEUE_CQ_GPU)) {
		return xnvme_be_upcie_queue_init(queue, opts);
	}
	if (g_upcie_rte.mproc) {
		XNVME_DEBUG("FAILED: XNVME_QUEUE_CQ_GPU on a secondary process");
		return -ENOTSUP;
	}

	nbytes = ((size_t)depth * sizeof(struct nvme_completion) + 4095) & ~(size_t)4095;
	upcie_queue->cq_gpu = cudamem_heap_block_alloc_array_aligned(&g_upcie_cuda_rte.cuda_heap,
								     1, nbytes, 4096);
	if (!upcie_queue->cq_gpu) {
		XNVME_DEBUG("FAILED: allocating %zu bytes of GPU memory for the CQ; errno(%d)",
			    nbytes, errno);
		return -errno;
	}

	/* The controller may only ever see phase one where a completion was
	 * written, and the warp expects the same of the host copy, which
	 * nvme_qpair_dmamem_init() zeroes. */
	cuCtxPushCurrent(g_upcie_cuda_rte.cu_ctx);
	res = cuMemsetD8((CUdeviceptr)upcie_queue->cq_gpu, 0, nbytes);
	if (res == CUDA_SUCCESS) {
		res = cuStreamSynchronize(NULL);
	}
	cuCtxPopCurrent(&prev);
	if (res != CUDA_SUCCESS) {
		XNVME_DEBUG("FAILED: zeroing the CQ; res(%d)", res);
		err = -EIO;
		goto free_cq;
	}

	cq_iova = dmamem_va_to_iova(state->dmem, upcie_queue->cq_gpu);
	if (!cq_iova) {
		XNVME_DEBUG("FAILED: the CQ has no address the controller can reach");
		err = -EFAULT;
		goto free_cq;
	}

	err = nvme_controller_create_io_qpair_dmamem_cq_iova(
		state->ctrlr->ctrl, &upcie_queue->qpair, depth, &g_upcie_rte.mem.heap,
		&upcie_queue->offsets.sq, &upcie_queue->offsets.cq, &upcie_queue->offsets.prp,
		cq_iova);
	if (err) {
		XNVME_DEBUG("FAILED: creating a queue of %u; err(%d)", depth, err);
		goto free_cq;
	}

	err = xnvme_be_upcie_cuda_cqmirror_attach(g_upcie_cuda_rte.cu_ctx, host->cpu_va,
						  host->size, upcie_queue->cq_gpu,
						  upcie_queue->qpair.cq, depth);
	if (err < 0) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_cuda_cqmirror_attach(); err(%d)", err);
		goto delete_qpair;
	}
	upcie_queue->cqmirror_slot = err;

	return 0;

delete_qpair:
	nvme_controller_delete_io_qpair_dmamem(state->ctrlr->ctrl, &upcie_queue->qpair,
					       &g_upcie_rte.mem.heap, upcie_queue->offsets.sq,
					       upcie_queue->offsets.cq, upcie_queue->offsets.prp);
free_cq:
	cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, upcie_queue->cq_gpu);
	upcie_queue->cq_gpu = NULL;

	return err;
}

int
xnvme_be_upcie_cuda_queue_term(struct xnvme_queue *queue)
{
	struct xnvme_queue_upcie *upcie_queue = (void *)queue;
	struct xnvme_be_upcie_state *state = (void *)queue->base.dev->be.state;
	int err;

	if (!upcie_queue->cq_gpu) {
		return xnvme_be_upcie_queue_term(queue);
	}

	/* The warp first, so nothing reads the CQ once the controller is told
	 * to let go of it; the memory last, since a failed delete keeps it. */
	xnvme_be_upcie_cuda_cqmirror_detach(upcie_queue->cqmirror_slot);
	err = nvme_controller_delete_io_qpair_dmamem(
		state->ctrlr->ctrl, &upcie_queue->qpair, &g_upcie_rte.mem.heap,
		upcie_queue->offsets.sq, upcie_queue->offsets.cq, upcie_queue->offsets.prp);
	if (err) {
		XNVME_DEBUG("FAILED: deleting qid %u; err(%d), keeping its GPU memory too",
			    upcie_queue->qpair.qid, err);
		return 0;
	}
	cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, upcie_queue->cq_gpu);
	upcie_queue->cq_gpu = NULL;

	return 0;
}
#endif
