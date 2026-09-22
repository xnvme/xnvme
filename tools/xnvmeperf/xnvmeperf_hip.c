// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause
//
// The ROCm twin of the host-bounce path and the copy roofline in
// xnvmeperf_cuda.cu: the same seven entry points, over the HIP runtime. Plain C
// against libamdhip64, as the library's HIP backend is; there are no kernels
// here, only host-side copies, streams and events, so hipcc is not involved.
#include <hip/hip_runtime.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libxnvme.h>
#include "xnvmeperf.h"

struct xnvmeperf_gpu {
	uint32_t iosize;
	uint32_t nslots;
	uint8_t *dev;       ///< device buffer, nslots * iosize
	hipStream_t stream; ///< one stream carries every slot's copy in order
	hipEvent_t *events; ///< per-slot completion, so a slot is reused only once drained
	void **hbufs;       ///< host buffers page-locked here, unregistered at close
	uint8_t *pending;   ///< 1 while a slot's copy is enqueued and not yet observed done
};

int
xnvmeperf_gpu_set_device(uint32_t gpu_id)
{
	hipError_t herr = hipSetDevice((int)gpu_id);

	if (herr != hipSuccess) {
		fprintf(stderr, "Failed: hipSetDevice(%u): %s\n", gpu_id, hipGetErrorString(herr));
		return -EIO;
	}
	return 0;
}

struct xnvmeperf_gpu *
xnvmeperf_gpu_bounce_open(uint32_t gpu_id, uint32_t iosize, uint32_t nslots)
{
	struct xnvmeperf_gpu *gpu;
	hipError_t herr;

	if (xnvmeperf_gpu_set_device(gpu_id)) {
		return NULL;
	}
	gpu = calloc(1, sizeof(*gpu));
	if (!gpu) {
		errno = ENOMEM;
		return NULL;
	}
	gpu->iosize = iosize;
	gpu->nslots = nslots;
	gpu->events = calloc(nslots, sizeof(*gpu->events));
	gpu->hbufs = calloc(nslots, sizeof(*gpu->hbufs));
	gpu->pending = calloc(nslots, sizeof(*gpu->pending));
	if (!gpu->events || !gpu->hbufs || !gpu->pending) {
		goto failed;
	}

	herr = hipMalloc((void **)&gpu->dev, (size_t)nslots * iosize);
	if (herr != hipSuccess) {
		fprintf(stderr, "Failed: hipMalloc(%zu): %s\n", (size_t)nslots * iosize,
			hipGetErrorString(herr));
		goto failed;
	}
	herr = hipStreamCreateWithFlags(&gpu->stream, hipStreamNonBlocking);
	if (herr != hipSuccess) {
		fprintf(stderr, "Failed: hipStreamCreate(): %s\n", hipGetErrorString(herr));
		goto failed;
	}
	for (uint32_t i = 0; i < nslots; i++) {
		herr = hipEventCreateWithFlags(&gpu->events[i], hipEventDisableTiming);
		if (herr != hipSuccess) {
			fprintf(stderr, "Failed: hipEventCreate(): %s\n", hipGetErrorString(herr));
			goto failed;
		}
	}
	return gpu;

failed:
	xnvmeperf_gpu_bounce_close(gpu);
	errno = ENOMEM;
	return NULL;
}

int
xnvmeperf_gpu_bounce_register(struct xnvmeperf_gpu *gpu, uint32_t slot, void *hbuf)
{
	hipError_t herr;

	gpu->hbufs[slot] = hbuf;
	herr = hipHostRegister(hbuf, gpu->iosize, hipHostRegisterDefault);
	if (herr == hipErrorHostMemoryAlreadyRegistered) {
		hipGetLastError();
		return 0;
	}
	if (herr != hipSuccess) {
		/* Leave it unregistered: the copy still runs, just at the pageable
		 * rate, which the report then reflects rather than hides. */
		gpu->hbufs[slot] = NULL;
		fprintf(stderr, "Warning: hipHostRegister(%p): %s; copy will be pageable\n", hbuf,
			hipGetErrorString(herr));
		hipGetLastError();
		return -EIO;
	}
	return 0;
}

int
xnvmeperf_gpu_bounce_ready(struct xnvmeperf_gpu *gpu, uint32_t slot)
{
	if (!gpu->pending[slot]) {
		return 1;
	}
	if (hipEventQuery(gpu->events[slot]) == hipSuccess) {
		gpu->pending[slot] = 0;
		return 1;
	}
	return 0;
}

