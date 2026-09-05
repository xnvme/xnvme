// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause
#include <libxnvme.h>
#include <errno.h>
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <hip/hip_runtime_api.h>
#include <xnvme_be_upcie_hip_cqmirror.h>

#define CQMIRROR_NREGIONS 4
#define CQMIRROR_CQ_NBYTES (2UL << 20)

extern const unsigned char xnvme_be_upcie_hip_cqmirror_co[];
extern const size_t xnvme_be_upcie_hip_cqmirror_co_len;

struct cqmirror_region {
	void *base;
	size_t nbytes;
	void *dev_base;
};

struct cqmirror_cq {
	void *mem;
	int in_use;
};

static struct {
	pthread_mutex_t lock;
	int device;
	int warp_size;
	hipModule_t module;
	hipFunction_t kernel;
	hipStream_t stream;
	void *table;
	uint32_t gen[XNVME_BE_UPCIE_HIP_CQMIRROR_NSLOTS];
	struct cqmirror_region regions[CQMIRROR_NREGIONS];
	struct cqmirror_cq cqs[XNVME_BE_UPCIE_HIP_CQMIRROR_NSLOTS];
	int nactive;
	int running;
} g_cqmirror = {.lock = PTHREAD_MUTEX_INITIALIZER, .device = -1};

/** The device the backend opened, made current for the calling thread */
static int
_device_enter(int *prev)
{
	if (hipGetDevice(prev) != hipSuccess) {
		return -EIO;
	}
	if (g_cqmirror.device < 0) {
		g_cqmirror.device = *prev;
	}
	if (g_cqmirror.device != *prev && hipSetDevice(g_cqmirror.device) != hipSuccess) {
		return -EIO;
	}

	return 0;
}

static void
_device_leave(int prev)
{
	if (g_cqmirror.device != prev) {
		hipSetDevice(prev);
	}
}

static void *
_slot_field(int idx, size_t offset)
{
	return (char *)g_cqmirror.table +
	       offsetof(struct xnvme_be_upcie_hip_cqmirror_table, slots) +
	       idx * sizeof(struct xnvme_be_upcie_hip_cqmirror_slot) + offset;
}

/**
 * The GPU's address for a host address, registering its region on first use
 */
static int
_region_devptr(void *host_base, size_t host_nbytes, void *host, void **devptr)
{
	struct cqmirror_region *region = NULL;
	hipError_t res;

	for (int i = 0; i < CQMIRROR_NREGIONS; ++i) {
		if (g_cqmirror.regions[i].base == host_base) {
			region = &g_cqmirror.regions[i];
			break;
		}
		if (!g_cqmirror.regions[i].base && !region) {
			region = &g_cqmirror.regions[i];
		}
	}
	if (!region) {
		return -ENOSPC;
	}
	if (!region->base) {
		res = hipHostRegister(host_base, host_nbytes, hipHostRegisterMapped);
		if (res != hipSuccess) {
			XNVME_DEBUG("FAILED: hipHostRegister(%p, %zu); res(%d)", host_base,
				    host_nbytes, res);
			return -EIO;
		}
		res = hipHostGetDevicePointer(&region->dev_base, host_base, 0);
		if (res != hipSuccess) {
			hipHostUnregister(host_base);
			return -EIO;
		}
		region->base = host_base;
		region->nbytes = host_nbytes;
	}
	if ((char *)host < (char *)region->base ||
	    (char *)host >= (char *)region->base + region->nbytes) {
		return -EINVAL;
	}
	*devptr = (char *)region->dev_base + ((char *)host - (char *)region->base);

	return 0;
}

static int
_load(void)
{
	hipError_t res;

	res = hipDeviceGetAttribute(&g_cqmirror.warp_size, hipDeviceAttributeWarpSize,
				    g_cqmirror.device);
	if (res != hipSuccess) {
		return -EIO;
	}
	res = hipExtMallocWithFlags(&g_cqmirror.table,
				    sizeof(struct xnvme_be_upcie_hip_cqmirror_table),
				    hipDeviceMallocUncached);
	if (res != hipSuccess) {
		XNVME_DEBUG("FAILED: hipExtMallocWithFlags(table); res(%d)", res);
		return -ENOMEM;
	}
	res = hipModuleLoadData(&g_cqmirror.module, xnvme_be_upcie_hip_cqmirror_co);
	if (res != hipSuccess) {
		XNVME_DEBUG("FAILED: hipModuleLoadData(); res(%d)", res);
		goto free_table;
	}
	res = hipModuleGetFunction(&g_cqmirror.kernel, g_cqmirror.module,
				   "xnvme_be_upcie_hip_cqmirror_kernel");
	if (res != hipSuccess) {
		goto unload;
	}
	res = hipStreamCreateWithFlags(&g_cqmirror.stream, hipStreamNonBlocking);
	if (res != hipSuccess) {
		goto unload;
	}

	return 0;

unload:
	hipModuleUnload(g_cqmirror.module);
	g_cqmirror.module = NULL;
free_table:
	hipFree(g_cqmirror.table);
	g_cqmirror.table = NULL;

	return -EIO;
}

