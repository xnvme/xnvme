// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <xnvme_be_upcie_cuda_cqmirror.h>

#define CQMIRROR_NSLOTS 64
#define CQMIRROR_WARPS_PER_BLOCK 8
#define CQMIRROR_NREGIONS 4

/**
 * A queue as its warp sees it; 64 bytes so a slot is one line
 *
 * The host fills the pointers and then bumps gen to odd; the warp works until
 * it sees gen change, then reports the value it left on in ack. The table
 * lives in VRAM: an idle warp polls its slot, and sixty of them polling host
 * memory over PCIe cost the drive a third of its completion rate.
 */
struct cqmirror_slot {
	const uint4 *cq_gpu;
	uint4 *cq_host;
	uint32_t depth;
	uint32_t gen;
	uint32_t ack;
	uint32_t _rsvd[7];
};

struct cqmirror_table {
	int stop;
	int _rsvd[15];
	struct cqmirror_slot slots[CQMIRROR_NSLOTS];
};

/**
 * A warp per slot, copying whole completions as their phase bit turns
 *
 * Each lane loads one entry from the head onwards; the ballot of phase
 * matches gives how many are new, as a run from the head, and those are
 * stored. The loads bypass the caches, since the controller writes VRAM
 * behind the GPU's back, and the stores stream to host memory.
 */
__global__ static void
xnvme_be_upcie_cuda_cqmirror_kernel(struct cqmirror_table *table)
{
	const uint32_t lane = threadIdx.x & 31;
	const uint32_t warp = blockIdx.x * (blockDim.x >> 5) + (threadIdx.x >> 5);
	volatile struct cqmirror_slot *slot = &table->slots[warp];

	while (!*(volatile int *)&table->stop) {
		const uint32_t gen = slot->gen;
		const uint4 *cq_gpu;
		uint4 *cq_host;
		uint32_t depth, head = 0, phase = 1, spins = 0;

		if (!(gen & 1)) {
			__nanosleep(50000);
			continue;
		}
		cq_gpu = slot->cq_gpu;
		cq_host = slot->cq_host;
		depth = slot->depth;

		while (true) {
			uint32_t idx = head + lane, ph = phase, n;
			unsigned mask;
			uint4 v;

			if (idx >= depth) {
				idx -= depth;
				ph ^= 1;
			}
			v = __ldcv(&cq_gpu[lane < depth ? idx : 0]);
			mask = __ballot_sync(0xffffffffu, lane < depth && ((v.w >> 16) & 1) == ph);
			n = (mask == 0xffffffffu) ? 32 : (__ffs(~mask) - 1);
			if (lane < n) {
				__stcs(&cq_host[idx], v);
			}
			__syncwarp();
			head += n;
			if (head >= depth) {
				head -= depth;
				phase ^= 1;
			}
			if (!n) {
				__nanosleep(500);
				if (!(++spins & 1023) && slot->gen != gen) {
					break;
				}
			}
		}
		/* The host frees the CQ once it sees the ack, so the copies land first */
		__threadfence_system();
		__syncwarp();
		if (!lane) {
			slot->ack = gen;
		}
	}
}

struct cqmirror_region {
	void *base;
	size_t nbytes;
	CUdeviceptr dev_base;
};

static struct {
	pthread_mutex_t lock;
	CUcontext ctx;
	CUstream stream;
	CUdeviceptr table;
	uint32_t gen[CQMIRROR_NSLOTS];
	struct cqmirror_region regions[CQMIRROR_NREGIONS];
	int nactive;
	int running;
} g_cqmirror = {PTHREAD_MUTEX_INITIALIZER, NULL, NULL, 0, {}, {}, 0, 0};

static CUdeviceptr
_slot_field(int idx, size_t offset)
{
	return g_cqmirror.table + offsetof(struct cqmirror_table, slots) +
	       idx * sizeof(struct cqmirror_slot) + offset;
}

/**
 * The GPU's address for a host address, registering its region on first use
 */
static int
_region_devptr(void *host_base, size_t host_nbytes, void *host, CUdeviceptr *devptr)
{
	struct cqmirror_region *region = NULL;
	CUresult res;

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
		res = cuMemHostRegister(host_base, host_nbytes, CU_MEMHOSTREGISTER_DEVICEMAP);
		if (res != CUDA_SUCCESS) {
			return -EIO;
		}
		res = cuMemHostGetDevicePointer(&region->dev_base, host_base, 0);
		if (res != CUDA_SUCCESS) {
			cuMemHostUnregister(host_base);
			return -EIO;
		}
		region->base = host_base;
		region->nbytes = host_nbytes;
	}
	if ((char *)host < (char *)region->base ||
	    (char *)host >= (char *)region->base + region->nbytes) {
		return -EINVAL;
	}
	*devptr = region->dev_base + ((char *)host - (char *)region->base);

	return 0;
}

