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

struct _cuda_host_sq {
	struct xnvme_cuda_queue *queue; ///< NULL when the slot is free
	void *va;
	size_t offset; ///< Unused when served
	size_t nbytes;
	void *rpool;
	int served; ///< The block is the server's
};

static struct _cuda_host_sq g_host_sqs[XNVME_BE_CREF_MAX_ENTRIES * 64];

static struct _cuda_host_sq *
_cuda_host_sq_find(struct xnvme_cuda_queue *queue)
{
	for (size_t i = 0; i < sizeof(g_host_sqs) / sizeof(*g_host_sqs); ++i) {
		if (g_host_sqs[i].queue == queue) {
			return &g_host_sqs[i];
		}
	}

	return NULL;
}

static int
_cuda_host_sq_alloc(size_t nbytes, struct _cuda_host_sq *rec, void **sq_dev, uint64_t *sq_iova)
{
	const size_t page = 4096;
	CUresult res;
	int err;

	rec->nbytes = (nbytes + page - 1) & ~(page - 1);

	err = dmamem_heap_alloc_aligned(&g_upcie_rte.mem.heap, rec->nbytes, page, &rec->offset);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_heap_alloc_aligned(host sq); err(%d)", err);
		return err;
	}
	rec->va = dmamem_heap_at_va(&g_upcie_rte.mem.heap, rec->offset);
	memset(rec->va, 0, rec->nbytes);

	res = cuMemHostRegister(rec->va, rec->nbytes, CU_MEMHOSTREGISTER_DEVICEMAP);
	if (res != CUDA_SUCCESS) {
		XNVME_DEBUG("FAILED: cuMemHostRegister(host sq); res(%d)", res);
		err = -ENOTSUP;
		goto free_block;
	}
	res = cuMemHostGetDevicePointer((CUdeviceptr *)sq_dev, rec->va, 0);
	if (res != CUDA_SUCCESS) {
		XNVME_DEBUG("FAILED: cuMemHostGetDevicePointer(host sq); res(%d)", res);
		cuMemHostUnregister(rec->va);
		err = -ENOTSUP;
		goto free_block;
	}

	*sq_iova = dmamem_heap_at_iova(&g_upcie_rte.mem.heap, rec->offset);

	return 0;

free_block:
	dmamem_heap_free(&g_upcie_rte.mem.heap, rec->offset);
	rec->va = NULL;

	return err;
}

static int
_cuda_host_sq_adopt(void *sq_va, size_t nbytes, struct _cuda_host_sq *rec, void **sq_dev)
{
	const size_t page = 4096;
	CUresult res;

	rec->va = sq_va;
	rec->nbytes = (nbytes + page - 1) & ~(page - 1);
	rec->served = 1;

	res = cuMemHostRegister(rec->va, rec->nbytes, CU_MEMHOSTREGISTER_DEVICEMAP);
	if (res != CUDA_SUCCESS) {
		XNVME_DEBUG("FAILED: cuMemHostRegister(served sq); res(%d)", res);
		rec->va = NULL;
		return -ENOTSUP;
	}
	res = cuMemHostGetDevicePointer((CUdeviceptr *)sq_dev, rec->va, 0);
	if (res != CUDA_SUCCESS) {
		XNVME_DEBUG("FAILED: cuMemHostGetDevicePointer(served sq); res(%d)", res);
		cuMemHostUnregister(rec->va);
		rec->va = NULL;
		return -ENOTSUP;
	}

	return 0;
}