static int
_launch(void)
{
	void *args[] = {&g_cqmirror.table};
	hipError_t res;
	int err;

	if (!g_cqmirror.table) {
		err = _load();
		if (err) {
			return err;
		}
	}
	res = hipMemsetD8((hipDeviceptr_t)g_cqmirror.table, 0,
			  sizeof(struct xnvme_be_upcie_hip_cqmirror_table));
	if (res != hipSuccess || hipStreamSynchronize(NULL) != hipSuccess) {
		return -EIO;
	}
	memset(g_cqmirror.gen, 0, sizeof(g_cqmirror.gen));

	res = hipModuleLaunchKernel(
		g_cqmirror.kernel,
		XNVME_BE_UPCIE_HIP_CQMIRROR_NSLOTS / XNVME_BE_UPCIE_HIP_CQMIRROR_WAVES_PER_BLOCK,
		1, 1, g_cqmirror.warp_size * XNVME_BE_UPCIE_HIP_CQMIRROR_WAVES_PER_BLOCK, 1, 1, 0,
		g_cqmirror.stream, args, NULL);
	if (res != hipSuccess) {
		XNVME_DEBUG("FAILED: hipModuleLaunchKernel(); res(%d)", res);
		return -EIO;
	}
	g_cqmirror.running = 1;

	return 0;
}

static void
_stop(void)
{
	const int32_t stop = 1;

	if (!g_cqmirror.running) {
		return;
	}
	hipMemcpyHtoD((hipDeviceptr_t)g_cqmirror.table, (void *)&stop, sizeof(stop));
	hipStreamSynchronize(g_cqmirror.stream);
	g_cqmirror.running = 0;
}

void *
xnvme_be_upcie_hip_cqmirror_cq_alloc(void)
{
	struct cqmirror_cq *cq = NULL;
	hipError_t res;
	int prev, err;

	pthread_mutex_lock(&g_cqmirror.lock);
	err = _device_enter(&prev);
	if (err) {
		pthread_mutex_unlock(&g_cqmirror.lock);
		errno = -err;
		return NULL;
	}
	for (int i = 0; i < XNVME_BE_UPCIE_HIP_CQMIRROR_NSLOTS; ++i) {
		if (g_cqmirror.cqs[i].in_use) {
			continue;
		}
		if (g_cqmirror.cqs[i].mem) {
			cq = &g_cqmirror.cqs[i];
			break;
		}
		if (!cq) {
			cq = &g_cqmirror.cqs[i];
		}
	}
	if (!cq) {
		err = -ENOSPC;
		goto exit;
	}
	if (!cq->mem) {
		res = hipExtMallocWithFlags(&cq->mem, CQMIRROR_CQ_NBYTES, hipDeviceMallocUncached);
		if (res != hipSuccess) {
			XNVME_DEBUG("FAILED: hipExtMallocWithFlags(cq); res(%d)", res);
			err = -ENOMEM;
			goto exit;
		}
	}
	/* The controller may only ever see phase one where a completion was
	 * written, and the wavefront expects the same of the host copy */
	res = hipMemsetD8((hipDeviceptr_t)cq->mem, 0, CQMIRROR_CQ_NBYTES);
	if (res != hipSuccess || hipStreamSynchronize(NULL) != hipSuccess) {
		err = -EIO;
		goto exit;
	}
	cq->in_use = 1;

exit:
	_device_leave(prev);
	pthread_mutex_unlock(&g_cqmirror.lock);
	if (err) {
		errno = -err;
		return NULL;
	}

	return cq->mem;
}

void
xnvme_be_upcie_hip_cqmirror_cq_release(void *cq_gpu)
{
	pthread_mutex_lock(&g_cqmirror.lock);
	for (int i = 0; i < XNVME_BE_UPCIE_HIP_CQMIRROR_NSLOTS; ++i) {
		if (g_cqmirror.cqs[i].mem == cq_gpu) {
			g_cqmirror.cqs[i].in_use = 0;
			break;
		}
	}
	pthread_mutex_unlock(&g_cqmirror.lock);
}

