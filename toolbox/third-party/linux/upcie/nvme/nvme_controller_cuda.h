// SPDX-License-Identifier: BSD-3-Clause

/**
 * CUDA NVMe Controller Extension
 * ==============================
 *
 * This header extends the functionality defined in the uPCIe NVMe Controller
 * header `upcie/nvme/nvme_controller.h` with functions for CUDA compatible
 * NVMe controllers.
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
	int err;
	struct nvme_qpair_cuda _qpair = {0};
	err = cuMemcpyDtoH(&_qpair, (CUdeviceptr)qpair, sizeof(_qpair));
	if (err) {
		UPCIE_DEBUG("FAILED: cuMemcpyDtoH(device QP -> host QP); CUresult(%d)", err);
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
}

/**
 * Allocates a CUDA submission-queue, a CUDA completion-queue, and wraps them in
 * the nvme_qpair struct
 *
 * @param ctrlr Pointer to a pre-allocated NVMe controller
 * @param qpair Pointer to a pre-allocated queue-pair (using CUDA)
 * @param depth The queue depth
 * @param heap Pointer to CUDA Heap
 *
 * @return 0 on success. Negative values indicate errno-style errors, positive values are CUresult errors.
 */
static inline int
nvme_controller_cuda_create_io_qpair(struct nvme_controller *ctrlr,
                                     struct nvme_qpair_cuda *qpair, uint16_t depth,
                                     struct cudamem_heap *heap)
{
	/* _qpair declared at function scope so sq/cq remain accessible when building
	 * the Create I/O CQ/SQ admin commands below. This code is inlined here
	 * (instead of nvme_qpair_cuda.h) to avoid pulling in cudamem_heap.h into
	 * device-code compilation units.
	 */
	struct nvme_qpair_cuda _qpair = {0};
	uint16_t qid;
	int err;

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
			return err;
		}

		err = cuMemHostRegister(_qpair.cqdb, sizeof(uint32_t), CU_MEMHOSTREGISTER_IOMEMORY);
		if (err) {
			UPCIE_DEBUG("FAILED: cuMemHostRegister(cqdb); CUresult(%d)", err);
			cuMemHostUnregister(_qpair.sqdb);
			return err;
		}

		_qpair.sq = cudamem_heap_block_alloc(heap, nbytes);
		if (!_qpair.sq) {
			err = -errno;
			UPCIE_DEBUG("FAILED: cudamem_heap_block_alloc(sq); errno(%d)", err);
			cuMemHostUnregister(_qpair.sqdb);
			cuMemHostUnregister(_qpair.cqdb);
			return err;
		}

		_qpair.cq = cudamem_heap_block_alloc(heap, nbytes);
		if (!_qpair.cq) {
			err = -errno;
			UPCIE_DEBUG("FAILED: cudamem_heap_block_alloc(cq); errno(%d)", err);
			cuMemHostUnregister(_qpair.sqdb);
			cuMemHostUnregister(_qpair.cqdb);
			cudamem_heap_block_free(heap, _qpair.sq);
			return err;
		}

		err = cuMemcpyHtoD((CUdeviceptr)qpair, &_qpair, sizeof(_qpair));
		if (err) {
			UPCIE_DEBUG("FAILED: cuMemcpyHtoD(host QP -> device QP); CUresult(%d)", err);
			cuMemHostUnregister(_qpair.sqdb);
			cuMemHostUnregister(_qpair.cqdb);
			cudamem_heap_block_free(heap, _qpair.sq);
			cudamem_heap_block_free(heap, _qpair.cq);
			return err;
		}
	}

	{
		struct nvme_command cmd = {0};
		struct nvme_completion cpl = {0};

		cmd.opc = 0x5; ///< Create I/O Completion Queue
		cmd.prp1 = cudamem_heap_block_vtp(heap, _qpair.cq);
		cmd.cdw10 = ((depth - 1) << 16) | qid;
		cmd.cdw11 = 0x1; ///< Physically contigous

		err = nvme_qpair_submit_sync(&ctrlr->aq, &cmd, ctrlr->timeout_ms, &cpl);
		if (err) {
			UPCIE_DEBUG("FAILED: nvme_qpair_submit_sync(); err(%d)", err);
			return err;
		}
	}

	{
		struct nvme_command cmd = {0};
		struct nvme_completion cpl = {0};

		cmd.opc = 0x1; ///< Create I/O Submission Queue
		cmd.prp1 = cudamem_heap_block_vtp(heap, _qpair.sq);
		cmd.cdw10 = ((depth - 1) << 16) | qid;
		cmd.cdw11 = (qid << 16) | 0x1; ///< CQID and Physically contigous

		err = nvme_qpair_submit_sync(&ctrlr->aq, &cmd, ctrlr->timeout_ms, &cpl);
		if (err) {
			UPCIE_DEBUG("FAILED: nvme_qpair_submit_sync(); err(%d)", err);
			return err;
		}
	}

	return 0;
}