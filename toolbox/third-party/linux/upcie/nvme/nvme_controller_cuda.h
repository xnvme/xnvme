// SPDX-License-Identifier: BSD-3-Clause

/**
 * CUDA NVMe Controller Extension
 * ==============================
 *
 * This header extends the functionality defined in the uPCIe NVMe Controller
 * header `upcie/nvme/nvme_controller.h` with functions for CUDA compatible
 * NVMe controllers.
 * 
 * @file nvme_controller_cuda.h
 * @version 0.9.0
 */


/**
 * Deletes the CUDA submission-queue and CUDA completion-queue
 *
 * @param ctrlr Pointer to a pre-allocated NVMe controller
 * @param qpair Pointer to a queue-pair (from nvme_controller_cuda_create_io_qpair)
 * @param heap Pointer to CUDA Heap
 *
 */
static inline void
nvme_controller_cuda_delete_io_qpair(struct nvme_controller *ctrlr,
                                     struct nvme_qpair_cuda *qpair,
                                     struct cudamem_heap *heap)
{
	struct nvme_qpair_cuda _qpair = {0};
	int err;

	err = cuMemcpyDtoH(&_qpair, (CUdeviceptr)qpair, sizeof(_qpair));
	if (err) {
		UPCIE_DEBUG("FAILED: cuMemcpyDtoH(device QP -> host QP); CUresult(%d)", err);
		return;
	}

	err = cuMemHostUnregister(_qpair.sqdb);
	if (err) {
		UPCIE_DEBUG("FAILED: cuMemHostUnregister(sqdb); CUresult(%d)", err);
	}

	err = cuMemHostUnregister(_qpair.cqdb);
	if (err) {
		UPCIE_DEBUG("FAILED: cuMemHostUnregister(cqdb); CUresult(%d)", err);
	}
	
	{
		struct nvme_command cmd = {0};
		struct nvme_completion cpl = {0};

		cmd.opc = 0x0; ///< Delete I/O Submission Queue
		cmd.cdw10 = _qpair.qid;

		err = nvme_qpair_submit_sync(&ctrlr->aq, &cmd, ctrlr->timeout_ms, &cpl);
		if (err) {
			UPCIE_DEBUG("FAILED: nvme_qpair_submit_sync(Delete SQ); err(%d)", err);
		}
	}

	{
		struct nvme_command cmd = {0};
		struct nvme_completion cpl = {0};

		cmd.opc = 0x4; ///< Delete I/O completion Queue
		cmd.cdw10 = _qpair.qid;

		err = nvme_qpair_submit_sync(&ctrlr->aq, &cmd, ctrlr->timeout_ms, &cpl);
		if (err) {
			UPCIE_DEBUG("FAILED: nvme_qpair_submit_sync(Delete CQ); err(%d)", err);
		}
	}

	cudamem_heap_block_free(heap, _qpair.sq);
	cudamem_heap_block_free(heap, _qpair.cq);

	nvme_qid_free(ctrlr->qids, _qpair.qid);
}

/**
 * Allocates a CUDA submission-queue, a CUDA completion-queue, and wraps them in
 * the nvme_qpair struct
 *
 * The queues are allocated on `heap` and addressed through `dmem`, so
 * `dmem` must be the one wrapping `heap`, from dmamem_from_cuda_registry()
 * or dmamem_from_cuda_iommu_map_pa(). Whichever translator it carries then
 * applies: a physical address with iommu=pt/off, an IOVA where an IOMMU
 * translates for the controller.
 *
 * @param ctrlr Pointer to a pre-allocated NVMe controller
 * @param qpair Pointer to a pre-allocated queue-pair (using CUDA)
 * @param depth The queue depth
 * @param heap Pointer to CUDA Heap
 * @param dmem The dmamem wrapping `heap`; resolves the queue addresses
 *
 * @return 0 on success. Negative values indicate errno-style errors, positive values are CUresult errors.
 */
static inline int
nvme_controller_cuda_create_io_qpair(struct nvme_controller *ctrlr,
                                     struct nvme_qpair_cuda *qpair, uint16_t depth,
                                     struct cudamem_heap *heap, struct dmamem *dmem)
{
	/* _qpair declared at function scope so sq/cq remain accessible when building
	 * the Create I/O CQ/SQ admin commands below. This code is inlined here
	 * (instead of nvme_qpair_cuda.h) to avoid pulling in cudamem_heap.h into
	 * device-code compilation units.
	 */
	struct nvme_qpair_cuda _qpair = {0};
	uint64_t sq_iova, cq_iova;
	uint16_t qid;
	int err, del_err, qid_orphaned = 0;

	/* An unrelated dmamem resolves the queues to a plausible-looking address
	 * rather than to zero, which no later check would catch. */
	if (!heap || !dmem || (dmem->base_va != (void *)(uintptr_t)heap->vaddr)) {
		UPCIE_DEBUG("FAILED: dmem is not the dmamem wrapping heap");
		return -EINVAL;
	}

	err = nvme_qid_find_free(ctrlr->qids);
	if (err < 1) {
		return -ENOMEM;
	}
	qid = err;

	err = nvme_qid_alloc(ctrlr->qids, qid);
	if (err) {
		UPCIE_DEBUG("FAILED: nvme_qid_alloc(): err(%d)", err);
		return err;
	}

