// SPDX-License-Identifier: BSD-3-Clause

/**
 * CUDA NVMe Queue Pair Extension
 * ==============================
 *
 * This header extends the functionality defined in the uPCIe NVMe Queue Pair
 * header `upcie/nvme/nvme_qpair.h` with a CUDA compatible queue implementation.
 *
 * Look to `nvme_qpair_cuda_io()` to see the intended flow of IO function calls.
 */

#ifndef __device__
#define __device__
#endif
#ifndef __host__
#define __host__
#endif

struct nvme_qpair_cuda {
	void *sq;             ///< VA-Pointer to DMA-capable memory backing the Submission Queue (SQ)
	void *cq;             ///< VA-Pointer to DMA-capable memory backing the Completion Queue (CQ)
	void *sqdb;           ///< Pointer to Submission Queue Doorbell Register in bar0
	void *cqdb;           ///< Pointer to Completion Queue Doorbell Register in bar0
	uint32_t qid;         ///< The admin: queue-id == 0 ; io: queue-id > 0;
	uint16_t depth;       ///< Length of the queue-pair
	uint16_t tail;        ///< Submission Queue Tail Pointer
	uint16_t head;        ///< Completion Queue Head Pointer
	uint8_t phase;        ///< Expected CQ phase bit; flips each time the CQ head wraps to 0
	uint8_t _rsvd[3];     ///< Padding to align timeout_ms to a 4-byte boundary
	uint32_t timeout_ms;  ///< Command timeout in milliseconds (derived from cap.to)
	uint64_t clocks_per_ms; ///< SM clock cycles per millisecond (set from CU_DEVICE_ATTRIBUTE_CLOCK_RATE)
};

/**
 * Enqueue a command into an NVMe submission queue at a certain index
 *
 * That is, writes it into the submission queue memory. It does **not** increment
 * the tail-pointer or write the tail to the sq-doorbell.
 *
 * @param qp Pointer to the NVMe queue pair to submit command to
 * @param cmd Command to submit
 * @param offset Where to insert the Command in the queue in relation to the tail
 */
static inline __device__ void
nvme_qpair_cuda_enqueue_at_i(struct nvme_qpair_cuda *qp, struct nvme_command *cmd, uint16_t offset)
{
	struct nvme_command *sq = (struct nvme_command *)qp->sq;
	uint16_t index = (qp->tail + offset) % qp->depth;

	// Copy the command to the SQ word-by-word. The destination uses a volatile
	// pointer to bypass the per-SM L1 cache so writes reach system DRAM
	// without waiting for eviction, making them visible to the NVMe DMA engine.
	// The source does not need to be volatile: cmd is a per-thread local copy
	// held in registers.
	uint32_t *src = (uint32_t *)cmd;
	volatile uint32_t *dst = (volatile uint32_t *)&sq[index];

	for (unsigned i = 0; i < sizeof(struct nvme_command) / sizeof(uint32_t); i++) {
		dst[i] = src[i];
	}
}

/**
 * Update the submission queue tail and tail doorbell
 *
 * This function updates the tail and writes it to the MMIO doorbell register
 * for the given queue pair, notifying the controller of new commands.
 *
 * @param qp Pointer to the NVMe queue pair whose SQ doorbell should be updated.
 * @param increment Number of commands to increment the tail by
 */
static inline __device__ void
nvme_qpair_cuda_sq_update(struct nvme_qpair_cuda *qp, uint16_t increment)
{
	qp->tail = (qp->tail + increment) % qp->depth;
#ifdef __CUDACC__
	__threadfence_system(); // flush sq writes to system DRAM (visible to NVMe DMA)
#endif
	*(volatile uint32_t *)qp->sqdb = qp->tail;
}

/**
 * Reaps the completion queue entry at a certain index if ready before timeout
 *
 * This function does **not** increment the head-pointer or write the head to the
 * cq-doorbell.
 *
 * @param qp Pointer to the NVMe queue pair to reap completion from
 * @param timeout_ms Timeout in milliseconds
 * @param cpl Completion when one is reaped, NULL if timeout
 * @param offset Which completion to reap in relation to the head
 *
 * @return 0 on success, -EAGAIN on timeout
 */
