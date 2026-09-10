// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <errno.h>
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_upcie_cuda.h>

/**
 * A GPU-issued queue's submission queue when asked for with
 * XNVME_QUEUE_SQ_HOSTMEM: host memory, mapped into the GPU's address space
 *
 * The controller fetches submission entries, and a fetch from device memory is
 * a PCIe read across the root complex, a round trip of microseconds against
 * the few hundred nanoseconds of a DRAM read; a controller keeps only so many
 * in flight, so that latency caps its submission rate. The GPU's writes into
 * host memory are posted and cost nothing of the sort, so with the option the
 * queue sits where the reader is fast and the writer does not care, at the
 * price of every entry crossing PCIe on its way in. The completion queue stays
 * in device memory either way: there the GPU reads and the controller writes.
 * The record is what destroy needs to give the host block back, keyed by the
 * device-side queue it was made for; a queue without one submits from device
 * memory.
 */
struct _cuda_host_sq {
	struct xnvme_cuda_queue *queue; ///< The device-side queue, NULL when the slot is free
	void *va;                       ///< The submission queue, in a heap this process maps
	size_t offset; ///< Its offset in this process's own heap; unused when served
	size_t nbytes; ///< What was registered with CUDA, whole pages
	void *rpool;   ///< Served: the pool the client-side qpair came with
	int served;    ///< The block is the server's; only the registration is ours
};

static struct _cuda_host_sq g_host_sqs[XNVME_BE_UPCIE_GPU_CTRLRS_MAX * 64];

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

/**
 * Take a submission queue from the host heap and map it for the GPU
 *
 * @param nbytes The queue's size
 * @param rec Filled with what destroy needs
 * @param sq_dev Set to the device pointer the kernel writes through
 * @param sq_iova Set to the address the controller fetches from
 *
 * @return 0 on success, negative errno on failure
 */
static int
_cuda_host_sq_alloc(size_t nbytes, struct _cuda_host_sq *rec, void **sq_dev, uint64_t *sq_iova)
{
	const size_t page = 4096;
	CUresult res;
	int err;

	rec->nbytes = (nbytes + page - 1) & ~(page - 1);

	xnvme_be_upcie_heap_lock();
	err = dmamem_heap_alloc_aligned(&g_upcie_rte.mem.heap, rec->nbytes, page, &rec->offset);
	xnvme_be_upcie_heap_unlock();
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
	xnvme_be_upcie_heap_lock();
	dmamem_heap_free(&g_upcie_rte.mem.heap, rec->offset);
	xnvme_be_upcie_heap_unlock();
	rec->va = NULL;

	return err;
}

/**
 * Map a submission queue the server placed in its heap for the GPU
 *
 * The server hands out queues only in memory it holds or the client
 * registered, and its heap is host memory this process already maps, so the
 * queue sits in exactly the right place; what remains is the GPU's view of it.
 *
 * @param sq_va The submission queue, within this process's mapping of the heap
 * @param nbytes The queue's size
 * @param rec Filled with what destroy needs
 * @param sq_dev Set to the device pointer the kernel writes through
 *
 * @return 0 on success, negative errno on failure
 */
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
		xnvme_be_upcie_heap_lock();
		dmamem_heap_free(&g_upcie_rte.mem.heap, rec->offset);
		xnvme_be_upcie_heap_unlock();
	}
	free(rec->rpool);
	memset(rec, 0, sizeof(*rec));
}