int
xnvmeperf_gpu_bounce_copy(struct xnvmeperf_gpu *gpu, uint32_t slot, void *hbuf)
{
	hipError_t herr;

	herr = hipMemcpyAsync(gpu->dev + (size_t)slot * gpu->iosize, hbuf, gpu->iosize,
			      hipMemcpyHostToDevice, gpu->stream);
	if (herr != hipSuccess) {
		fprintf(stderr, "Failed: hipMemcpyAsync(): %s\n", hipGetErrorString(herr));
		return -EIO;
	}
	hipEventRecord(gpu->events[slot], gpu->stream);
	gpu->pending[slot] = 1;
	return 0;
}

void
xnvmeperf_gpu_bounce_drain(struct xnvmeperf_gpu *gpu)
{
	hipStreamSynchronize(gpu->stream);
	memset(gpu->pending, 0, gpu->nslots);
}

void
xnvmeperf_gpu_bounce_close(struct xnvmeperf_gpu *gpu)
{
	if (!gpu) {
		return;
	}
	if (gpu->stream) {
		hipStreamSynchronize(gpu->stream);
	}
	for (uint32_t i = 0; i < gpu->nslots; i++) {
		if (gpu->hbufs && gpu->hbufs[i]) {
			hipHostUnregister(gpu->hbufs[i]);
		}
		if (gpu->events && gpu->events[i]) {
			hipEventDestroy(gpu->events[i]);
		}
	}
	if (gpu->dev) {
		hipFree(gpu->dev);
	}
	if (gpu->stream) {
		hipStreamDestroy(gpu->stream);
	}
	hipGetLastError();
	free(gpu->events);
	free(gpu->hbufs);
	free(gpu->pending);
	free(gpu);
}

static double
_now_s(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int
xnvmeperf_htod_roofline(uint32_t gpu_id, uint32_t iosize, uint32_t nslots, uint32_t seconds,
			double *gbps)
{
	uint8_t *hbuf = NULL, *dev = NULL;
	hipStream_t stream = NULL;
	hipEvent_t *events = NULL;
	uint8_t *pending = NULL;
	uint64_t done = 0;
	double t0, elapsed;
	hipError_t herr;
	int err = 0;

	if (xnvmeperf_gpu_set_device(gpu_id)) {
		return -EIO;
	}
	events = calloc(nslots, sizeof(*events));
	pending = calloc(nslots, sizeof(*pending));
	if (!events || !pending) {
		err = -ENOMEM;
		goto out;
	}
	if ((herr = hipHostMalloc((void **)&hbuf, iosize, hipHostMallocDefault)) != hipSuccess ||
	    (herr = hipMalloc((void **)&dev, (size_t)nslots * iosize)) != hipSuccess ||
	    (herr = hipStreamCreateWithFlags(&stream, hipStreamNonBlocking)) != hipSuccess) {
		fprintf(stderr, "Failed: roofline setup: %s\n", hipGetErrorString(herr));
		err = -EIO;
		goto out;
	}
	for (uint32_t i = 0; i < nslots; i++) {
		if ((herr = hipEventCreateWithFlags(&events[i], hipEventDisableTiming)) !=
		    hipSuccess) {
			fprintf(stderr, "Failed: hipEventCreate(): %s\n", hipGetErrorString(herr));
			err = -EIO;
			goto out;
		}
	}

	t0 = _now_s();
	while (_now_s() - t0 < (double)seconds) {
		for (uint32_t s = 0; s < nslots; s++) {
			if (pending[s] && hipEventQuery(events[s]) != hipSuccess) {
				continue;
			}
			hipMemcpyAsync(dev + (size_t)s * iosize, hbuf, iosize,
				       hipMemcpyHostToDevice, stream);
			hipEventRecord(events[s], stream);
			pending[s] = 1;
			done++;
		}
	}
	hipStreamSynchronize(stream);
	elapsed = _now_s() - t0;
	*gbps = (double)done * (double)iosize / elapsed / 1e9;

out:
	if (events) {
		for (uint32_t i = 0; i < nslots; i++) {
			if (events[i]) {
				hipEventDestroy(events[i]);
			}
		}
	}
	if (stream) {
		hipStreamDestroy(stream);
	}
	if (dev) {
		hipFree(dev);
	}
	if (hbuf) {
		hipHostFree(hbuf);
	}
	hipGetLastError();
	free(events);
	free(pending);
	return err;
}
