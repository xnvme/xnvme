// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <xnvme_dev.h>
#include <xnvme_be_upcie_cplane_client.h>
#include <xnvme_be_upcie_cuda.h>
#include <xnvme_be_upcie_cuda_cqmirror.h>

static _Atomic int g_cuda_ctrlr_count;

static void
_cuda_ctrlr_term(struct xnvme_be_upcie_cuda_ctrlr *slot)
{
	CUcontext prev;

	if (!slot->ctrlr) {
		return;
	}

	/* The closing thread need not have the context current */
	if (slot->db_page) {
		cuCtxPushCurrent(g_upcie_cuda_rte.cu_ctx);
		cuMemHostUnregister(slot->db_page);
		cuCtxPopCurrent(&prev);
	}
	if (slot->db_own_map) {
		munmap(slot->db_own_map, slot->db_own_nbytes);
	}
	if (slot->reg_offset) {
		/* Unchecked: a server that has gone reclaims on the socket closing */
		xnvme_be_upcie_cplane_unregister_client_mem(slot->ctrlr, slot->reg_offset);
	}

	memset(slot, 0, sizeof(*slot));
}

static void
_cuda_ctx_term(void)
{
	CUcontext current = NULL;

	if (g_upcie_cuda_rte.cu_ctx_retained) {
		cuCtxGetCurrent(&current);
		if (current == g_upcie_cuda_rte.cu_ctx) {
			cuCtxSetCurrent(NULL);
		}
		cuDevicePrimaryCtxRelease(g_upcie_cuda_rte.cu_dev);
	}

	g_upcie_cuda_rte.cu_ctx = NULL;
	g_upcie_cuda_rte.cu_ctx_retained = 0;
}

static void
_cuda_rte_term(void)
{
	if (!g_upcie_cuda_rte.is_initialized) {
		return;
	}

	xnvme_be_upcie_cuda_cqmirror_term();
	for (int i = 0; i < XNVME_BE_CREF_MAX_ENTRIES; ++i) {
		_cuda_ctrlr_term(&g_upcie_cuda_rte.ctrlrs[i]);
	}

	dmamem_destroy(&g_upcie_cuda_rte.dmem);
	cudamem_heap_term(&g_upcie_cuda_rte.cuda_heap);
	_cuda_ctx_term();

	g_upcie_cuda_rte.is_initialized = 0;
}

static struct xnvme_be_upcie_cuda_ctrlr *
_cuda_ctrlr_slot(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	struct xnvme_be_upcie_cuda_ctrlr *free_slot = NULL;

	for (int i = 0; i < XNVME_BE_CREF_MAX_ENTRIES; ++i) {
		struct xnvme_be_upcie_cuda_ctrlr *slot = &g_upcie_cuda_rte.ctrlrs[i];

		if (slot->ctrlr == ctrlr) {
			return slot;
		}
		if (!slot->ctrlr && !free_slot) {
			free_slot = slot;
		}
	}

	if (free_slot) {
		memset(free_slot, 0, sizeof(*free_slot));
		free_slot->ctrlr = ctrlr;
	}

	return free_slot;
}

