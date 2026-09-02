// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <errno.h>
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_upcie_cuda.h>

/**
 * Give back what a queue was built from
 *
 * @param qpair The device-side queue-pair allocation
 * @param sq Submission queue, or NULL
 * @param cq Completion queue, or NULL
 */
static void
_cuda_qpair_unwind(struct xnvme_cuda_queue *qpair, void *sq, void *cq)
{
	cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, sq);
	cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, cq);
	cuMemFree((CUdeviceptr)qpair);
}

/**
 * Give a queue back to whoever handed out its identifier
 *
 * @param dev The device the queue was created on
 * @param ctrlr The controller behind it
 * @param qid The identifier to release
 */
static void
_cuda_qpair_release(struct xnvme_dev *dev, struct xnvme_be_upcie_ctrlr *ctrlr, uint32_t qid)
{
	if (g_upcie_rte.connection.alive) {
		xnvme_be_upcie_cplane_free_qpair_at(ctrlr, qid);
	} else {
		xnvme_be_upcie_ctrlr_qpair_delete_at(dev, qid);
	}
}

int
xnvme_cuda_queue_create(struct xnvme_dev *dev, uint16_t depth, struct xnvme_cuda_queue **queue)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	struct xnvme_cuda_queue *qpair;
	int err;

	err = cuMemAlloc((CUdeviceptr *)&qpair, sizeof(struct xnvme_cuda_queue));
	if (err) {
		XNVME_DEBUG("FAILED: cuMemAlloc(qpair); CUresult(%d)", err);
		return -ENOMEM;
	}

	/* The queue lives in this process's device memory either way, because a
	 * kernel that writes submission entries has to reach it. What differs is
	 * who holds the identifier space and the admin queue the create commands
	 * go on: a served controller's belong to the server, so it is asked; one
	 * this process opened is its own to use.
	 *
	 * The spec says that where memory ordering is not guaranteed one should
	 * leave room in the queue to avoid races, hence one more entry than
	 * asked for. */
	{
		struct nvme_qpair_cuda _qpair = {0};
		size_t sq_nbytes = (size_t)(depth + 1) * sizeof(struct nvme_command);
		size_t cq_nbytes = (size_t)(depth + 1) * sizeof(struct nvme_completion);
		struct xnvme_be_upcie_cuda_ctrlr *slot;
		uint64_t heap_base = g_upcie_cuda_rte.cuda_heap.vaddr;
		int clock_rate_khz = 0;
		uint32_t qid = 0;
		CUdevice cu_dev;

		slot = _cuda_ctrlr_slot_of(state->ctrlr);
		if (!slot || !slot->db_base) {
			XNVME_DEBUG("FAILED: no doorbell mapping the GPU can reach");
			cuMemFree((CUdeviceptr)qpair);
			return -ENOTSUP;
		}

		_qpair.sq = cudamem_dma_alloc_array(&g_upcie_cuda_rte.cuda_heap, 1, sq_nbytes);
		_qpair.cq = cudamem_dma_alloc_array(&g_upcie_cuda_rte.cuda_heap, 1, cq_nbytes);
		if (!_qpair.sq || !_qpair.cq) {
			XNVME_DEBUG("FAILED: cudamem_dma_alloc_array(); errno(%d)", errno);
			_cuda_qpair_unwind(qpair, _qpair.sq, _qpair.cq);
			return -ENOMEM;
		}

		if (g_upcie_rte.connection.alive) {
			if (!slot->reg_offset) {
				XNVME_DEBUG("FAILED: device heap not registered with the server");
				err = -ENOTCONN;
			} else {
				/* Named by offset into the region this process
				 * registered, so the server resolves the address
				 * rather than taking one it was handed. */
				err = xnvme_be_upcie_cplane_alloc_qpair_at(
					state->ctrlr, slot->reg_offset,
					(uint64_t)(uintptr_t)_qpair.sq - heap_base,
					(uint64_t)(uintptr_t)_qpair.cq - heap_base, depth + 1,
					&qid);
			}
		} else {
			err = xnvme_be_upcie_ctrlr_qpair_create_at(
				dev, dmamem_va_to_iova(&g_upcie_cuda_rte.dmem, _qpair.sq),
				dmamem_va_to_iova(&g_upcie_cuda_rte.dmem, _qpair.cq), depth + 1,
				&qid);
		}
		if (err) {
			XNVME_DEBUG("FAILED: creating the queue; err(%d)", err);
			_cuda_qpair_unwind(qpair, _qpair.sq, _qpair.cq);
			return err;
		}

		{
			uint8_t *db = slot->db_base;
			int dstrd = nvme_reg_cap_get_dstrd(nvme_mmio_cap_read(db));

			_qpair.sqdb =
				db + XNVME_BE_UPCIE_DOORBELL_OFFSET + ((2 * qid) << (2 + dstrd));
			_qpair.cqdb = db + XNVME_BE_UPCIE_DOORBELL_OFFSET +
				      ((2 * qid + 1) << (2 + dstrd));
		}
		_qpair.qid = (uint16_t)qid;
		_qpair.depth = depth + 1;
		_qpair.phase = 1;
		_qpair.timeout_ms = state->ctrlr->ctrl->timeout_ms;

		cuCtxGetDevice(&cu_dev);
		cuDeviceGetAttribute(&clock_rate_khz, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, cu_dev);
		_qpair.clocks_per_ms = (uint64_t)clock_rate_khz;

		/* The doorbells need no registration here: the page they sit in
		 * was registered when this runtime came up. */
		err = cuMemcpyHtoD((CUdeviceptr)qpair, &_qpair, sizeof(_qpair));
		if (err) {
			XNVME_DEBUG("FAILED: cuMemcpyHtoD(qpair); CUresult(%d)", err);
			_cuda_qpair_release(dev, state->ctrlr, qid);
			_cuda_qpair_unwind(qpair, _qpair.sq, _qpair.cq);
			return -EIO;
		}
	}

	*queue = qpair;
	return 0;
}

void
xnvme_cuda_queue_destroy(struct xnvme_dev *dev, struct xnvme_cuda_queue *queue)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	struct nvme_qpair_cuda qpair = {0};
	int cu_err;

	/* What the queue was built from lives in device memory, so it is read
	 * back to find the identifier and the blocks to return. A failure here
	 * leaks both rather than freeing the wrong thing. */
	cu_err = cuMemcpyDtoH(&qpair, (CUdeviceptr)queue, sizeof(qpair));
	if (cu_err) {
		XNVME_DEBUG("FAILED: cuMemcpyDtoH(device QP -> host QP); CUresult(%d)", cu_err);
	}

	/* The queue goes back to whoever handed out its identifier, and the
	 * memory it sat in was always this process's to free. */
	if (!cu_err) {
		_cuda_qpair_release(dev, state->ctrlr, qpair.qid);
		cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, qpair.sq);
		cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, qpair.cq);
	}

	cuMemFree((CUdeviceptr)queue);
}

#else

int
xnvme_cuda_queue_create(struct xnvme_dev *XNVME_UNUSED(dev), uint16_t XNVME_UNUSED(depth),
			struct xnvme_cuda_queue **XNVME_UNUSED(queue))
{
	return -ENOSYS;
}

void
xnvme_cuda_queue_destroy(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_cuda_queue *XNVME_UNUSED(queue))
{
}

#endif
