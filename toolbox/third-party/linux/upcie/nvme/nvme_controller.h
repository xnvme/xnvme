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
 * @version 0.10.0
 */

/**
 * This is one way of combining the various components needed
 */
struct nvme_controller {
	struct pci_func func;                 ///< The PCIe function and mapped bars
	struct nvme_qpair aq;                 ///< Admin qpair
	uint64_t qids[NVME_QID_BITMAP_WORDS]; ///< Allocation status of IO queues

	uint32_t csts; ///< Controller Status Register Value
	uint32_t cap;  ///< Controller Capabilities Register Value
	uint32_t cc;   ///< Controller configuration Register Value

	int timeout_ms; ///< Command timeout in milliseconds (derived from cap.to)
};

/**
 * Sends a Delete I/O Completion Queue admin command for `qid`
 *
 * @param ctrlr Pointer to a pre-allocated NVMe controller
 * @param qid The identifier of the completion-queue to delete
 *
 * @return 0 on success, negative errno on error.
 */
static inline int
nvme_controller_delete_io_cq(struct nvme_controller *ctrlr, uint16_t qid)
{
	struct nvme_command cmd = {0};
	struct nvme_completion cpl = {0};

	cmd.opc = 0x4; ///< Delete I/O Completion Queue
	cmd.cdw10 = qid;

	return nvme_qpair_submit_sync(&ctrlr->aq, &cmd, ctrlr->timeout_ms, &cpl);
}