static int
_cuda_doorbells_init(struct xnvme_be_upcie_cuda_ctrlr *slot, void *bar0, uint64_t bar0_nbytes,
		     const char *bdf)
{
	long page_nbytes = sysconf(_SC_PAGESIZE);
	char path[256];
	void *mapped;
	int fd;

	if (!slot || !bar0 || !bar0_nbytes || !bdf) {
		return -EINVAL;
	}

	if (!cuMemHostRegister((char *)bar0 + XNVME_BE_UPCIE_DOORBELL_OFFSET, (size_t)page_nbytes,
			       CU_MEMHOSTREGISTER_IOMEMORY)) {
		slot->db_base = bar0;
		slot->db_page = (char *)bar0 + XNVME_BE_UPCIE_DOORBELL_OFFSET;

		return 0;
	}

	snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/resource0", bdf);
	fd = open(path, O_RDWR);
	if (fd < 0) {
		XNVME_DEBUG("FAILED: open(%s); errno(%d)", path, errno);
		return -errno;
	}

	mapped = mmap(NULL, bar0_nbytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	if (mapped == MAP_FAILED) {
		XNVME_DEBUG("FAILED: mmap(%s); errno(%d)", path, errno);
		return -errno;
	}

	if (cuMemHostRegister((char *)mapped + XNVME_BE_UPCIE_DOORBELL_OFFSET, (size_t)page_nbytes,
			      CU_MEMHOSTREGISTER_IOMEMORY)) {
		XNVME_DEBUG("FAILED: cuMemHostRegister(sysfs doorbells)");
		munmap(mapped, bar0_nbytes);
		return -ENOTSUP;
	}

	slot->db_base = mapped;
	slot->db_page = (char *)mapped + XNVME_BE_UPCIE_DOORBELL_OFFSET;
	slot->db_own_map = mapped;
	slot->db_own_nbytes = bar0_nbytes;

	return 0;
}

static int
_cuda_ctrlr_init(struct xnvme_be_upcie_cuda_ctrlr *slot, struct xnvme_be_upcie_ctrlr *ctrlr,
		 const char *bdf)
{
	void *bar0 = g_upcie_rte.connection.alive ? ctrlr->bar0 : ctrlr->ctrl->func.bars[0].region;
	size_t bar0_nbytes =
		g_upcie_rte.connection.alive ? ctrlr->bar0_nbytes : ctrlr->ctrl->func.bars[0].size;
	CUcontext prev;
	int err;

	/* Already registered when run again for doorbells the GPU could not reach */
	if (g_upcie_rte.connection.alive && !slot->reg_offset) {
		const struct hostmem_shared_desc *desc = NULL;

		err = xnvme_be_upcie_cplane_register_client_mem(
			ctrlr, g_upcie_cuda_rte.cuda_heap.dmabuf.fd,
			g_upcie_cuda_rte.cuda_heap.size,
			(uint32_t)g_upcie_cuda_rte.cuda_config.device_pagesize, &desc,
			&slot->reg_offset);
		if (err) {
			XNVME_DEBUG("FAILED: registering the CUDA heap for %s; err(%d)", bdf, err);
			memset(slot, 0, sizeof(*slot));
			return err;
		}
	}

	cuCtxPushCurrent(g_upcie_cuda_rte.cu_ctx);
	err = _cuda_doorbells_init(slot, bar0, bar0_nbytes, bdf);
	cuCtxPopCurrent(&prev);
	if (err) {
		XNVME_DEBUG("FAILED: no doorbell mapping the GPU can reach");
	}

	return 0;
}

static int
_cuda_ctx_init(CUdevice cu_dev, uint32_t gpu_id)
{
	CUcontext current = NULL;
	CUdevice current_dev;
	int err;

	err = cuCtxGetCurrent(&current);
	if (err) {
		XNVME_DEBUG("FAILED: cuCtxGetCurrent(); err(%d)", err);
		return -EIO;
	}
	g_upcie_cuda_rte.cu_dev = cu_dev;

	if (current) {
		err = cuCtxGetDevice(&current_dev);
		if (err) {
			XNVME_DEBUG("FAILED: cuCtxGetDevice(); err(%d)", err);
			return -EIO;
		}
		if (current_dev != cu_dev) {
			XNVME_DEBUG("FAILED: the current CUDA context is not on gpu_id(%u)",
				    gpu_id);
			return -EINVAL;
		}
		g_upcie_cuda_rte.cu_ctx = current;
		g_upcie_cuda_rte.cu_ctx_retained = 0;
		return 0;
	}

	err = cuDevicePrimaryCtxRetain(&g_upcie_cuda_rte.cu_ctx, cu_dev);
	if (err) {
		XNVME_DEBUG("FAILED: cuDevicePrimaryCtxRetain(); err(%d)", err);
		return -EIO;
	}
	err = cuCtxSetCurrent(g_upcie_cuda_rte.cu_ctx);
	if (err) {
		XNVME_DEBUG("FAILED: cuCtxSetCurrent(); err(%d)", err);
		cuDevicePrimaryCtxRelease(cu_dev);
		g_upcie_cuda_rte.cu_ctx = NULL;
		return -EIO;
	}
	g_upcie_cuda_rte.cu_ctx_retained = 1;

	return 0;
}

static int
_cuda_rte_init(size_t heap_size, uint32_t gpu_id, struct xnvme_be_upcie_ctrlr *ctrlr,
	       const char *bdf)
{
	struct xnvme_be_upcie_cuda_ctrlr *slot;
	CUdevice cu_dev;
	int err;

	slot = _cuda_ctrlr_slot(ctrlr);
	if (!slot) {
		XNVME_DEBUG("FAILED: already driving %d controllers from this process",
			    XNVME_BE_CREF_MAX_ENTRIES);
		return -ENOSPC;
	}

	if (g_upcie_cuda_rte.is_initialized) {
		return slot->db_base ? 0 : _cuda_ctrlr_init(slot, ctrlr, bdf);
	}

	if (!heap_size) {
		heap_size = XNVME_BE_UPCIE_DEFAULT_HEAP_SIZE;
	}

	err = cuInit(0);
	if (err) {
		XNVME_DEBUG("FAILED: cuInit(); err(%d)", err);
		_cuda_ctrlr_term(slot);
		return -ENODEV;
	}

	err = cuDeviceGet(&cu_dev, gpu_id);
	if (err) {
		XNVME_DEBUG("FAILED: cuDeviceGet(); err(%d)", err);
		_cuda_ctrlr_term(slot);
		return -ENODEV;
	}

	err = _cuda_ctx_init(cu_dev, gpu_id);
	if (err) {
		_cuda_ctrlr_term(slot);
		return err;
	}
	err = cudamem_config_init(&g_upcie_cuda_rte.cuda_config, 0);
	if (err) {
		XNVME_DEBUG("FAILED: cudamem_config_init(); err(%d)", err);
		_cuda_ctrlr_term(slot);
		_cuda_ctx_term();
		return err;
	}

	// align to the dma-buf page granularity used by the cudamem heap
	heap_size = ((heap_size + g_upcie_cuda_rte.cuda_config.device_pagesize - 1) /
		     g_upcie_cuda_rte.cuda_config.device_pagesize) *
		    g_upcie_cuda_rte.cuda_config.device_pagesize;

	err = cudamem_heap_init(&g_upcie_cuda_rte.cuda_heap, heap_size,
				&g_upcie_cuda_rte.cuda_config);
	if (err) {
		XNVME_DEBUG("FAILED: cudamem_heap_init(); err(%d)", err);
		_cuda_ctrlr_term(slot);
		_cuda_ctx_term();
		return err;
	}

	if (g_upcie_rte.connection.alive) {
		const struct hostmem_shared_desc *desc = NULL;

		err = xnvme_be_upcie_cplane_register_client_mem(
			ctrlr, g_upcie_cuda_rte.cuda_heap.dmabuf.fd,
			g_upcie_cuda_rte.cuda_heap.size,
			(uint32_t)g_upcie_cuda_rte.cuda_config.device_pagesize, &desc,
			&slot->reg_offset);
		if (!err) {
			if (_cuda_doorbells_init(slot, ctrlr->bar0, ctrlr->bar0_nbytes, bdf)) {
				XNVME_DEBUG("FAILED: no doorbell mapping the GPU can reach");
			}
			err = xnvme_be_upcie_dmem_from_desc(
				&g_upcie_cuda_rte.dmem,
				(void *)(uintptr_t)g_upcie_cuda_rte.cuda_heap.vaddr, desc,
				DMAMEM_BACKING_CUDAMEM,
				(uint32_t)g_upcie_cuda_rte.cuda_config.device_pagesize);
		}
		if (err) {
			XNVME_DEBUG("FAILED: registering the CUDA heap with the server; err(%d)",
				    err);
			_cuda_ctrlr_term(slot);
			cudamem_heap_term(&g_upcie_cuda_rte.cuda_heap);
			_cuda_ctx_term();
			return err;
		}
		g_upcie_cuda_rte.dmem_is_shared = 1;
	} else if (!xnvme_be_upcie_iova_range_required()) {
		err = dmamem_from_cuda_registry(&g_upcie_cuda_rte.dmem,
						&g_upcie_cuda_rte.cuda_heap,
						xnvme_be_upcie_va_bits());
		if (err) {
			XNVME_DEBUG("FAILED: dmamem_from_cuda_registry(); err(%d)", err);
			_cuda_ctrlr_term(slot);
			cudamem_heap_term(&g_upcie_cuda_rte.cuda_heap);
			_cuda_ctx_term();
			return err;
		}
		g_upcie_cuda_rte.dmem_is_shared = 1;
	} else if (g_upcie_rte.mode == XNVME_BE_UPCIE_MODE_VFIO_CDEV) {
		err = dmamem_from_cuda_iommufd(&g_upcie_cuda_rte.dmem, &g_upcie_cuda_rte.cuda_heap,
					       &g_upcie_rte.cdev.iommufd);
		if (err) {
			XNVME_DEBUG("FAILED: dmamem_from_cuda_iommufd(); err(%d); mapping "
				    "per controller instead",
				    err);
		} else {
			g_upcie_cuda_rte.dmem_is_shared = 1;
		}
	}

	if (!g_upcie_rte.connection.alive &&
	    _cuda_doorbells_init(slot, ctrlr->ctrl->func.bars[0].region,
				 ctrlr->ctrl->func.bars[0].size, bdf)) {
		XNVME_DEBUG("FAILED: no doorbell mapping the GPU can reach");
	}

	g_upcie_cuda_rte.is_initialized = 1;

	return 0;
}

static int
_cuda_dev_dmem_init(struct xnvme_dev *dev)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	int err;

	if (g_upcie_cuda_rte.dmem_is_shared && !g_upcie_cuda_rte.dmem_in_range) {
		state->dmem = &g_upcie_cuda_rte.dmem;
		return 0;
	}

	err = xnvme_be_upcie_iova_range_open(&state->range, dev->ident.uri);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_iova_range_open(%s); err(%d)", dev->ident.uri,
			    err);
		return err;
	}

	if (!g_upcie_cuda_rte.dmem_is_shared) {
		err = dmamem_from_cuda_iommu_map_pa(&g_upcie_cuda_rte.dmem,
						    &g_upcie_cuda_rte.cuda_heap,
						    xnvme_be_upcie_va_bits(), state->range.imp);
		if (err) {
			XNVME_DEBUG("FAILED: dmamem_from_cuda_iommu_map_pa(); err(%d)", err);
			xnvme_be_upcie_iova_range_close(&state->range);
			return err;
		}
		g_upcie_cuda_rte.dmem_is_shared = 1;
		g_upcie_cuda_rte.dmem_in_range = 1;
	}

	state->dmem = &g_upcie_cuda_rte.dmem;

	return 0;
}

