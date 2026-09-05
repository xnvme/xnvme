// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Completion queues in GPU memory, mirrored into host memory, for `upcie-hip`
 *
 * The HIP form of xnvme_be_upcie_cuda_cqmirror.h: one kernel resident on the
 * GPU gives every queue with its CQ in device memory a wavefront that copies
 * completions into the host CQ the CPU polls. See that header for why.
 *
 * The kernel is the only HIP device code in the library. It is compiled by
 * hipcc into a code object on its own, embedded, and loaded through the
 * module API, so the library itself stays a gcc build against the runtime.
 * The table below is the contract between the two; it is included by both.
 */
#ifndef __INTERNAL_XNVME_BE_UPCIE_HIP_CQMIRROR_H
#define __INTERNAL_XNVME_BE_UPCIE_HIP_CQMIRROR_H
#include <stddef.h>
#include <stdint.h>

#define XNVME_BE_UPCIE_HIP_CQMIRROR_NSLOTS          64
#define XNVME_BE_UPCIE_HIP_CQMIRROR_WAVES_PER_BLOCK 8

/**
 * A queue as its wavefront sees it; 64 bytes so a slot is one line
 *
 * The host fills the pointers and then bumps gen to odd; the wavefront works
 * until it sees gen change, then reports the value it left on in ack. The
 * addresses are the GPU's, and are integers so that the header is plain C.
 */
struct xnvme_be_upcie_hip_cqmirror_slot {
	uint64_t cq_gpu;
	uint64_t cq_host;
	uint32_t depth;
	uint32_t gen;
	uint32_t ack;
	uint32_t _rsvd[7];
};

struct xnvme_be_upcie_hip_cqmirror_table {
	int32_t stop;
	int32_t _rsvd[15];
	struct xnvme_be_upcie_hip_cqmirror_slot slots[XNVME_BE_UPCIE_HIP_CQMIRROR_NSLOTS];
};

#ifndef __HIPCC__
/**
 * Device memory for a CQ the controller writes and the kernel reads
 *
 * Uncached on the GPU, since the controller writes it behind the GPU's
 * caches, 2 MiB and aligned so that it exports as a dma-buf on its own. It is
 * kept for the next queue of the process rather than freed, since freeing
 * device memory waits for the device, and the kernel does not finish.
 *
 * @return The memory, zeroed, or NULL with errno set
 */
void *
xnvme_be_upcie_hip_cqmirror_cq_alloc(void);

/**
 * Hand the memory of a CQ back for the next queue
 */
void
xnvme_be_upcie_hip_cqmirror_cq_release(void *cq_gpu);

/**
 * Have a wavefront keep cq_host a copy of cq_gpu
 *
 * cq_host must lie in host memory the GPU can be given; the range it is in
 * is registered with the runtime the first time it is seen and stays so until
 * xnvme_be_upcie_hip_cqmirror_term(). Both queues must be zeroed, since the
 * wavefront starts at entry zero expecting phase one.
 *
 * @param host_base Start of the host region cq_host lies in
 * @param host_nbytes Size of that region
 * @param cq_gpu The CQ the controller completes into
 * @param cq_host The CQ the CPU reads
 * @param depth Entries in the CQ
 *
 * @return The slot the queue was given, or negative errno
 */
int
xnvme_be_upcie_hip_cqmirror_attach(void *host_base, size_t host_nbytes, const void *cq_gpu,
				   void *cq_host, uint16_t depth);

/**
 * Take a queue back from its wavefront
 *
 * Returns once the wavefront has let go of both queues, so they can be
 * deleted and their memory reused.
 *
 * @param slot What xnvme_be_upcie_hip_cqmirror_attach() returned
 */
void
xnvme_be_upcie_hip_cqmirror_detach(int slot);

/**
 * Stop the kernel if it is up, drop the host registrations and the CQ memory
 *
 * For the runtime's teardown; every queue must have been detached.
 */
void
xnvme_be_upcie_hip_cqmirror_term(void);
#endif /* __HIPCC__ */

#endif /* __INTERNAL_XNVME_BE_UPCIE_HIP_CQMIRROR_H */