/**
 * Give back what a queue was built from
 *
 * @param qpair The device-side queue-pair allocation
 * @param host_sq Submission queue in host memory, or NULL
 * @param dev_sq Submission queue in device memory, or NULL
 * @param cq Completion queue, or NULL
 */
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
xnvme_cuda_queue_create(struct xnvme_dev *dev, uint16_t depth, int opts,
			struct xnvme_cuda_queue **queue)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	struct xnvme_cuda_queue *qpair;
	int hostmem = opts & XNVME_QUEUE_SQ_HOSTMEM;
	int err;

	err = cuMemAlloc((CUdeviceptr *)&qpair, sizeof(struct xnvme_cuda_queue));
	if (err) {
		XNVME_DEBUG("FAILED: cuMemAlloc(qpair); CUresult(%d)", err);
		return -ENOMEM;
	}

	/* The completion queue lives in this process's device memory either
	 * way, since the kernel that reaps it has to read it fast. The
	 * submission queue lives there too unless XNVME_QUEUE_SQ_HOSTMEM asks
	 * for host memory, see struct _cuda_host_sq: then it comes from this
	 * process's heap on a controller it owns, and from the server's heap on
	 * a served one, asked for as a queue whose completion queue alone is
	 * placed in the device memory this process registered. What also
	 * differs is who holds the identifier space and the admin queue the
	 * create commands go on: a served controller's belong to the server, so
	 * it is asked; one this process opened is its own to use.
	 *
	 * The spec says that where memory ordering is not guaranteed one should
	 * leave room in the queue to avoid races, hence one more entry than
	 * asked for. */
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

		_qpair.cq = cudamem_dma_alloc_array(&g_upcie_cuda_rte.cuda_heap, 1, cq_nbytes);
		if (!_qpair.cq) {
			XNVME_DEBUG("FAILED: cudamem_dma_alloc_array(cq); errno(%d)", errno);
			cuMemFree((CUdeviceptr)qpair);
			return -ENOMEM;
		}
		if (hostmem) {
			host_sq = _cuda_host_sq_find(NULL);
			if (!host_sq) {
				XNVME_DEBUG("FAILED: no room to record another GPU-issued queue");
				_cuda_qpair_unwind(qpair, NULL, NULL, _qpair.cq);
				return -ENOSPC;
			}
		} else {
			_qpair.sq =
				cudamem_dma_alloc_array(&g_upcie_cuda_rte.cuda_heap, 1, sq_nbytes);
			if (!_qpair.sq) {
				XNVME_DEBUG("FAILED: cudamem_dma_alloc_array(sq); errno(%d)",
					    errno);
				_cuda_qpair_unwind(qpair, NULL, NULL, _qpair.cq);
				return -ENOMEM;
			}
		}

		if (g_upcie_rte.connection.alive) {
			if (!slot->reg_offset) {
				XNVME_DEBUG("FAILED: device heap not registered with the server");
				err = -ENOTCONN;
			} else if (hostmem) {
				/* The completion queue is named by offset into the
				 * region this process registered, so the server
				 * resolves the address rather than taking one it was
				 * handed; the submission queue it takes from its own
				 * heap and describes by offset in return. */
				err = xnvme_be_upcie_cplane_alloc_qpair_cq_at(
					state->ctrlr, &served, depth + 1, slot->reg_offset,
					(uint64_t)(uintptr_t)_qpair.cq - heap_base);
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
				/* Both queues named by offset into the registered
				 * region, for the same reason. */
				err = xnvme_be_upcie_cplane_alloc_qpair_at(
					state->ctrlr, slot->reg_offset,
					(uint64_t)(uintptr_t)_qpair.sq - heap_base,
					(uint64_t)(uintptr_t)_qpair.cq - heap_base, depth + 1,
					&qid);
			}
		} else {
			if (hostmem) {
				err = _cuda_host_sq_alloc(sq_nbytes, host_sq, &_qpair.sq,
							  &sq_iova);
			} else {
				sq_iova = dmamem_va_to_iova(state->dmem, _qpair.sq);
			}
			if (!err) {
				/* The device's table: under a translating IOMMU the
				 * heap has an IOVA per controller, and the shared
				 * table is empty. */
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
	 * memory it sat in was always this process's to free; a submission
	 * queue with a host record was in host memory, any other in the device
	 * heap. */
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

	cuMemFree((CUdeviceptr)queue);
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
