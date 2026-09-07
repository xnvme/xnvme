// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <errno.h>
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_be_nosys.h>
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_upcie_cplane_client.h>
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

	if (!(opts & XNVME_QUEUE_P2P_CQ_MIRROR)) {
		return xnvme_be_upcie_queue_init(queue, opts);
	}
	/* A GPU-issued queue synchronises the device, which a resident kernel never lets idle */
	if (xnvme_be_upcie_cuda_gpu_nqueues()) {
		XNVME_DEBUG("FAILED: a GPU-issued queue is open");
		return -EBUSY;
	}
	nbytes = ((size_t)depth * sizeof(struct nvme_completion) + 4095) & ~(size_t)4095;
	upcie_queue->cq_gpu = cudamem_heap_block_alloc_array_aligned(&g_upcie_cuda_rte.cuda_heap,
								     1, nbytes, 4096);
	if (!upcie_queue->cq_gpu) {
		XNVME_DEBUG("FAILED: allocating %zu bytes of GPU memory for the CQ; errno(%d)",
			    nbytes, errno);
		return -errno;
	}

	/* The warp starts at entry zero expecting phase one, so the CQ is zeroed */
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

	/* Served: the server resolves the offset itself */
	cq_iova = g_upcie_rte.connection.alive
			  ? 0
			  : dmamem_va_to_iova(state->dmem, upcie_queue->cq_gpu);
	if (!cq_iova && !g_upcie_rte.connection.alive) {
		XNVME_DEBUG("FAILED: the CQ has no address the controller can reach");
		err = -EFAULT;
		goto free_cq;
	}

	if (g_upcie_rte.connection.alive) {
		struct xnvme_be_upcie_cuda_ctrlr *slot = _cuda_ctrlr_slot_of(state->ctrlr);

		if (!slot || !slot->reg_offset) {
			XNVME_DEBUG("FAILED: device heap not registered with the server");
			err = -ENOTCONN;
			goto free_cq;
		}
		err = xnvme_be_upcie_cplane_alloc_qpair_at(
			state->ctrlr, slot->reg_offset, NVME_CPLANE_QUEUE_IN_HEAP,
			(uint64_t)(uintptr_t)upcie_queue->cq_gpu -
				g_upcie_cuda_rte.cuda_heap.vaddr,
			depth, &upcie_queue->qpair, NULL);
	} else {
		err = nvme_controller_create_io_qpair_dmamem_cq_iova(
			state->ctrlr->ctrl, &upcie_queue->qpair, depth, &g_upcie_rte.mem.heap,
			&upcie_queue->offsets.sq, &upcie_queue->offsets.cq,
			&upcie_queue->offsets.prp, cq_iova);
	}
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
	if (g_upcie_rte.connection.alive) {
		xnvme_be_upcie_cplane_free_qpair(state->ctrlr, &upcie_queue->qpair);
	} else {
		nvme_controller_delete_io_qpair_dmamem(
			state->ctrlr->ctrl, &upcie_queue->qpair, &g_upcie_rte.mem.heap,
			upcie_queue->offsets.sq, upcie_queue->offsets.cq,
			upcie_queue->offsets.prp);
	}
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
#ifdef XNVME_DEBUG_ENABLED
	uint32_t qid = upcie_queue->qpair.qid;
#endif
	int err;

	if (!upcie_queue->cq_gpu) {
		return xnvme_be_upcie_queue_term(queue);
	}

	/* The warp first, so nothing reads the CQ after the controller lets go; the memory
	 * last, since a failed delete keeps it */
	xnvme_be_upcie_cuda_cqmirror_detach(upcie_queue->cqmirror_slot);
	if (g_upcie_rte.connection.alive) {
		err = xnvme_be_upcie_cplane_free_qpair(state->ctrlr, &upcie_queue->qpair);
	} else {
		err = nvme_controller_delete_io_qpair_dmamem(
			state->ctrlr->ctrl, &upcie_queue->qpair, &g_upcie_rte.mem.heap,
			upcie_queue->offsets.sq, upcie_queue->offsets.cq,
			upcie_queue->offsets.prp);
	}
	if (err) {
		XNVME_DEBUG("FAILED: deleting qid %u; err(%d), keeping its GPU memory too", qid,
			    err);
		return 0;
	}
	cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, upcie_queue->cq_gpu);
	upcie_queue->cq_gpu = NULL;

	return 0;
}

struct xnvme_be_async g_xnvme_be_upcie_cuda_async = {
	.id = "upcie-cuda",
	.cmd_io = xnvme_be_upcie_async_cmd_io,
	.cmd_iov = xnvme_be_upcie_async_cmd_iov,
	.poke = xnvme_be_upcie_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_upcie_cuda_queue_init,
	.term = xnvme_be_upcie_cuda_queue_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
};

#endif
