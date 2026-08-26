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
 * the receiving process builds its own controller from that plus its own BAR
 * mapping, its own view of the heap and its own request pool. Nothing is
 * rebased and no pointer written by one process is read by another.
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
 * @version 0.9.0
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
 * addresses with its own mapping, and derives the doorbells from its own BAR0.
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

/**
 * Describe a queue pair this process created for another to use
 *
 * @param ctrlr The controller the queue belongs to
 * @param qpair A queue pair created with nvme_controller_create_io_qpair
 * @param prps PRP scratch the server allocated for the client, of
 * NVME_REQUEST_POOL_LEN pages
 * @param allocation Pre-allocated allocation to fill
 *
 * @return 0 on success, negative errno on error
 */
static inline int
nvme_ioqpair_export(const struct nvme_controller *ctrlr, const struct nvme_qpair *qpair,
		    const void *prps, struct nvme_ioqpair *allocation)
{
	const char *base;

	if (!ctrlr || !qpair || !prps || !allocation || !ctrlr->heap) {
		return -EINVAL;
	}
	if (!qpair->qid) {
		return -EINVAL; // The admin queue is never allocated
	}

	base = (const char *)ctrlr->heap->memory.virt;

	memset(allocation, 0, sizeof(*allocation));
	allocation->sq_offset = (uint64_t)((const char *)qpair->sq - base);
	allocation->cq_offset = (uint64_t)((const char *)qpair->cq - base);
	allocation->prp_offset = (uint64_t)((const char *)prps - base);
	allocation->qid = qpair->qid;
	allocation->depth = qpair->depth;

	return 0;
}

/**
 * Build a controller from a record, without touching the device
 *
 * The controller is usable for submitting on allocated queues. It does not own
 * the admin queue, the queue identifier space or the heap, so closing it must
 * not go through nvme_controller_close().
 *
 * @param ctrlr Pre-allocated controller to fill
 * @param record A record from nvme_runtime_record_export
 * @param bar0 This process's mapping of BAR0
 * @param heap This process's mapping of the heap the record names
 *
 * @return 0 on success, negative errno on error
 */
static inline int
nvme_runtime_record_import(struct nvme_controller *ctrlr, const struct nvme_runtime_record *record,
			   void *bar0, struct hostmem_heap *heap)
{
	if (!ctrlr || !record || !bar0 || !heap) {
		return -EINVAL;
	}
	if (record->version != NVME_RUNTIME_RECORD_VERSION) {
		UPCIE_DEBUG("FAILED: record version(%u), expected(%u)", record->version,
			    NVME_RUNTIME_RECORD_VERSION);
		return -EPROTO;
	}
	if (heap->memory.size != record->heap_nbytes) {
		UPCIE_DEBUG("FAILED: heap is %zu bytes, record says %u", heap->memory.size,
			    record->heap_nbytes);
		return -EINVAL;
	}

	memset(ctrlr, 0, sizeof(*ctrlr));
	ctrlr->heap = heap;
	ctrlr->timeout_ms = (int)record->timeout_ms;
	ctrlr->cc = record->cc;
	ctrlr->func.bars[0].region = bar0;
	ctrlr->func.bars[0].fd = -1; // Not ours; the server holds it
	snprintf(ctrlr->func.bdf, sizeof(ctrlr->func.bdf), "%s", record->bdf);

	return 0;
}

/**
 * Build a queue pair from a allocation, without creating anything on the device
 *
 * The queue itself already exists; this attaches to it. The request pool is
 * allocated here because it is this process's, and points at the scratch the
 * allocation names, since a client cannot allocate from the server's heap. Release
 * it with nvme_ioqpair_release().
 *
 * @param qpair Pre-allocated queue pair to fill
 * @param allocation A allocation from nvme_ioqpair_export
 * @param ctrlr A controller from nvme_runtime_record_import
 *
 * @return 0 on success, negative errno on error
 */
static inline int
nvme_ioqpair_import(struct nvme_qpair *qpair, const struct nvme_ioqpair *allocation,
		    struct nvme_controller *ctrlr)
{
	int dstrd;
	char *base;
	int err;

	if (!qpair || !allocation || !ctrlr || !ctrlr->heap || !allocation->qid) {
		return -EINVAL;
	}
	if ((allocation->sq_offset >= ctrlr->heap->memory.size) ||
	    (allocation->cq_offset >= ctrlr->heap->memory.size)) {
		UPCIE_DEBUG("FAILED: allocation offsets fall outside the heap");
		return -ERANGE;
	}

	base = (char *)ctrlr->heap->memory.virt;
	dstrd = nvme_reg_cap_get_dstrd(nvme_mmio_cap_read(ctrlr->func.bars[0].region));

	memset(qpair, 0, sizeof(*qpair));
	qpair->heap = ctrlr->heap;
	qpair->qid = allocation->qid;
	qpair->depth = allocation->depth;
	qpair->sq = base + allocation->sq_offset;
	qpair->cq = base + allocation->cq_offset;
	qpair->sqdb = (char *)ctrlr->func.bars[0].region + 0x1000 +
		      ((2 * allocation->qid) << (2 + dstrd));
	qpair->cqdb = (char *)ctrlr->func.bars[0].region + 0x1000 +
		      ((2 * allocation->qid + 1) << (2 + dstrd));
	qpair->tail = 0;
	qpair->tail_last_written = UINT16_MAX;
	qpair->head = 0;
	qpair->phase = 1;

	qpair->rpool = (struct nvme_request_pool *)calloc(1, sizeof(*qpair->rpool));
	if (!qpair->rpool) {
		return -errno;
	}
	nvme_request_pool_init(qpair->rpool);

	err = nvme_request_pool_attach_prps(qpair->rpool, ctrlr->heap, allocation->prp_offset);
	if (err) {
		free(qpair->rpool);
		qpair->rpool = NULL;
		return err;
	}

	return 0;
}

/**
 * Release what nvme_ioqpair_import() allocated, leaving the queue alone
 *
 * @param qpair A queue pair from nvme_ioqpair_import
 */
static inline void
nvme_ioqpair_release(struct nvme_qpair *qpair)
{
	if (!qpair || !qpair->rpool) {
		return;
	}

	/* The scratch belongs to whoever allocated the queue. */
	free(qpair->rpool);
	memset(qpair, 0, sizeof(*qpair));
}

#endif /* __UPCIE_NVME_RUNTIME_RECORD_H */