static int
_launch(void)
{
	CUresult res;
	cudaError_t cerr;

	if (!g_cqmirror.table) {
		res = cuMemAlloc(&g_cqmirror.table, sizeof(struct cqmirror_table));
		if (res != CUDA_SUCCESS) {
			return -ENOMEM;
		}
		res = cuStreamCreate(&g_cqmirror.stream, CU_STREAM_NON_BLOCKING);
		if (res != CUDA_SUCCESS) {
			cuMemFree(g_cqmirror.table);
			g_cqmirror.table = 0;
			return -EIO;
		}
	}
	res = cuMemsetD8(g_cqmirror.table, 0, sizeof(struct cqmirror_table));
	if (res != CUDA_SUCCESS || cuStreamSynchronize(NULL) != CUDA_SUCCESS) {
		return -EIO;
	}
	memset(g_cqmirror.gen, 0, sizeof(g_cqmirror.gen));

	xnvme_be_upcie_cuda_cqmirror_kernel<<<CQMIRROR_NSLOTS / CQMIRROR_WARPS_PER_BLOCK,
					      32 * CQMIRROR_WARPS_PER_BLOCK, 0,
					      (cudaStream_t)g_cqmirror.stream>>>(
		(struct cqmirror_table *)g_cqmirror.table);
	cerr = cudaGetLastError();
	if (cerr != cudaSuccess) {
		return -EIO;
	}
	g_cqmirror.running = 1;

	return 0;
}

static void
_stop(void)
{
	if (!g_cqmirror.running) {
		return;
	}
	const int stop = 1;

	cuMemcpyHtoD(g_cqmirror.table + offsetof(struct cqmirror_table, stop), &stop,
		     sizeof(stop));
	cuStreamSynchronize(g_cqmirror.stream);
	g_cqmirror.running = 0;
}

int
xnvme_be_upcie_cuda_cqmirror_attach(CUcontext ctx, void *host_base, size_t host_nbytes,
				    const void *cq_gpu, void *cq_host, uint16_t depth)
{
	struct cqmirror_slot slot = {};
	CUdeviceptr cq_host_dev;
	CUcontext prev;
	int err, idx;

	pthread_mutex_lock(&g_cqmirror.lock);
	cuCtxPushCurrent(ctx);
	g_cqmirror.ctx = ctx;

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
	for (idx = 0; idx < CQMIRROR_NSLOTS; ++idx) {
		if (!(g_cqmirror.gen[idx] & 1)) {
			break;
		}
	}
	if (idx == CQMIRROR_NSLOTS) {
		err = -ENOSPC;
		goto exit;
	}

	// The copies are synchronous, so the pointers are in place before gen turns
	slot.cq_gpu = (const uint4 *)cq_gpu;
	slot.cq_host = (uint4 *)cq_host_dev;
	slot.depth = depth;
	if (cuMemcpyHtoD(_slot_field(idx, 0), &slot, offsetof(struct cqmirror_slot, gen)) !=
	    CUDA_SUCCESS) {
		err = -EIO;
		goto exit;
	}
	g_cqmirror.gen[idx]++;
	if (cuMemcpyHtoD(_slot_field(idx, offsetof(struct cqmirror_slot, gen)),
			 &g_cqmirror.gen[idx], sizeof(uint32_t)) != CUDA_SUCCESS) {
		g_cqmirror.gen[idx]--;
		err = -EIO;
		goto exit;
	}
	g_cqmirror.nactive++;
	err = idx;

exit:
	cuCtxPopCurrent(&prev);
	pthread_mutex_unlock(&g_cqmirror.lock);

	return err;
}

void
xnvme_be_upcie_cuda_cqmirror_detach(int slot_idx)
{
	CUcontext prev;
	uint32_t gen, ack;

	if (slot_idx < 0 || slot_idx >= CQMIRROR_NSLOTS) {
		return;
	}

	pthread_mutex_lock(&g_cqmirror.lock);
	cuCtxPushCurrent(g_cqmirror.ctx);
	gen = g_cqmirror.gen[slot_idx]++;
	cuMemcpyHtoD(_slot_field(slot_idx, offsetof(struct cqmirror_slot, gen)),
		     &g_cqmirror.gen[slot_idx], sizeof(uint32_t));
	do {
		usleep(100);
		if (cuMemcpyDtoH(&ack, _slot_field(slot_idx, offsetof(struct cqmirror_slot, ack)),
				 sizeof(ack)) != CUDA_SUCCESS) {
			break;
		}
	} while (ack != gen);
	if (!--g_cqmirror.nactive) {
		_stop();
	}
	cuCtxPopCurrent(&prev);
	pthread_mutex_unlock(&g_cqmirror.lock);
}

void
xnvme_be_upcie_cuda_cqmirror_term(void)
{
	CUcontext prev;

	pthread_mutex_lock(&g_cqmirror.lock);
	if (!g_cqmirror.table) {
		pthread_mutex_unlock(&g_cqmirror.lock);
		return;
	}
	cuCtxPushCurrent(g_cqmirror.ctx);
	_stop();
	for (int i = 0; i < CQMIRROR_NREGIONS; ++i) {
		if (g_cqmirror.regions[i].base) {
			cuMemHostUnregister(g_cqmirror.regions[i].base);
			g_cqmirror.regions[i].base = NULL;
		}
	}
	cuStreamDestroy(g_cqmirror.stream);
	cuMemFree(g_cqmirror.table);
	g_cqmirror.table = 0;
	cuCtxPopCurrent(&prev);
	pthread_mutex_unlock(&g_cqmirror.lock);
}
