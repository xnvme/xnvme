// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_HIP_GPUINIT_H
#define __INTERNAL_XNVME_BE_UPCIE_HIP_GPUINIT_H
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <xnvme_be.h>

#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_hip.h>
#include <upcie/upcie_hip_gpuinit.h>

/**
 * A GPU-initiated I/O queue.
 *
 * Holds the device-resident qpair that the HIP kernel drives together with its
 * host-side doorbell bridge. The qpair lives in GPU device memory so the kernel
 * can access it; the bridge is host memory (it owns a poller thread and host
 * shadow allocations), so the two cannot share a single allocation.
 */
struct xnvme_hip_gpuinit_queue {
	struct nvme_qpair_hip *qpair;     ///< device-memory qpair (hipMalloc'd)
	struct nvme_hip_db_bridge bridge; ///< host-memory doorbell/CQ bridge
};

int
xnvme_hip_gpuinit_queue_create(struct xnvme_dev *dev, uint16_t depth,
			       struct xnvme_hip_gpuinit_queue **queue);

void
xnvme_hip_gpuinit_queue_destroy(struct xnvme_dev *dev, struct xnvme_hip_gpuinit_queue *queue);

extern struct xnvme_be_async g_xnvme_be_upcie_hip_gpuinit_async;

#endif /* XNVME_BE_UPCIE_HIP_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_HIP_GPUINIT_H */
