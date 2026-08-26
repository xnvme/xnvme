// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * NVMe Queue Pair Abstraction
 * ===========================
 *
 * This header defines a minimal software abstraction for managing NVMe queue pairs (SQ/CQ) in a
 * user-space NVMe driver context. It provides basic functionality for queue setup, submission,
 * completion handling, and doorbell notification.
 *
 * A queue pair is represented by 'struct nvme_qpair', which includes memory-mapped pointers to
 * the submission and completion queues, doorbell registers, and associated tracking state (head,
 * tail, phase).
 *
 * Key functions include:
 *
 * nvme_qpair_reap_cpl():    Polls the CQ for a completion, updates head/phase, and rings the
 *                           CQ doorbell.
 * nvme_qpair_sqdb_update(): Notifies the controller by ringing the SQ doorbell.
 * nvme_qpair_enqueue():     Writes the given command into the SQ
 * nvme_qpair_submit_sync(): Submits a command and waits synchronously for its completion.
 *
 * The memory behind a queue pair comes from a dmamem_heap; see nvme_qpair_dmamem_init() and
 * nvme_qpair_dmamem_term() in nvme_controller_dmamem_vfio.h.
 *
 * See also: nvme_qid.h for queue ID (qid) management.
 *
 * @file nvme_qpair.h
 * @version 0.10.0
 */

struct nvme_qpair {
	void *sq;       ///< VA-Pointer to DMA-capable memory backing the Submission Queue (SQ)
	void *cq;       ///< VA-Pointer to DMA-capable memory backing the Completion Queue (CQ)
	void *sqdb;     ///< Pointer to Submission Queue Doorbell Register in bar0
	void *cqdb;     ///< Pointer to Completion Queue Doorbell Register in bar0
	uint32_t qid;   ///< The admin: queue-id == 0 ; io: queue-id > 0;
	uint16_t depth; ///< Length of the queue-pair
	uint16_t tail;  ///< Submissin Queue Tail Pointer
	uint16_t tail_last_written; ///< Last tail-value written to DB-reg. init to UINT16_MAX
	uint16_t head;              ///< Completion Queue Head Pointer
	uint8_t phase;
	uint8_t _rsdv[3];
	struct nvme_request_pool *rpool; ///< Command Identifier tracking and user-callback
};

/**
 * Reaps at most a single completion and informs the controller via qp->cqdb
 *
 * @param qp A queue-pair as represented by 'struct nvme_qp'
 * @param cpl Completion when one is reaped
 * @param timeout_ms Timeout in milliseconds
 *
 * @return Pointer to a valid completion, or NULL on timeout.
 */
static inline int
nvme_qpair_reap_cpl(struct nvme_qpair *qp, int timeout_ms, struct nvme_completion *cpl)
{
	struct nvme_completion *cq = qp->cq;

	for (int i = 0; i < timeout_ms; ++i) {
		struct nvme_completion *cqe = &cq[qp->head];

		if ((cqe->cid < 0xFFFF) && ((cqe->status & 0x1) == qp->phase)) {
			*cpl = *cqe;

			// Advance CQ head and toggle phase if wrapping
			qp->head++;
			if (qp->head == qp->depth) {
				qp->head = 0;
				qp->phase ^= 1;
			}

			mmio_write32(qp->cqdb, 0, qp->head);
			return 0;
		}

		usleep(1000);
	}

	return -EAGAIN;
}

/**
 * Update the submission queue tail doorbell if needed.
 *
 * This function writes the current SQ tail index to the MMIO doorbell register
 * for the given queue pair, notifying the controller of new commands.
 * To avoid redundant MMIO writes, the function checks whether the tail value
 * has changed since the last call. The last written value is tracked in
 * nvme_qpair->tail_last_written.
 *
 * @param qp Pointer to the NVMe queue pair whose SQ doorbell should be updated.
 */
static inline void
nvme_qpair_sqdb_update(struct nvme_qpair *qp)
{
	if (qp->tail == qp->tail_last_written) {
		return;
	}

	mmio_write32(qp->sqdb, 0, qp->tail);
	qp->tail_last_written = qp->tail;
}

/**
 * Enqueue a command into an NVMe submission queue of a `nvme_qpair`
 *
 * That is, writes it into the submission queue memory and increments the tail-pointer, it does
 * **not** write the tail to the sq-doorbell.
 *
 * @param qp The queue-pair
 * @param cmd Command to submit
 * @param user Optional opaque pointer returned on completion
 *
 * @return On success 0 is returned. On error then negative errno is set to indicate the error.
 */
static inline int
nvme_qpair_enqueue(struct nvme_qpair *qp, struct nvme_command *cmd)
{
	volatile struct nvme_command *sq = qp->sq;

	sq[qp->tail] = *cmd;

	qp->tail = (qp->tail + 1) % qp->depth;

	return 0;
}

/**
 * Submits a command on the given qpair, waits for completion, and populates `cpl`.
 *
 * This is intended for synchronous I/O or Admin commands where the caller manages the payload
 * and sets up PRP1/PRP2 manually. The function does not modify or validate the PRP fields.
 *
 * @param qp         Pointer to the submission queue pair.
 * @param cmd        Pointer to the command to submit; `cid` will be assigned.
 * @param timeout_ms Timeout in milliseconds to wait for command completion.
 * @param cpl        Pointer to a completion structure to receive the result.
 *
 * @return On success 0 is returned. On error, negative errno is returned to indicate the error.
 */
static inline int
nvme_qpair_submit_sync(struct nvme_qpair *qp, struct nvme_command *cmd, int timeout_ms,
		       struct nvme_completion *cpl)
{
	struct nvme_request *req;
	int err;

	req = nvme_request_alloc(qp->rpool);
	if (!req) {
		UPCIE_DEBUG("FAILED: nvme_request_alloc(); errno(%d)", errno);
		return -errno;
	}
	cmd->cid = req->cid;

	err = nvme_qpair_enqueue(qp, cmd);
	if (err) {
		return -err;
	}

	nvme_qpair_sqdb_update(qp);

	err = nvme_qpair_reap_cpl(qp, timeout_ms, cpl);
	if (err) {
		return -err;
	}

	nvme_request_free(qp->rpool, req->cid);

	if (cpl->status & 0x1FE) {
		err = -EIO;
	}

	return err;
}