static void
_cuda_dev_dmem_term(struct xnvme_dev *dev)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;

	state->dmem = NULL;

	if (!state->range.alive) {
		return;
	}

	/* The mappings go with the range, so the table goes with the last hold */
	if (atomic_load(&g_cuda_ctrlr_count) == 1) {
		dmamem_destroy(&g_upcie_cuda_rte.dmem);
		g_upcie_cuda_rte.dmem_is_shared = 0;
		g_upcie_cuda_rte.dmem_in_range = 0;
	}
	xnvme_be_upcie_iova_range_close(&state->range);
}

/**
 * Open a uPCIe CUDA device handle.
 *
 * Memory layout
 * -------------
 * This backend uses a hybrid memory model for PCIe P2P DMA:
 *
 *  - NVMe control structures (SQ, CQ, PRP lists) are allocated from the host
 *    hugepage heap (g_upcie_rte).  The CPU writes these structures and the
 *    NVMe controller reads them; host hugepages are required because the
 *    controller cannot DMA-read GPU DRAM through BAR1 for the control path.
 *
 *  - Data buffers (xnvme_buf_alloc) are allocated from the CUDA device heap
 *    (g_upcie_cuda_rte).  The NVMe controller accesses these directly via
 *    PCIe P2P DMA, bypassing host DRAM entirely.
 *
 * Consequently, both the host hugepage runtime and the CUDA heap are
 * initialized when the first upcie-cuda device is opened.
 */
