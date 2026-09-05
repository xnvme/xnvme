// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <errno.h>
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_upcie_cplane_client.h>
#include <xnvme_be_upcie_cuda.h>
#include <xnvme_be_upcie_cuda_cqmirror.h>
#include <xnvme_lock.h>
#include <unistd.h>

static int
_cuda_iova_contiguous(struct dmamem *dmem, void *va, size_t nbytes)
{
	uint64_t base = dmamem_va_to_iova(dmem, va);

	for (size_t off = 4096; off < nbytes; off += 4096) {
		if (dmamem_va_to_iova(dmem, (uint8_t *)va + off) != base + off) {
			return 0;
		}
	}

	return 1;
}

static void
_cuda_qpair_unwind(struct xnvme_cuda_queue *qpair, void *sq, void *cq)
{
	cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, sq);
	cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, cq);
	cuMemFree((CUdeviceptr)qpair);
}

static void
_cuda_qpair_release(struct xnvme_dev *dev, struct xnvme_be_upcie_ctrlr *ctrlr, uint32_t qid)
{
	if (g_upcie_rte.connection.alive) {
		xnvme_be_upcie_cplane_free_qpair_at(ctrlr, qid);
	} else {
		xnvme_be_upcie_ctrlr_qpair_delete_id(dev, qid);
	}
}

static int
_xnvme_cuda_queue_create(struct xnvme_dev *dev, uint16_t depth, struct xnvme_cuda_queue **queue)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	struct xnvme_cuda_queue *qpair;
	int err;

	/* Creating and destroying the queue synchronise the device, which a resident mirror
	 * kernel never lets idle */
	if (xnvme_be_upcie_cuda_cqmirror_nactive()) {
		XNVME_DEBUG("FAILED: a XNVME_QUEUE_P2P_CQ_MIRROR queue is open");
		return -EBUSY;
	}

	err = cuMemAlloc((CUdeviceptr *)&qpair, sizeof(struct xnvme_cuda_queue));
	if (err) {
		XNVME_DEBUG("FAILED: cuMemAlloc(qpair); CUresult(%d)", err);
		return -ENOMEM;
	}

	/* One more entry than asked for: the spec wants room where ordering is not guaranteed */
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

		/* Served: contiguity across GPU pages cannot be checked here */
		if (g_upcie_rte.connection.alive &&
		    sq_nbytes > (size_t)g_upcie_cuda_rte.cuda_heap.config->device_pagesize) {
			XNVME_DEBUG("FAILED: served SQ of depth(%u) spans GPU pages", depth);
			cuMemFree((CUdeviceptr)qpair);
			return -ERANGE;
		}

		_qpair.sq = cudamem_dma_alloc_array(&g_upcie_cuda_rte.cuda_heap, depth + 1,
						    sizeof(struct nvme_command));
		_qpair.cq = cudamem_dma_alloc_array(&g_upcie_cuda_rte.cuda_heap, 1, cq_nbytes);
		if (!_qpair.sq || !_qpair.cq) {
			XNVME_DEBUG("FAILED: cudamem_dma_alloc_array(); errno(%d)", errno);
			_cuda_qpair_unwind(qpair, _qpair.sq, _qpair.cq);
			return -ENOMEM;
		}
		/* An entry with its phase bit set reads as a completion */
		err = cuMemsetD8((CUdeviceptr)_qpair.sq, 0, sq_nbytes);
		if (!err) {
			err = cuMemsetD8((CUdeviceptr)_qpair.cq, 0, cq_nbytes);
		}
		if (err) {
			XNVME_DEBUG("FAILED: cuMemsetD8(); CUresult(%d)", err);
			_cuda_qpair_unwind(qpair, _qpair.sq, _qpair.cq);
			return -EIO;
		}
		if (!g_upcie_rte.connection.alive &&
		    !_cuda_iova_contiguous(state->dmem, _qpair.sq, sq_nbytes)) {
			XNVME_DEBUG(
				"FAILED: SQ of depth(%u) spans GPU pages at discontiguous IOVAs",
				depth);
			_cuda_qpair_unwind(qpair, _qpair.sq, _qpair.cq);
			return -ERANGE;
		}

		if (g_upcie_rte.connection.alive) {
			if (!slot->reg_offset) {
				XNVME_DEBUG("FAILED: device heap not registered with the server");
				err = -ENOTCONN;
			} else {
				err = xnvme_be_upcie_cplane_alloc_qpair_at(
					state->ctrlr, slot->reg_offset,
					(uint64_t)(uintptr_t)_qpair.sq - heap_base,
					(uint64_t)(uintptr_t)_qpair.cq - heap_base, depth + 1,
					&qid);
			}
		} else {
			err = xnvme_be_upcie_ctrlr_qpair_create_at(
				dev, dmamem_va_to_iova(state->dmem, _qpair.sq),
				dmamem_va_to_iova(state->dmem, _qpair.cq), depth + 1, &qid);
		}
		if (err) {
			XNVME_DEBUG("FAILED: creating the queue; err(%d)", err);
			_cuda_qpair_unwind(qpair, _qpair.sq, _qpair.cq);
			return err;
		}

		{
			uint8_t *db = slot->db_base;
			int dstrd = nvme_reg_cap_get_dstrd(nvme_mmio_cap_read(db));

			/* The GPU reaches only the doorbell page registered with it */
			if ((qid > UINT16_MAX) || (((size_t)(2 * qid + 1) << (2 + dstrd)) + 4 >
						   (size_t)sysconf(_SC_PAGESIZE))) {
				XNVME_DEBUG("FAILED: the doorbells of queue(%u) lie past the page "
					    "the GPU reaches",
					    qid);
				_cuda_qpair_release(dev, state->ctrlr, qid);
				_cuda_qpair_unwind(qpair, _qpair.sq, _qpair.cq);
				return -EPROTO;
			}
			_qpair.sqdb =
				db + XNVME_BE_UPCIE_DOORBELL_OFFSET + ((2 * qid) << (2 + dstrd));
			_qpair.cqdb = db + XNVME_BE_UPCIE_DOORBELL_OFFSET +
				      ((2 * qid + 1) << (2 + dstrd));
		}
		_qpair.qid = (uint16_t)qid;
		_qpair.depth = depth + 1;
		_qpair.phase = 1;
		_qpair.timeout_ms = state->ctrlr->ctrl->base.timeout_ms;

		cuCtxGetDevice(&cu_dev);
		cuDeviceGetAttribute(&clock_rate_khz, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, cu_dev);
		_qpair.clocks_per_ms = (uint64_t)clock_rate_khz;

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

