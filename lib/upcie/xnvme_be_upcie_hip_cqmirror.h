// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_HIP_CQMIRROR_H
#define __INTERNAL_XNVME_BE_UPCIE_HIP_CQMIRROR_H
#include <stddef.h>
#include <stdint.h>

#define XNVME_BE_UPCIE_HIP_CQMIRROR_NSLOTS 64

/* The dma-buf export wants a range this size and aligned to it, so a CQ is a region of its own */
#define XNVME_BE_UPCIE_HIP_CQMIRROR_CQ_NBYTES       (2UL << 20)
#define XNVME_BE_UPCIE_HIP_CQMIRROR_WAVES_PER_BLOCK 8

/**
 * The host fills the pointers, then bumps gen to odd; the wavefront acks the gen it stopped on,
 * and acks the odd value before an even gen it never served
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

#ifdef __cplusplus
extern "C" {
#endif
int
xnvme_be_upcie_hip_cqmirror_launch(struct xnvme_be_upcie_hip_cqmirror_table *table,
				   unsigned int nblocks, unsigned int nthreads, void *stream);
#ifdef __cplusplus
}
#endif

#ifndef __HIPCC__
/** Pooled: freeing device memory waits for the device, and the kernel does not finish */
void *
xnvme_be_upcie_hip_cqmirror_cq_alloc(void);

void
xnvme_be_upcie_hip_cqmirror_cq_release(void *cq_gpu);

/**
 * Both queues zeroed, since the wavefront starts at entry zero expecting phase one; the host
 * range stays registered until xnvme_be_upcie_hip_cqmirror_term()
 */
int
xnvme_be_upcie_hip_cqmirror_attach(void *host_base, size_t host_nbytes, const void *cq_gpu,
				   void *cq_host, uint16_t depth);

/** Returns once the wavefront has let go of both queues */
void
xnvme_be_upcie_hip_cqmirror_detach(int slot);

/** Every queue must have been detached */
void
xnvme_be_upcie_hip_cqmirror_term(void);
#endif /* __HIPCC__ */

#endif /* __INTERNAL_XNVME_BE_UPCIE_HIP_CQMIRROR_H */