static inline __device__ int
nvme_qpair_cuda_reap_at_i(struct nvme_qpair_cuda *qp, int timeout_ms, struct nvme_completion *cpl, uint16_t offset)
{
#ifndef __CUDACC__
	(void)qp; (void)timeout_ms; (void)cpl; (void)offset;
#else
	volatile struct nvme_completion *cq = (volatile struct nvme_completion *)qp->cq;
	uint16_t index = (qp->head + offset) % qp->depth;
	volatile struct nvme_completion *cqe;
	// If this thread's entry is past the CQ wrap point, the controller will
	// have already flipped its phase tag for that entry.
	uint8_t expected_phase = ((uint16_t)(qp->head + offset) >= qp->depth) ? (qp->phase ^ 1) : qp->phase;

	int64_t deadline = (int64_t)clock64() + (int64_t)timeout_ms * (int64_t)qp->clocks_per_ms;

	do {
		cqe = &cq[index];

		if ((cqe->cid < 0xFFFF) && ((cqe->status & 0x1) == expected_phase)) {
			*cpl = *(struct nvme_completion *)cqe;
			return 0;
		}
	} while ((int64_t)clock64() < deadline);
#endif /* __CUDACC__ */

	return -EAGAIN;
}

/**
 * Update the completion queue head and head doorbell
 *
 * This function updates the head and writes it to the MMIO doorbell register
 * for the given queue pair, notifying the controller that all completions have
 * been reaped.
 *
 * @param qp Pointer to the NVMe queue pair whose CQ doorbell should be updated.
 * @param increment Number of commands to increment the head by
 */
static inline __device__ void
nvme_qpair_cuda_cq_update(struct nvme_qpair_cuda *qp, uint16_t increment)
{
	uint16_t new_head = qp->head + increment;

	if (new_head >= qp->depth) {
		new_head -= qp->depth;
		qp->phase ^= 1;
	}
	qp->head = new_head;
	*(volatile uint32_t *)qp->cqdb = qp->head;
}

/**
 * Submit IO and reap completions
 *
 * The intention is to use this in a CUDA kernel where the threadblock size is
 * equal to the queue depth and the number of threadblocks is equal to the number
 * of queues.
 *
 * The function works as follows:
 *   1) Each active thread (tid < batch_size) enqueues its command with
 *      `nvme_qpair_cuda_enqueue_at_i()` where offset == thread index
 *   2) A threadblock barrier ensures all commands are written before continuing
 *   3) **Only** the first thread in the block calls
 *      `nvme_qpair_cuda_sq_update()` with increment == batch_size
 *   4) Each active thread reaps a completion with `nvme_qpair_cuda_reap_at_i()`
 *      where offset == thread index
 *   5) A threadblock barrier ensures all completions are reaped before continuing
 *   6) **Only** the first thread in the block calls `nvme_qpair_cuda_cq_update()`
 *      with increment == batch_size
 *
 * Inactive threads (tid >= batch_size) participate in the barriers but do not
 * submit or reap commands and return 0. This allows partial rounds where fewer
 * than blockDim.x IOs are needed without breaking the collective barrier contract.
 *
 * The barriers in steps 2 and 5 are required for correct operation when the
 * threadblock spans multiple warps (i.e. batch_size > 32). Without them, thread 0
 * may ring the SQ doorbell before other warps have written their commands, or
 * advance the CQ head and flip the phase before other warps have reaped.
 *
 * @param qp         Pointer to the NVMe queue pair to submit command to
 * @param cmd        Command to submit (only read for tid < batch_size)
 * @param tid        The id of the thread calling the function (threadIdx.x)
 * @param batch_size Number of IOs to submit; threads with tid >= batch_size
 *                   participate in barriers but do not submit or reap
 *
 * @return 0 on success. Negative values indicate errno-style errors, positive values are NVMe status codes.
 */
static inline __device__ int
nvme_qpair_cuda_io(struct nvme_qpair_cuda *qp, struct nvme_command *cmd, size_t tid,
		   size_t batch_size)
{
	struct nvme_completion cpl = {0};
	int err = 0;

	if (tid < batch_size) {
		cmd->cid = tid;
		nvme_qpair_cuda_enqueue_at_i(qp, cmd, tid);
	}

#ifdef __CUDACC__
	__syncthreads();
#endif

	if (tid == 0 && batch_size) {
		nvme_qpair_cuda_sq_update(qp, batch_size);
	}

	if (tid < batch_size) {
		err = nvme_qpair_cuda_reap_at_i(qp, qp->timeout_ms, &cpl, tid);
	}

#ifdef __CUDACC__
	__syncthreads();
#endif

	if (tid == 0 && batch_size) {
		nvme_qpair_cuda_cq_update(qp, batch_size);
	}

	if (err) {
		return err;
	}

	return (cpl.status & 0x1FE) >> 1;
}