/* Changed under xnvme_lock() */
static int g_cuda_gpu_nqueues;

int
xnvme_be_upcie_cuda_gpu_nqueues(void)
{
	return g_cuda_gpu_nqueues;
}

int
xnvme_cuda_queue_create(struct xnvme_dev *dev, uint16_t depth, struct xnvme_cuda_queue **queue)
{
	CUcontext prev;
	int err;

	/* Taken first, so a close cannot release the context in between */
	xnvme_lock();
	if (!g_upcie_cuda_rte.is_initialized) {
		XNVME_DEBUG("FAILED: no upcie-cuda device is open");
		xnvme_unlock();
		return -ENOTCONN;
	}

	if (cuCtxPushCurrent(g_upcie_cuda_rte.cu_ctx) != CUDA_SUCCESS) {
		xnvme_unlock();
		return -EIO;
	}
	err = _xnvme_cuda_queue_create(dev, depth, queue);
	if (!err) {
		++g_cuda_gpu_nqueues;
	}
	cuCtxPopCurrent(&prev);
	xnvme_unlock();

	return err;
}

static int
_xnvme_cuda_queue_destroy(struct xnvme_dev *dev, struct xnvme_cuda_queue *queue)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	struct nvme_qpair_cuda qpair = {0};
	int cu_err;

	cu_err = cuMemcpyDtoH(&qpair, (CUdeviceptr)queue, sizeof(qpair));
	if (cu_err) {
		XNVME_DEBUG("FAILED: cuMemcpyDtoH(device QP -> host QP); CUresult(%d)", cu_err);
	}

	if (!cu_err) {
		_cuda_qpair_release(dev, state->ctrlr, qpair.qid);
		cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, qpair.sq);
		cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, qpair.cq);
	}

	return 0;
}

void
xnvme_cuda_queue_destroy(struct xnvme_dev *dev, struct xnvme_cuda_queue *queue)
{
	CUcontext prev;
	int err;

	xnvme_lock();
	if (!g_upcie_cuda_rte.is_initialized ||
	    (cuCtxPushCurrent(g_upcie_cuda_rte.cu_ctx) != CUDA_SUCCESS)) {
		XNVME_DEBUG("FAILED: no upcie-cuda runtime to destroy the queue in");
		xnvme_unlock();
		return;
	}
	err = _xnvme_cuda_queue_destroy(dev, queue);
	if (!err) {
		--g_cuda_gpu_nqueues;
	}

	if (!err) {
		cuMemFree((CUdeviceptr)queue);
	}
	cuCtxPopCurrent(&prev);
	xnvme_unlock();
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