static void
_cuda_host_sq_free(struct _cuda_host_sq *rec)
{
	if (!rec || !rec->va) {
		return;
	}
	cuMemHostUnregister(rec->va);
	if (!rec->served) {
		dmamem_heap_free(&g_upcie_rte.mem.heap, rec->offset);
	}
	free(rec->rpool);
	memset(rec, 0, sizeof(*rec));
}

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
_cuda_qpair_unwind(struct xnvme_cuda_queue *qpair, struct _cuda_host_sq *host_sq, void *dev_sq,
		   void *cq)
{
	_cuda_host_sq_free(host_sq);
	if (dev_sq) {
		cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, dev_sq);
	}
	if (cq) {
		cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, cq);
	}
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
_xnvme_cuda_queue_create(struct xnvme_dev *dev, uint16_t depth, int opts,
			 struct xnvme_cuda_queue **queue)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	struct xnvme_cuda_queue *qpair;
	int hostmem = opts & XNVME_QUEUE_SQ_HOSTMEM;
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
		struct _cuda_host_sq *host_sq = NULL;
		struct nvme_qpair served = {0};
		uint64_t sq_iova = 0;
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
		if (!hostmem && g_upcie_rte.connection.alive &&
		    sq_nbytes > (size_t)g_upcie_cuda_rte.cuda_heap.config->device_pagesize) {
			XNVME_DEBUG("FAILED: served SQ of depth(%u) spans GPU pages; use "
				    "XNVME_QUEUE_SQ_HOSTMEM",
				    depth);
			cuMemFree((CUdeviceptr)qpair);
			return -ERANGE;
		}

		_qpair.cq = cudamem_dma_alloc_array(&g_upcie_cuda_rte.cuda_heap, 1, cq_nbytes);
		if (!_qpair.cq) {
			XNVME_DEBUG("FAILED: cudamem_dma_alloc_array(cq); errno(%d)", errno);
			cuMemFree((CUdeviceptr)qpair);
			return -ENOMEM;
		}
		/* An entry with its phase bit set reads as a completion */
		err = cuMemsetD8((CUdeviceptr)_qpair.cq, 0, cq_nbytes);
		if (err) {
			XNVME_DEBUG("FAILED: cuMemsetD8(cq); CUresult(%d)", err);
			_cuda_qpair_unwind(qpair, NULL, NULL, _qpair.cq);
			return -EIO;
		}
		if (hostmem) {
			host_sq = _cuda_host_sq_find(NULL);
			if (!host_sq) {
				XNVME_DEBUG("FAILED: no room to record another GPU-issued queue");
				_cuda_qpair_unwind(qpair, NULL, NULL, _qpair.cq);
				return -ENOSPC;
			}
		} else {
			_qpair.sq = cudamem_dma_alloc_array(&g_upcie_cuda_rte.cuda_heap, depth + 1,
							    sizeof(struct nvme_command));
			if (!_qpair.sq) {
				XNVME_DEBUG("FAILED: cudamem_dma_alloc_array(sq); errno(%d)",
					    errno);
				_cuda_qpair_unwind(qpair, NULL, NULL, _qpair.cq);
				return -ENOMEM;
			}
			err = cuMemsetD8((CUdeviceptr)_qpair.sq, 0, sq_nbytes);
			if (err) {
				XNVME_DEBUG("FAILED: cuMemsetD8(sq); CUresult(%d)", err);
				_cuda_qpair_unwind(qpair, NULL, _qpair.sq, _qpair.cq);
				return -EIO;
			}
			if (!g_upcie_rte.connection.alive &&
			    !_cuda_iova_contiguous(state->dmem, _qpair.sq, sq_nbytes)) {
				XNVME_DEBUG("FAILED: SQ of depth(%u) spans GPU pages at "
					    "discontiguous IOVAs; use XNVME_QUEUE_SQ_HOSTMEM",
					    depth);
				_cuda_qpair_unwind(qpair, NULL, _qpair.sq, _qpair.cq);
				return -ERANGE;
			}
		}

		if (g_upcie_rte.connection.alive) {
			if (!slot->reg_offset) {
				XNVME_DEBUG("FAILED: device heap not registered with the server");
				err = -ENOTCONN;
			} else if (hostmem) {
				err = xnvme_be_upcie_cplane_alloc_qpair_at(
					state->ctrlr, slot->reg_offset, NVME_CPLANE_QUEUE_IN_HEAP,
					(uint64_t)(uintptr_t)_qpair.cq - heap_base, depth + 1,
					&served, NULL);
				if (!err) {
					qid = served.qid;
					host_sq->rpool = served.rpool;
					err = _cuda_host_sq_adopt(served.sq, sq_nbytes, host_sq,
								  &_qpair.sq);
					if (err) {
						xnvme_be_upcie_cplane_free_qpair_at(state->ctrlr,
										    qid);
						free(served.rpool);
						memset(host_sq, 0, sizeof(*host_sq));
					}
				}
			} else {
				err = xnvme_be_upcie_cplane_alloc_qpair_at(
					state->ctrlr, slot->reg_offset,
					(uint64_t)(uintptr_t)_qpair.sq - heap_base,
					(uint64_t)(uintptr_t)_qpair.cq - heap_base, depth + 1,
					NULL, &qid);
			}
		} else {
			if (hostmem) {
				err = _cuda_host_sq_alloc(sq_nbytes, host_sq, &_qpair.sq,
							  &sq_iova);
			} else {
				sq_iova = dmamem_va_to_iova(state->dmem, _qpair.sq);
			}
			if (!err) {
				err = xnvme_be_upcie_ctrlr_qpair_create_at(
					dev, sq_iova, dmamem_va_to_iova(state->dmem, _qpair.cq),
					depth + 1, &qid);
			}
		}
		if (err) {
			XNVME_DEBUG("FAILED: creating the queue; err(%d)", err);
			_cuda_qpair_unwind(qpair, host_sq, hostmem ? NULL : _qpair.sq, _qpair.cq);
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
				_cuda_qpair_unwind(qpair, host_sq, hostmem ? NULL : _qpair.sq,
						   _qpair.cq);
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
			_cuda_qpair_unwind(qpair, host_sq, hostmem ? NULL : _qpair.sq, _qpair.cq);
			return -EIO;
		}
		if (host_sq) {
			host_sq->queue = qpair;
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
xnvme_cuda_queue_create(struct xnvme_dev *dev, uint16_t depth, int opts,
			struct xnvme_cuda_queue **queue)
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
	err = _xnvme_cuda_queue_create(dev, depth, opts, queue);
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
		struct _cuda_host_sq *host_sq = _cuda_host_sq_find(queue);

		_cuda_qpair_release(dev, state->ctrlr, qpair.qid);
		if (host_sq) {
			_cuda_host_sq_free(host_sq);
		} else {
			cudamem_heap_block_free(&g_upcie_cuda_rte.cuda_heap, qpair.sq);
		}
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
			int XNVME_UNUSED(opts), struct xnvme_cuda_queue **XNVME_UNUSED(queue))
{
	return -ENOSYS;
}

void
xnvme_cuda_queue_destroy(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_cuda_queue *XNVME_UNUSED(queue))
{
}

#endif