	{
		uint8_t *bar0 = ctrlr->func.bars[0].region;
		int dstrd = nvme_reg_cap_get_dstrd(nvme_mmio_cap_read(bar0));
		size_t nbytes = depth * 64;

		_qpair.sqdb = bar0 + 0x1000 + ((2 * qid) << (2 + dstrd));
		_qpair.cqdb = bar0 + 0x1000 + ((2 * qid + 1) << (2 + dstrd));
		_qpair.qid = qid;
		_qpair.tail = 0;
		_qpair.head = 0;
		_qpair.depth = depth;
		_qpair.phase = 1;
		_qpair.timeout_ms = ctrlr->timeout_ms;

		{
			int clock_rate_khz = 0;
			CUdevice dev;

			cuCtxGetDevice(&dev);
			cuDeviceGetAttribute(&clock_rate_khz, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, dev);
			_qpair.clocks_per_ms = (uint64_t)clock_rate_khz;
		}

		err = cuMemHostRegister(_qpair.sqdb, sizeof(uint32_t), CU_MEMHOSTREGISTER_IOMEMORY);
		if (err) {
			UPCIE_DEBUG("FAILED: cuMemHostRegister(sqdb); CUresult(%d)", err);
			goto free_qid;
		}

		err = cuMemHostRegister(_qpair.cqdb, sizeof(uint32_t), CU_MEMHOSTREGISTER_IOMEMORY);
		if (err) {
			UPCIE_DEBUG("FAILED: cuMemHostRegister(cqdb); CUresult(%d)", err);
			goto unregister_sqdb;
		}

		/* One element: the queue is created Physically Contiguous below. */
		_qpair.sq = cudamem_dma_alloc_array(heap, 1, nbytes);
		if (!_qpair.sq) {
			err = -errno;
			UPCIE_DEBUG("FAILED: cudamem_dma_alloc_array(sq); errno(%d)", err);
			goto unregister_cqdb;
		}

		_qpair.cq = cudamem_dma_alloc_array(heap, 1, nbytes);
		if (!_qpair.cq) {
			err = -errno;
			UPCIE_DEBUG("FAILED: cudamem_dma_alloc_array(cq); errno(%d)", err);
			goto free_sq;
		}

		/* The heap does not clear what it hands out, and a consumer reads a
		 * completion as ready from its phase tag. Stale bytes carrying the
		 * awaited phase are a completion that never happened. */
		err = cuMemsetD8((CUdeviceptr)_qpair.sq, 0, nbytes);
		if (err) {
			UPCIE_DEBUG("FAILED: cuMemsetD8(sq); CUresult(%d)", err);
			goto free_cq;
		}

		err = cuMemsetD8((CUdeviceptr)_qpair.cq, 0, nbytes);
		if (err) {
			UPCIE_DEBUG("FAILED: cuMemsetD8(cq); CUresult(%d)", err);
			goto free_cq;
		}

		sq_iova = dmamem_va_to_iova(dmem, _qpair.sq);
		cq_iova = dmamem_va_to_iova(dmem, _qpair.cq);
		if (!sq_iova || !cq_iova) {
			err = -EFAULT;
			UPCIE_DEBUG("FAILED: dmamem_va_to_iova(sq/cq); not registered");
			goto free_cq;
		}

		err = cuMemcpyHtoD((CUdeviceptr)qpair, &_qpair, sizeof(_qpair));
		if (err) {
			UPCIE_DEBUG("FAILED: cuMemcpyHtoD(host QP -> device QP); CUresult(%d)", err);
			goto free_cq;
		}
	}

	{
		struct nvme_command cmd = {0};
		struct nvme_completion cpl = {0};

		cmd.opc = 0x5; ///< Create I/O Completion Queue
		cmd.prp1 = cq_iova;
		cmd.cdw10 = ((depth - 1) << 16) | qid;
		cmd.cdw11 = 0x1; ///< Physically contigous

		err = nvme_qpair_submit_sync(&ctrlr->aq, &cmd, ctrlr->timeout_ms, &cpl);
		if (err) {
			UPCIE_DEBUG("FAILED: nvme_qpair_submit_sync(Create CQ); err(%d)", err);
			goto free_cq;
		}
	}

	{
		struct nvme_command cmd = {0};
		struct nvme_completion cpl = {0};

		cmd.opc = 0x1; ///< Create I/O Submission Queue
		cmd.prp1 = sq_iova;
		cmd.cdw10 = ((depth - 1) << 16) | qid;
		cmd.cdw11 = (qid << 16) | 0x1; ///< CQID and Physically contigous

		err = nvme_qpair_submit_sync(&ctrlr->aq, &cmd, ctrlr->timeout_ms, &cpl);
		if (err) {
			UPCIE_DEBUG("FAILED: nvme_qpair_submit_sync(Create SQ); err(%d)", err);
			goto delete_cq;
		}
	}

	return 0;

delete_cq:
	/* Kept out of err, which carries the failure being unwound. */
	del_err = nvme_controller_delete_io_cq(ctrlr, qid);
	if (del_err) {
		UPCIE_DEBUG("FAILED: nvme_controller_delete_io_cq(); err(%d)", del_err);

		/* The controller still holds a completion queue under this qid, so the
		 * qid is retired instead of returned to the pool */
		qid_orphaned = 1;
	}
free_cq:
	cudamem_heap_block_free(heap, _qpair.cq);
free_sq:
	cudamem_heap_block_free(heap, _qpair.sq);
unregister_cqdb:
	cuMemHostUnregister(_qpair.cqdb);
unregister_sqdb:
	cuMemHostUnregister(_qpair.sqdb);
free_qid:
	if (!qid_orphaned) {
		nvme_qid_free(ctrlr->qids, qid);
	}

	return err;
}