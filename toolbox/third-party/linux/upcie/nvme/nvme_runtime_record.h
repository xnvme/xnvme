// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Describing a live runtime to a process that does not have one
 * =============================================================
 *
 * A process holding a controller can let another process use it, but not by
 * handing over its struct. An inventory of what is reachable from
 * struct nvme_controller sorts into four kinds: values that mean the same
 * everywhere, addresses into the heap which are offsets wearing a disguise,
 * addresses into BAR0 which every process computes from its own mapping, and
 * things that must never leave the process that made them, the request pool
 * among them, since its entries carry a pointer belonging to whoever
 * submitted.
 *
 * So what travels is a record of the first kind and offsets of the second, and
 * the receiving process builds its own controller and queues from that plus
 * its own BAR mapping, its own view of the heap and its own request pool: the
 * fields of struct nvme_runtime_record and struct nvme_ioqpair are the whole of
 * the contract. Nothing is rebased and no pointer written by one process is
 * read by another.
 *
 * A client also has to translate, and physical addresses come from pagemap,
 * which it may not be able to read. So the server leaves a description of the
 * memory in the memory, and the record says where: see hostmem_shared_desc.
 *
 * The record is filled once, when the runtime is built, and is not written
 * again. That is deliberate: the queue identifier space, the heap allocator
 * and the admin queue stay with the process that owns the controller, and a
 * client receives a allocation naming a queue that has already been created for
 * it. Nothing here needs a lock, because nothing here changes.
 *
 * @file nvme_runtime_record.h
 * @version 0.10.0
 */

#ifndef __UPCIE_NVME_RUNTIME_RECORD_H
#define __UPCIE_NVME_RUNTIME_RECORD_H

/**
 * Bumped when the layout of the record, or of anything it describes, changes.
 *
 * The record describes queue memory whose layout comes from this library, so a
 * client built against a different version cannot be trusted to read it.
 */
#define NVME_RUNTIME_RECORD_VERSION 1U

/**
 * An immutable description of a controller another process has opened
 */
struct nvme_runtime_record {
	uint32_t version;    ///< NVME_RUNTIME_RECORD_VERSION as written
	uint32_t timeout_ms; ///< Command timeout, derived from CAP.TO
	uint64_t cap;        ///< Controller capabilities as read at open
	uint32_t cc;         ///< Controller configuration as written at open
	uint32_t _rsvd;
	uint64_t heap_nbytes; ///< Size of the heap the offsets below refer into
	uint64_t desc_offset; ///< Offset of the heap's hostmem_shared_desc
	char bdf[16];         ///< The controller, for a client to check it agrees
};

/**
 * A queue created on a client's behalf, described in terms it can resolve
 *
 * The offsets are into the heap the record names; the client turns them into
 * addresses with its own mapping, and derives the doorbells from its own BAR0
 * as nvme_qpair_dmamem_init() does: 0x1000 + ((2 * qid) << (2 + CAP.DSTRD))
 * for the submission doorbell, and the entry after it for the completion
 * doorbell.
 */
struct nvme_ioqpair {
	uint64_t sq_offset;  ///< Submission queue, as a heap offset
	uint64_t cq_offset;  ///< Completion queue, as a heap offset
	uint64_t prp_offset; ///< PRP scratch for the client's request pool
	uint32_t qid;        ///< The identifier allocated, never zero
	uint16_t depth;      ///< Entries in the queue pair
	uint16_t _rsvd;
};

/**
 * Fill a record from a controller this process opened
 *
 * @param ctrlr An opened controller
 * @param heap_nbytes Size of the region the offsets refer into. The caller
 * names it because it is not always a hostmem_heap: a runtime built on dmamem
 * has one all the same, and this has no business knowing which
 * @param record Pre-allocated record to fill
 *
 * @return 0 on success, negative errno on error
 */
static inline int
nvme_runtime_record_export(const struct nvme_controller *ctrlr, size_t heap_nbytes,
			   struct nvme_runtime_record *record)
{
	if (!ctrlr || !record || !heap_nbytes) {
		return -EINVAL;
	}

	memset(record, 0, sizeof(*record));
	record->version = NVME_RUNTIME_RECORD_VERSION;
	record->timeout_ms = (uint32_t)ctrlr->timeout_ms;
	record->cap = nvme_mmio_cap_read(ctrlr->func.bars[0].region);
	record->cc = ctrlr->cc;
	record->heap_nbytes = heap_nbytes;
	snprintf(record->bdf, sizeof(record->bdf), "%s", ctrlr->func.bdf);

	return 0;
}

#endif /* __UPCIE_NVME_RUNTIME_RECORD_H */
