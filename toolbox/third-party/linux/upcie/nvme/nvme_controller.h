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
 * Admin command timeout in milliseconds, the default admin_timeout of the Linux NVMe driver
 */
#define NVME_ADMIN_TIMEOUT_MS 60000

/**
 * What a controller is, as another process using it needs to know
 *
 * Values only, no pointers, so it can be handed to another process as it is.
 * A process given it builds a controller of its own around it, with its own
 * BAR0 mapping, heap view and admin-less queues.
 */
struct nvme_controller_base {
	uint64_t cap;              ///< Controller Capabilities, as read at open
	uint32_t cc;               ///< Controller Configuration, as written at enable
	uint32_t timeout_ms;       ///< CSTS.RDY transition timeout in milliseconds, from CAP.TO
	uint32_t admin_timeout_ms; ///< Admin command timeout in milliseconds
	uint32_t _rsvd;
};

UPCIE_STATIC_ASSERT(sizeof(struct nvme_controller_base) == 24,
		    "nvme_controller_base changed size; another process reads it as it is")

/**
 * This is one way of combining the various components needed
 */
struct nvme_controller {
	struct pci_func func;                 ///< The PCIe function and mapped bars
	struct nvme_qpair aq;                 ///< Admin qpair
	uint64_t qids[NVME_QID_BITMAP_WORDS]; ///< Allocation status of IO queues

	uint32_t csts; ///< Controller Status Register Value

	struct nvme_controller_base base; ///< What another process using it needs to know
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

	return nvme_qpair_submit_sync(&ctrlr->aq, &cmd, ctrlr->base.admin_timeout_ms, &cpl);
}
