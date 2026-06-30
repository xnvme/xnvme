// SPDX-License-Identifier: BSD-3-Clause

/**
 * HIP NVMe Controller Extension
 * ==============================
 *
 * This header extends the functionality defined in the uPCIe NVMe Controller
 * header `upcie/nvme/nvme_controller.h` with functions for HIP compatible
 * NVMe controllers.
 */


/**
 * Deletes the HIP submission-queue and HIP completion-queue
 *
 * @param ctrlr Pointer to a pre-allocated NVMe controller
 * @param qpair Pointer to a queue-pair (from nvme_controller_hip_create_io_qpair)
 * @param heap Pointer to HIP Heap
 *
 */
static inline void
nvme_controller_hip_delete_io_qpair(struct nvme_controller *ctrlr,
                                     struct nvme_qpair_hip *qpair,
                                     struct hipmem_heap *heap)
{
	struct nvme_qpair_hip _qpair = {0};
	int err;

	err = hipMemcpyDtoH(&_qpair, (hipDeviceptr_t)qpair, sizeof(_qpair));
	if (err) {
		UPCIE_DEBUG("FAILED: hipMemcpyDtoH(device QP -> host QP); hipError_t(%d)", err);
	}

	err = hipHostUnregister(_qpair.sqdb);
	if (err) {
		UPCIE_DEBUG("FAILED: hipHostUnregister(sqdb); hipError_t(%d)", err);
	}

	err = hipHostUnregister(_qpair.cqdb);
	if (err) {
		UPCIE_DEBUG("FAILED: hipHostUnregister(cqdb); hipError_t(%d)", err);
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

	hipmem_heap_block_free(heap, _qpair.sq);
	hipmem_heap_block_free(heap, _qpair.cq);
}

/**
 * Allocates a HIP submission-queue, a HIP completion-queue, and wraps them in
 * the nvme_qpair struct
 *
 * @param ctrlr Pointer to a pre-allocated NVMe controller
 * @param qpair Pointer to a pre-allocated queue-pair (using HIP)
 * @param depth The queue depth
 * @param heap Pointer to HIP Heap
 *
 * @return 0 on success. Negative values indicate errno-style errors, positive values are hipError_t errors.
 */
static inline int
nvme_controller_hip_create_io_qpair(struct nvme_controller *ctrlr,
                                     struct nvme_qpair_hip *qpair, uint16_t depth,
                                     struct hipmem_heap *heap)
{
	/* _qpair declared at function scope so sq/cq remain accessible when building
	 * the Create I/O CQ/SQ admin commands below. This code is inlined here
	 * (instead of nvme_qpair_hip.h) to avoid pulling in hipmem_heap.h into
	 * device-code compilation units.
	 */
	struct nvme_qpair_hip _qpair = {0};
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
			int dev;

			hipGetDevice(&dev);
			hipDeviceGetAttribute(&clock_rate_khz, hipDeviceAttributeClockRate, dev);
			_qpair.clocks_per_ms = (uint64_t)clock_rate_khz;
		}

		err = hipHostRegister(_qpair.sqdb, sizeof(uint32_t), hipHostRegisterIoMemory);
		if (err) {
			UPCIE_DEBUG("FAILED: hipHostRegister(sqdb); hipError_t(%d)", err);
			return err;
		}

		err = hipHostRegister(_qpair.cqdb, sizeof(uint32_t), hipHostRegisterIoMemory);
		if (err) {
			UPCIE_DEBUG("FAILED: hipHostRegister(cqdb); hipError_t(%d)", err);
			hipHostUnregister(_qpair.sqdb);
			return err;
		}

		_qpair.sq = hipmem_heap_block_alloc(heap, nbytes);
		if (!_qpair.sq) {
			err = -errno;
			UPCIE_DEBUG("FAILED: hipmem_heap_block_alloc(sq); errno(%d)", err);
			hipHostUnregister(_qpair.sqdb);
			hipHostUnregister(_qpair.cqdb);
			return err;
		}

		_qpair.cq = hipmem_heap_block_alloc(heap, nbytes);
		if (!_qpair.cq) {
			err = -errno;
			UPCIE_DEBUG("FAILED: hipmem_heap_block_alloc(cq); errno(%d)", err);
			hipHostUnregister(_qpair.sqdb);
			hipHostUnregister(_qpair.cqdb);
			hipmem_heap_block_free(heap, _qpair.sq);
			return err;
		}

		err = hipMemcpyHtoD((hipDeviceptr_t)qpair, &_qpair, sizeof(_qpair));
		if (err) {
			UPCIE_DEBUG("FAILED: hipMemcpyHtoD(host QP -> device QP); hipError_t(%d)", err);
			hipHostUnregister(_qpair.sqdb);
			hipHostUnregister(_qpair.cqdb);
			hipmem_heap_block_free(heap, _qpair.sq);
			hipmem_heap_block_free(heap, _qpair.cq);
			return err;
		}
	}

	{
		struct nvme_command cmd = {0};
		struct nvme_completion cpl = {0};

		cmd.opc = 0x5; ///< Create I/O Completion Queue
		cmd.prp1 = hipmem_heap_block_vtp(heap, _qpair.cq);
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
		cmd.prp1 = hipmem_heap_block_vtp(heap, _qpair.sq);
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