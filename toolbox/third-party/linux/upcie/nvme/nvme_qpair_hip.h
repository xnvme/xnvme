// SPDX-License-Identifier: BSD-3-Clause

/**
 * HIP NVMe Queue Pair Extension
 * ==============================
 *
 * This header extends the functionality defined in the uPCIe NVMe Queue Pair
 * header `upcie/nvme/nvme_qpair.h` with a HIP compatible queue implementation.
 *
 * Look to `nvme_qpair_hip_io()` to see the intended flow of IO function calls.
 *
 * @file nvme_qpair_hip.h
 * @version 0.5.0
 */

#ifndef __device__
#define __device__
#endif
#ifndef __host__
#define __host__
#endif

// Upper bound on the GPU-side completion wait, kept well under the amdgpu ring
// lockup watchdog so a missed completion fails fast instead of resetting the GPU.
#ifndef NVME_QPAIR_HIP_REAP_CAP_MS
#define NVME_QPAIR_HIP_REAP_CAP_MS 1000
#endif

struct nvme_qpair_hip {
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
	uint64_t clocks_per_ms; ///< wall_clock64 cycles per millisecond (set from hipDeviceAttributeWallClockRate)
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
nvme_qpair_hip_enqueue_at_i(struct nvme_qpair_hip *qp, struct nvme_command *cmd, uint16_t offset)
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
 * Ring a doorbell with RDNA-aware fencing
 *
 * The default __threadfence_system() + atomic-release store leaves the doorbell
 * lingering in the GPU write path on consumer RDNA: it is not made visible to a
 * peer (or to the CPU bridge poller) until near kernel exit, so submission
 * happens too late for the reap. Match rocm-xio's ringDoorbellFenced: drain
 * outstanding memory ops, store bypassing all caches (glc/slc/dlc), and
 * invalidate GL0/GL1 so the write reaches the system coherence point promptly.
 * gfx12 uses the restructured wait/scope encoding; other targets fall back to
 * the fence+store. The asm is only emitted in the device compilation pass.
 */
static inline __device__ void
nvme_qpair_hip_ring_doorbell(volatile uint32_t *doorbell, uint32_t value)
{
#ifdef __HIP_DEVICE_COMPILE__
// Guard on the GFX family macros (__GFX10__/__GFX11__/__GFX12__), which are
// defined for both a specific arch (gfx1101) and a generic one (gfx11-generic).
// The specific __gfxNNNN__ macros are NOT defined for a generic --offload-arch,
// which silently drops the fence to the weak path.
#if defined(__GFX12__)
	asm volatile("s_wait_kmcnt 0x0 \n"
		     "s_wait_loadcnt 0x0 \n"
		     "s_wait_storecnt 0x0 \n"
		     "global_store_b32 %0, %1, off scope:SCOPE_SYS \n"
		     "s_wait_kmcnt 0x0 \n"
		     "s_wait_loadcnt 0x0 \n"
		     "s_wait_storecnt 0x0 \n" ::"v"(doorbell), "v"(value)
		     : "memory");
	__threadfence_system();
#elif defined(__GFX10__) || defined(__GFX11__)
	asm volatile("s_waitcnt lgkmcnt(0) vmcnt(0) \n"
		     "s_waitcnt_vscnt null, 0x0 \n"
		     "global_store_dword %0, %1, off glc slc dlc \n"
		     "s_waitcnt lgkmcnt(0) vmcnt(0) \n"
		     "s_waitcnt_vscnt null, 0x0 \n"
		     "buffer_gl1_inv \n"
		     "buffer_gl0_inv \n" ::"v"(doorbell), "v"(value)
		     : "memory");
	__threadfence_system();
#else
	__threadfence_system();
	*doorbell = value;
	__threadfence_system();
#endif
#else
	*doorbell = value;
#endif
}

/**
 * Invalidate the per-CU read caches before a poll load
 *
 * On consumer RDNA a polling load can keep hitting a stale GL0/GL1 line and
 * never observe a write made by the CPU bridge mirror (or a peer), so the reap
 * misses completions that are already present. Invalidate GL0/GL1 so the next
 * load re-fetches from L2/system, where the mirror write is visible.
 */
static inline __device__ void
nvme_qpair_hip_invalidate_read(void)
{
#ifdef __HIP_DEVICE_COMPILE__
#if defined(__GFX10__) || defined(__GFX11__)
	asm volatile("buffer_gl1_inv \n"
		     "buffer_gl0_inv \n" ::: "memory");
#else
	__threadfence_system();
#endif
#endif
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
nvme_qpair_hip_sq_update(struct nvme_qpair_hip *qp, uint16_t increment)
{
	qp->tail = (qp->tail + increment) % qp->depth;
#ifdef __HIPCC__
	__threadfence_system(); // publish all lanes' SQEs to system before the doorbell
#endif
	nvme_qpair_hip_ring_doorbell((volatile uint32_t *)qp->sqdb, qp->tail);
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
nvme_qpair_hip_reap_at_i(struct nvme_qpair_hip *qp, int timeout_ms, struct nvme_completion *cpl, uint16_t offset)
{
#ifndef __HIPCC__
	(void)qp; (void)timeout_ms; (void)cpl; (void)offset;
#else
	volatile struct nvme_completion *cq = (volatile struct nvme_completion *)qp->cq;
	uint16_t index = (qp->head + offset) % qp->depth;
	volatile struct nvme_completion *cqe;
	// If this thread's entry is past the CQ wrap point, the controller will
	// have already flipped its phase tag for that entry.
	uint8_t expected_phase = ((uint16_t)(qp->head + offset) >= qp->depth) ? (qp->phase ^ 1) : qp->phase;

	// Cap the GPU-side wait far below the amdgpu ring lockup watchdog (~10 s). A
	// real completion lands in microseconds, so a long spin only ever means a
	// miss; letting it run to the controller timeout (tens of seconds) would trip
	// the watchdog and reset the whole GPU, cascading into later runs. Bound it.
	if (timeout_ms > NVME_QPAIR_HIP_REAP_CAP_MS) {
		timeout_ms = NVME_QPAIR_HIP_REAP_CAP_MS;
	}

	// wall_clock64() is a fixed-frequency counter (hipDeviceAttributeWallClockRate);
	// clock64() on some GPUs (e.g. gfx1101) advances at a wholly different rate than
	// hipDeviceAttributeClockRate, which would miscalibrate this deadline by orders
	// of magnitude and let a missing CQE spin past the GPU watchdog into a reset.
	int64_t deadline = (int64_t)wall_clock64() + (int64_t)timeout_ms * (int64_t)qp->clocks_per_ms;

	do {
		// Invalidate GL0/GL1 so the load re-fetches the bridge-mirrored CQ from
		// L2/system rather than a stale per-CU line. Read the last CQE dword (cid
		// low 16, status high 16); NVMe writes the phase bit last, so a matching
		// phase means the whole entry is valid.
		nvme_qpair_hip_invalidate_read();

		cqe = &cq[index];
		uint32_t cid_status = *(const volatile uint32_t *)((const void *)&cqe->cid);

		if (((cid_status & 0xFFFF) < 0xFFFF) && (((cid_status >> 16) & 0x1) == expected_phase)) {
			cpl->cid = (uint16_t)(cid_status & 0xFFFF);
			cpl->status = (uint16_t)(cid_status >> 16);
			return 0;
		}
	} while ((int64_t)wall_clock64() < deadline);
#endif /* __HIPCC__ */

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
nvme_qpair_hip_cq_update(struct nvme_qpair_hip *qp, uint16_t increment)
{
	uint16_t new_head = qp->head + increment;

	if (new_head >= qp->depth) {
		new_head -= qp->depth;
		qp->phase ^= 1;
	}
	qp->head = new_head;
	nvme_qpair_hip_ring_doorbell((volatile uint32_t *)qp->cqdb, qp->head);
}

/**
 * Submit IO and reap completions
 *
 * The intention is to use this in a HIP kernel where the threadblock size is
 * equal to the queue depth and the number of threadblocks is equal to the number
 * of queues.
 *
 * The function works as follows:
 *   1) Each active thread (tid < batch_size) enqueues its command with
 *      `nvme_qpair_hip_enqueue_at_i()` where offset == thread index
 *   2) A threadblock barrier ensures all commands are written before continuing
 *   3) **Only** the first thread in the block calls
 *      `nvme_qpair_hip_sq_update()` with increment == batch_size
 *   4) Each active thread reaps a completion with `nvme_qpair_hip_reap_at_i()`
 *      where offset == thread index
 *   5) A threadblock barrier ensures all completions are reaped before continuing
 *   6) **Only** the first thread in the block calls `nvme_qpair_hip_cq_update()`
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
nvme_qpair_hip_io(struct nvme_qpair_hip *qp, struct nvme_command *cmd, size_t tid,
		   size_t batch_size)
{
	struct nvme_completion cpl = {0};
	int err = 0;

	if (tid < batch_size) {
		cmd->cid = tid;
		nvme_qpair_hip_enqueue_at_i(qp, cmd, tid);
	}

#ifdef __HIPCC__
	__syncthreads();
#endif

	if (tid == 0 && batch_size) {
		nvme_qpair_hip_sq_update(qp, batch_size);
	}

	if (tid < batch_size) {
		err = nvme_qpair_hip_reap_at_i(qp, qp->timeout_ms, &cpl, tid);
	}

#ifdef __HIPCC__
	__syncthreads();
#endif

	if (tid == 0 && batch_size) {
		nvme_qpair_hip_cq_update(qp, batch_size);
	}

	if (err) {
		return err;
	}

	return (cpl.status & 0x1FE) >> 1;
}
