// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __XNVMEPERF_H
#define __XNVMEPERF_H

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#include <libxnvme.h>

enum iopattern {
	IOPATTERN_READ      = 1,
	IOPATTERN_WRITE     = 2,
	IOPATTERN_RANDREAD  = 3,
	IOPATTERN_RANDWRITE = 4,
	IOPATTERN_VERIFY    = 5, ///< Used for verify subcommand
};

struct xnvmeperf_args {
	int ndevs;
	const char **dev_uris;
	uint16_t ncpus;
	uint16_t *cpus;
	uint32_t qdepth;
	uint32_t iosize;
	uint32_t time;
	uint32_t count;
	uint32_t nqueues;
	uint32_t nbatches; ///< GPU-issued: batches the depth is split into, see cuda-run
	double report_freq;
	enum iopattern pattern;
	int queue_opts;      ///< Passed to xnvme_queue_init() or xnvme_cuda_queue_create()
	int buf_host_bounce; ///< Read into host memory and copy each payload to the GPU
	struct xnvme_opts opts;
};

int
fill_pattern(void *buf, size_t nbytes, uint64_t slba, uint16_t nlb);

void
print_intermediate_header(void);

void
print_intermediate_result(double elapsed, double interval, uint64_t completed, uint32_t iosize);

#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
int
xnvmeperf_cuda_run_io(struct xnvme_dev **devs, const struct xnvmeperf_args *args,
		      uint64_t *completed_per_dev, uint64_t *failed_per_dev, float *elapsed_ms);
int
xnvmeperf_cuda_verify_io(struct xnvme_dev **devs, const struct xnvmeperf_args *args);
#else
static inline int
xnvmeperf_cuda_run_io(struct xnvme_dev **XNVME_UNUSED(devs),
		      const struct xnvmeperf_args *XNVME_UNUSED(args),
		      uint64_t *XNVME_UNUSED(completed_per_dev),
		      uint64_t *XNVME_UNUSED(failed_per_dev), float *XNVME_UNUSED(elapsed_ms))
{
	return -ENOSYS;
}

static inline int
xnvmeperf_cuda_verify_io(struct xnvme_dev **XNVME_UNUSED(devs),
			 const struct xnvmeperf_args *XNVME_UNUSED(args))
{
	return -ENOSYS;
}
#endif

/*
 * Host-bounce staging (``run --buf-host-bounce``) and the host-to-device copy
 * roofline (``htod-roofline``). The NVMe I/O stays on a host backend; these
 * move the payload on to the GPU with the vendor runtime's async copy. The
 * generic loop in xnvmeperf.c owns the per-slot host buffers and the free-list;
 * the vendor file (CUDA .cu, HIP .hip) owns the device buffer, the copy stream
 * and the per-slot completion events. One opaque handle per queue.
 */
struct xnvmeperf_gpu;

#if defined(XNVME_BE_UPCIE_CUDA_ENABLED) || defined(XNVME_BE_UPCIE_HIP_ENABLED)

/** Make @p gpu_id the calling thread's device before any copy on it. */
int
xnvmeperf_gpu_set_device(uint32_t gpu_id);

/** A device buffer of @p nslots * @p iosize, a copy stream and @p nslots events. */
struct xnvmeperf_gpu *
xnvmeperf_gpu_bounce_open(uint32_t gpu_id, uint32_t iosize, uint32_t nslots);

/** Page-lock a host buffer so its copy runs at link speed; kept for teardown. */
int
xnvmeperf_gpu_bounce_register(struct xnvmeperf_gpu *gpu, uint32_t slot, void *hbuf);

/** 1 when @p slot's previous copy has finished (or it never copied), else 0. */
int
xnvmeperf_gpu_bounce_ready(struct xnvmeperf_gpu *gpu, uint32_t slot);

/** Enqueue the host-to-device copy of @p slot and record its completion event. */
int
xnvmeperf_gpu_bounce_copy(struct xnvmeperf_gpu *gpu, uint32_t slot, void *hbuf);

/** Wait for every enqueued copy to finish. */
void
xnvmeperf_gpu_bounce_drain(struct xnvmeperf_gpu *gpu);

/** Unregister the host buffers and free the device buffer, stream and events. */
void
xnvmeperf_gpu_bounce_close(struct xnvmeperf_gpu *gpu);

/**
 * Host-to-device copy roofline: keep @p nslots pinned copies of @p iosize in
 * flight for @p seconds and report the delivered GB/s in @p gbps.
 */
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

#endif /* __XNVMEPERF_H */
