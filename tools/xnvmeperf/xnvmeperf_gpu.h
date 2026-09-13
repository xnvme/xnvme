// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __XNVMEPERF_GPU_H
#define __XNVMEPERF_GPU_H

#include <stdint.h>
#include <errno.h>

#include <libxnvme.h>

struct xnvmeperf_gpu;

#if defined(XNVME_BE_UPCIE_CUDA_ENABLED) || defined(XNVME_BE_UPCIE_HIP_ENABLED)

int
xnvmeperf_gpu_set_device(uint32_t gpu_id);

struct xnvmeperf_gpu *
xnvmeperf_gpu_bounce_open(uint32_t gpu_id, uint32_t iosize, uint32_t nslots);

int
xnvmeperf_gpu_bounce_register(struct xnvmeperf_gpu *gpu, uint32_t slot, void *hbuf);

int
xnvmeperf_gpu_bounce_ready(struct xnvmeperf_gpu *gpu, uint32_t slot);

int
xnvmeperf_gpu_bounce_copy(struct xnvmeperf_gpu *gpu, uint32_t slot, void *hbuf);

void
xnvmeperf_gpu_bounce_drain(struct xnvmeperf_gpu *gpu);

void
xnvmeperf_gpu_bounce_close(struct xnvmeperf_gpu *gpu);

int
xnvmeperf_htod_roofline(uint32_t gpu_id, uint32_t iosize, uint32_t nslots, uint32_t seconds,
			double *gbps);

#else
static inline int
xnvmeperf_gpu_set_device(uint32_t XNVME_UNUSED(gpu_id))
{
	return -ENOSYS;
}
static inline struct xnvmeperf_gpu *
xnvmeperf_gpu_bounce_open(uint32_t XNVME_UNUSED(gpu_id), uint32_t XNVME_UNUSED(iosize),
			  uint32_t XNVME_UNUSED(nslots))
{
	errno = ENOSYS;
	return NULL;
}
static inline int
xnvmeperf_gpu_bounce_register(struct xnvmeperf_gpu *XNVME_UNUSED(gpu), uint32_t XNVME_UNUSED(slot),
			      void *XNVME_UNUSED(hbuf))
{
	return -ENOSYS;
}
static inline int
xnvmeperf_gpu_bounce_ready(struct xnvmeperf_gpu *XNVME_UNUSED(gpu), uint32_t XNVME_UNUSED(slot))
{
	return 0;
}
static inline int
xnvmeperf_gpu_bounce_copy(struct xnvmeperf_gpu *XNVME_UNUSED(gpu), uint32_t XNVME_UNUSED(slot),
			  void *XNVME_UNUSED(hbuf))
{
	return -ENOSYS;
}
static inline void
xnvmeperf_gpu_bounce_drain(struct xnvmeperf_gpu *XNVME_UNUSED(gpu))
{
}
static inline void
xnvmeperf_gpu_bounce_close(struct xnvmeperf_gpu *XNVME_UNUSED(gpu))
{
}
static inline int
xnvmeperf_htod_roofline(uint32_t XNVME_UNUSED(gpu_id), uint32_t XNVME_UNUSED(iosize),
			uint32_t XNVME_UNUSED(nslots), uint32_t XNVME_UNUSED(seconds),
			double *XNVME_UNUSED(gbps))
{
	return -ENOSYS;
}
#endif

#endif /* __XNVMEPERF_GPU_H */
