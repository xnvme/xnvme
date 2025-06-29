// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Rudimentary Representation of Controller, BAR Mapping, Registers, and Derived Values
 * ====================================================================================
 *
 * This header defines basic structures and access patterns for working with an NVMe controller,
 * including BAR-space mappings, controller registers, and values derived from register content.
 *
 * @file nvme_controller.h
 * @version 0.3.2
 */

/**
 * This is one way of combining the various components needed
 */
struct nvme_controller {
	struct pci_func func;                 ///< The PCIe function and mapped bars
	struct nvme_qpair aq;                 ///< Admin qpair
	uint64_t qids[NVME_QID_BITMAP_WORDS]; ///< Allocation status of IO queues

	struct hostmem_heap *heap; ///< Heap for DMA-capable memory
	void *buf;                 ///< IO-buffer for identify-commands, io-qpair-creation etc.

	uint32_t csts; ///< Controller Status Register Value
	uint32_t cap;  ///< Controller Capabilities Register Value
	uint32_t cc;   ///< Controller configuration Register Value

	int timeout_ms; ///< Command timeout in milliseconds (derived from cap.to)
};

static inline void
nvme_controller_close(struct nvme_controller *ctrlr)
{
	hostmem_dma_free(ctrlr->heap, ctrlr->buf);
	pci_func_close(&ctrlr->func);
	memset(ctrlr, 0, sizeof(*memset));
}

/**
 * Disables the NVMe controller at 'bdf', sets up admin-queues and enables it again
 */
static inline int
nvme_controller_open(struct nvme_controller *ctrlr, const char *bdf, struct hostmem_heap *heap)
{
	void *bar0;
	int err;

	memset(ctrlr, 0, sizeof(*ctrlr));
	ctrlr->heap = heap;

	ctrlr->buf = hostmem_dma_malloc(ctrlr->heap, 4096);
	if (!ctrlr->buf) {
		UPCIE_DEBUG("FAILED: hostmem_dma_malloc(buf); errno(%d)\n", errno);
		return -errno;
	}
	memset(ctrlr->buf, 0, 4096);

	nvme_qid_bitmap_init(ctrlr->qids);

	err = pci_func_open(bdf, &ctrlr->func);
	if (err) {
		UPCIE_DEBUG("FAILED: pci_func_open(%.*s); err(%d)", 13, bdf, err);
		return -err;
	}

	err = pci_bar_map(ctrlr->func.bdf, 0, &ctrlr->func.bars[0]);
	if (err) {
		UPCIE_DEBUG("FAILED: pci_bar_map(BAR0); err(%d)", err);
		return -err;
	}
	bar0 = ctrlr->func.bars[0].region;

	ctrlr->timeout_ms = nvme_reg_cap_get_to(nvme_mmio_cap_read(bar0)) * 500;

	nvme_mmio_cc_disable(bar0);

	err = nvme_mmio_csts_wait_until_not_ready(bar0, ctrlr->timeout_ms);
	if (err) {
		UPCIE_DEBUG("FAILED: nvme_mmio_csts_wait_until_ready(); err(%d)\n", err);
		return -err;
	}

	err = nvme_qpair_init(&ctrlr->aq, 0, 256, ctrlr->func.bars[0].region, ctrlr->heap);
	if (err) {
		UPCIE_DEBUG("FAILED: nvme_qpair_init(); err(%d)", err);
		return -err;
	}

	nvme_mmio_aq_setup(bar0, hostmem_dma_v2p(heap, ctrlr->aq.sq),
			   hostmem_dma_v2p(heap, ctrlr->aq.cq), ctrlr->aq.depth);

	{
		uint32_t cc = 0;
		cc = nvme_reg_cc_set_css(cc, 0x0);
		cc = nvme_reg_cc_set_shn(cc, 0x0);
		cc = nvme_reg_cc_set_mps(cc, 0x0);
		cc = nvme_reg_cc_set_ams(cc, 0x0);
		cc = nvme_reg_cc_set_iosqes(cc, 6);
		cc = nvme_reg_cc_set_iocqes(cc, 4);
		cc = nvme_reg_cc_set_en(cc, 0x1);

		nvme_mmio_cc_write(bar0, cc);
	}

	err = nvme_mmio_csts_wait_until_ready(bar0, ctrlr->timeout_ms);
	if (err) {
		UPCIE_DEBUG("FAILED: nvme_mmio_csts_wait_until_ready(); err(%d)", err);
		return -err;
	}

	return 0;
}

/**
 * Allocates a submission-queue, a completion-queue, and wraps them in the nvme_qpair struct
 */
static inline int
nvme_controller_create_io_qpair(struct nvme_controller *ctrlr, struct nvme_qpair *qpair,
				uint16_t depth)
{
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

	err = nvme_qpair_init(qpair, qid, depth, ctrlr->func.bars[0].region, ctrlr->heap);
	if (err) {
		UPCIE_DEBUG("FAILED: nvme_qpair_init(); err(%d)\n", err);
		nvme_qid_free(ctrlr->qids, depth);

		return err;
	}

	{
		struct nvme_command cmd = {0};
		struct nvme_completion cpl = {0};

		cmd.opc = 0x5; ///< Create I/O Completion Queue
		cmd.prp1 = hostmem_dma_v2p(ctrlr->heap, qpair->cq);
		cmd.cdw10 = ((depth - 1) << 16) | qid;
		cmd.cdw11 = 0x1; ///< Physically contigous

		err = nvme_qpair_submit_sync(&ctrlr->aq, &cmd, ctrlr->timeout_ms, &cpl);
		if (err) {
			UPCIE_DEBUG("FAILED: nvme_qpair_submit_sync(); err(%d)\n", err);
			return err;
		}
	}

	{
		struct nvme_command cmd = {0};
		struct nvme_completion cpl = {0};

		cmd.opc = 0x1; ///< Create I/O Submission Queue
		cmd.prp1 = hostmem_dma_v2p(ctrlr->heap, qpair->sq);
		cmd.cdw10 = ((depth - 1) << 16) | qid;
		cmd.cdw11 = (qid << 16) | 0x1; ///< CQID and Physically contigous

		err = nvme_qpair_submit_sync(&ctrlr->aq, &cmd, ctrlr->timeout_ms, &cpl);
		if (err) {
			UPCIE_DEBUG("FAILED: nvme_qpair_submit_sync(); err(%d)\n", err);
			return err;
		}
	}

	return 0;
}