static int
xnvme_be_upcie_cuda_dev_open(struct xnvme_dev *dev)
{
	int err;

	err = xnvme_be_upcie_dev_open(dev);
	if (err) {
		return err;
	}

	{
		struct xnvme_be_upcie_state *state = (void *)dev->be.state;

		err = _cuda_rte_init(dev->opts.device_heap_size, dev->opts.gpu_id, state->ctrlr,
				     dev->ident.uri);
	}
	if (err) {
		XNVME_DEBUG("FAILED: _cuda_rte_init(); err(%d)", err);
		return err;
	}

	/* Data buffers live in device memory for this backend; the control path
	 * (queues, PRP lists) stays on the host heap set by the base dev_open. */
	err = _cuda_dev_dmem_init(dev);
	if (err) {
		XNVME_DEBUG("FAILED: _cuda_dev_dmem_init(); err(%d)", err);
		if (!atomic_load(&g_cuda_ctrlr_count)) {
			_cuda_rte_term();
		}
		return err;
	}

	atomic_fetch_add(&g_cuda_ctrlr_count, 1);
	return 0;
}

static int
xnvme_be_upcie_cuda_ctrlr_term(void *handle)
{
	struct xnvme_be_upcie_cuda_ctrlr *slot = _cuda_ctrlr_slot_of(handle);

	if (slot) {
		_cuda_ctrlr_term(slot);
	}

	return xnvme_be_upcie_ctrlr_term(handle);
}

static void
xnvme_be_upcie_cuda_dev_close(struct xnvme_dev *dev)
{
	_cuda_dev_dmem_term(dev);

	if (atomic_fetch_sub(&g_cuda_ctrlr_count, 1) == 1) {
		_cuda_rte_term();
	}
	xnvme_be_upcie_dev_close(dev);
}

#endif

struct xnvme_be_dev g_xnvme_be_upcie_cuda_dev = {
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
	.dev_open = xnvme_be_upcie_cuda_dev_open,
	.dev_close = xnvme_be_upcie_cuda_dev_close,
	.id = "upcie-cuda",
	.ctrlr_init = xnvme_be_upcie_ctrlr_init,
	.ctrlr_term = xnvme_be_upcie_cuda_ctrlr_term,
#else
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};