int
xnvme_be_upcie_hip_cqmirror_attach(void *host_base, size_t host_nbytes, const void *cq_gpu,
				   void *cq_host, uint16_t depth)
{
	struct xnvme_be_upcie_hip_cqmirror_slot slot = {0};
	void *cq_host_dev;
	int prev, err, idx;

	pthread_mutex_lock(&g_cqmirror.lock);
	err = _device_enter(&prev);
	if (err) {
		pthread_mutex_unlock(&g_cqmirror.lock);
		return err;
	}

	err = _region_devptr(host_base, host_nbytes, cq_host, &cq_host_dev);
	if (err) {
		goto exit;
	}
	if (!g_cqmirror.running) {
		err = _launch();
		if (err) {
			goto exit;
		}
	}
	for (idx = 0; idx < XNVME_BE_UPCIE_HIP_CQMIRROR_NSLOTS; ++idx) {
		if (!(g_cqmirror.gen[idx] & 1)) {
			break;
		}
	}
	if (idx == XNVME_BE_UPCIE_HIP_CQMIRROR_NSLOTS) {
		err = -ENOSPC;
		goto exit;
	}

	// The copies are synchronous, so the pointers are in place before gen turns
	slot.cq_gpu = (uintptr_t)cq_gpu;
	slot.cq_host = (uintptr_t)cq_host_dev;
	slot.depth = depth;
	if (hipMemcpyHtoD((hipDeviceptr_t)_slot_field(idx, 0), &slot,
			  offsetof(struct xnvme_be_upcie_hip_cqmirror_slot, gen)) != hipSuccess) {
		err = -EIO;
		goto exit;
	}
	g_cqmirror.gen[idx]++;
	if (hipMemcpyHtoD((hipDeviceptr_t)_slot_field(
				  idx, offsetof(struct xnvme_be_upcie_hip_cqmirror_slot, gen)),
			  &g_cqmirror.gen[idx], sizeof(uint32_t)) != hipSuccess) {
		g_cqmirror.gen[idx]--;
		err = -EIO;
		goto exit;
	}
	g_cqmirror.nactive++;
	err = idx;

exit:
	_device_leave(prev);
	pthread_mutex_unlock(&g_cqmirror.lock);

	return err;
}

void
xnvme_be_upcie_hip_cqmirror_detach(int slot_idx)
{
	uint32_t gen, ack;
	int prev;

	if (slot_idx < 0 || slot_idx >= XNVME_BE_UPCIE_HIP_CQMIRROR_NSLOTS) {
		return;
	}

	pthread_mutex_lock(&g_cqmirror.lock);
	if (_device_enter(&prev)) {
		pthread_mutex_unlock(&g_cqmirror.lock);
		return;
	}
	gen = g_cqmirror.gen[slot_idx]++;
	hipMemcpyHtoD((hipDeviceptr_t)_slot_field(
			      slot_idx, offsetof(struct xnvme_be_upcie_hip_cqmirror_slot, gen)),
		      &g_cqmirror.gen[slot_idx], sizeof(uint32_t));
	do {
		usleep(100);
		if (hipMemcpyDtoH(&ack,
				  (hipDeviceptr_t)_slot_field(
					  slot_idx,
					  offsetof(struct xnvme_be_upcie_hip_cqmirror_slot, ack)),
				  sizeof(ack)) != hipSuccess) {
			break;
		}
	} while (ack != gen);
	if (!--g_cqmirror.nactive) {
		_stop();
	}
	_device_leave(prev);
	pthread_mutex_unlock(&g_cqmirror.lock);
}

void
xnvme_be_upcie_hip_cqmirror_term(void)
{
	int prev;

	pthread_mutex_lock(&g_cqmirror.lock);
	if (g_cqmirror.device < 0 || _device_enter(&prev)) {
		pthread_mutex_unlock(&g_cqmirror.lock);
		return;
	}
	_stop();
	for (int i = 0; i < CQMIRROR_NREGIONS; ++i) {
		if (g_cqmirror.regions[i].base) {
			hipHostUnregister(g_cqmirror.regions[i].base);
			g_cqmirror.regions[i].base = NULL;
		}
	}
	for (int i = 0; i < XNVME_BE_UPCIE_HIP_CQMIRROR_NSLOTS; ++i) {
		if (g_cqmirror.cqs[i].mem) {
			hipFree(g_cqmirror.cqs[i].mem);
			g_cqmirror.cqs[i].mem = NULL;
		}
	}
	if (g_cqmirror.table) {
		hipStreamDestroy(g_cqmirror.stream);
		hipModuleUnload(g_cqmirror.module);
		hipFree(g_cqmirror.table);
		g_cqmirror.table = NULL;
	}
	_device_leave(prev);
	g_cqmirror.device = -1;
	pthread_mutex_unlock(&g_cqmirror.lock);
}
#endif
