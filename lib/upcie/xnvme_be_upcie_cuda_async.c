// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Queue setup for `upcie-cuda`: XNVME_QUEUE_P2P_CQ_MIRROR
 *
 * A queue without the flag is `upcie`'s. With it, the controller completes
 * into a CQ in the GPU heap, next to the data, and a resident warp keeps the
 * queue's dmamem CQ a copy of it, so submission and completion handling are
 * `upcie`'s unchanged. On a served controller the server creates the queue and
 * is told where the CQ is, by offset into the heap this process registered.
 */
#include <libxnvme.h>
#include <errno.h>
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_upcie_cuda.h>
#include <xnvme_be_upcie_cuda_cqmirror.h>

static int
_queue_init(struct xnvme_queue *queue, int opts)
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
		return xnvme_be_upcie_queue_init_unlocked(queue, opts);
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

	/* The address is the server's to know where the controller is served:
	 * it resolves the offset through the registration it holds. */
	cq_iova = g_upcie_rte.connection.alive
			  ? 0
			  : dmamem_va_to_iova(state->dmem, upcie_queue->cq_gpu);
	if (!cq_iova && !g_upcie_rte.connection.alive) {
		XNVME_DEBUG("FAILED: the CQ has no address the controller can reach");
		err = -EFAULT;
		goto free_cq;
	}

	if (g_upcie_rte.connection.alive) {
		/* The server creates the queue and takes the SQ and the PRP
		 * scratch from its heap; the CQ is named by offset into the
		 * region this process registered, which is the whole heap. */
		struct xnvme_be_upcie_cuda_ctrlr *slot = _cuda_ctrlr_slot_of(state->ctrlr);

		if (!slot || !slot->reg_offset) {
			XNVME_DEBUG("FAILED: device heap not registered with the server");
			err = -ENOTCONN;
			goto free_cq;
		}
		err = xnvme_be_upcie_cplane_alloc_qpair_cq_at(
			state->ctrlr, &upcie_queue->qpair, depth, slot->reg_offset,
			(uint64_t)(uintptr_t)upcie_queue->cq_gpu -
				g_upcie_cuda_rte.cuda_heap.vaddr);
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
xnvme_be_upcie_cuda_queue_init(struct xnvme_queue *queue, int opts)
{
	int err;

	xnvme_be_upcie_heap_lock();
	err = _queue_init(queue, opts);
	xnvme_be_upcie_heap_unlock();
	return err;
}

static int
_queue_term(struct xnvme_queue *queue)
{
	struct xnvme_queue_upcie *upcie_queue = (void *)queue;
	struct xnvme_be_upcie_state *state = (void *)queue->base.dev->be.state;
	int err;

	if (!upcie_queue->cq_gpu) {
		return xnvme_be_upcie_queue_term_unlocked(queue);
	}

	/* The warp first, so nothing reads the CQ once the controller is told
	 * to let go of it; the memory last, since a failed delete keeps it. A
	 * served queue is handed back instead, and the server keeps or frees
	 * its half by the same rule. */
	xnvme_be_upcie_cuda_cqmirror_detach(upcie_queue->cqmirror_slot);
	if (g_upcie_rte.connection.alive) {
		xnvme_be_upcie_cplane_free_qpair(state->ctrlr, &upcie_queue->qpair);
		err = 0;
	} else {
		err = nvme_controller_delete_io_qpair_dmamem(
			state->ctrlr->ctrl, &upcie_queue->qpair, &g_upcie_rte.mem.heap,
			upcie_queue->offsets.sq, upcie_queue->offsets.cq,
			upcie_queue->offsets.prp);
	}
	if (err) {
		XNVME_DEBUG("FAILED: deleting qid %u; err(%d), keeping its GPU memory too",
			    upcie_queue->qpair.qid, err);
		return 0;
	}
	cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, upcie_queue->cq_gpu);
	upcie_queue->cq_gpu = NULL;

	return 0;
}

int
xnvme_be_upcie_cuda_queue_term(struct xnvme_queue *queue)
{
	int err;

	xnvme_be_upcie_heap_lock();
	err = _queue_term(queue);
	xnvme_be_upcie_heap_unlock();
	return err;
}
#endif
