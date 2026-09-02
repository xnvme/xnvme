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
#include <xnvme_be_upcie_cuda.h>

static _Atomic int g_cuda_ctrlr_count;

/**
 * Give back what one controller needed, leaving the heap alone
 *
 * @param slot The controller's slot; cleared on return
 */
static void
_cuda_ctrlr_term(struct xnvme_be_upcie_cuda_ctrlr *slot)
{
	if (!slot->ctrlr) {
		return;
	}

	if (slot->db_page) {
		cuMemHostUnregister(slot->db_page);
	}
	if (slot->db_own_map) {
		munmap(slot->db_own_map, slot->db_own_nbytes);
	}
	if (slot->reg_offset) {
		/* Handed back before the memory behind it goes away, so the
		 * server is not left attached to a freed region. A server that
		 * has gone reclaims on the socket closing regardless, hence the
		 * unchecked return. */
		xnvme_be_upcie_cplane_unregister_client_mem(slot->ctrlr, slot->reg_offset);
	}

	memset(slot, 0, sizeof(*slot));
}

static void
_cuda_rte_term(void)
{
	if (!g_upcie_cuda_rte.is_initialized) {
		return;
	}

	for (int i = 0; i < XNVME_BE_UPCIE_GPU_CTRLRS_MAX; ++i) {
		_cuda_ctrlr_term(&g_upcie_cuda_rte.ctrlrs[i]);
	}

	dmamem_destroy(&g_upcie_cuda_rte.dmem);
	/* After dmamem_destroy(): closing the range drops every mapping made
	 * in it. */
	cudamem_heap_term(&g_upcie_cuda_rte.cuda_heap);
	cuCtxDestroy(g_upcie_cuda_rte.cu_ctx);

	g_upcie_cuda_rte.is_initialized = 0;
}

/**
 * The per-controller slot for `ctrlr`, claiming a free one if it has none
 *
 * @param ctrlr The controller to look up
 *
 * @return The slot, or NULL when this process is already driving as many
 *         controllers as it can
 */
static struct xnvme_be_upcie_cuda_ctrlr *
_cuda_ctrlr_slot(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	struct xnvme_be_upcie_cuda_ctrlr *free_slot = NULL;

	for (int i = 0; i < XNVME_BE_UPCIE_GPU_CTRLRS_MAX; ++i) {
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

/**
 * Find a doorbell mapping the GPU can be given
 *
 * A kernel issuing I/O writes the doorbell itself, which means the doorbell
 * page has to be mapped into the GPU's address space, and only the driver can
 * put it there. It resolves the mapping to a physical address to do so, and it
 * cannot do that for a vfio device mapping: only the first page of one is ever
 * accepted, and the doorbells are not in it. The BAR's sysfs resource maps to
 * the same registers and does resolve, so that is what the GPU is given when
 * this process's own mapping is refused.
 *
 * @param slot The controller's slot, filled in on success
 * @param bar0 This process's mapping of the controller's BAR0
 * @param bar0_nbytes How much of it is mapped
 * @param bdf The controller's address, for the sysfs fallback
 *
 * @return 0 on success, negative errno on failure
 */
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

/**
 * Make one more controller reachable, reusing the heap already built
 *
 * The heap and the addresses it resolves to belong to the process, so a second
 * controller adds only what is its own. Where those addresses are physical that
 * is just the doorbells. Where they are IOVAs the heap has to reach the new
 * controller at the addresses the table already holds, which is what attaching
 * it to the range does, or what registering the heap again asks the server
 * for.
 *
 * @param slot The controller's slot, filled in on success
 * @param ctrlr The controller being opened
 * @param bdf Its address, for the sysfs doorbell fallback
 *
 * @return 0 on success, negative errno on failure
 */
static int
_cuda_ctrlr_init(struct xnvme_be_upcie_cuda_ctrlr *slot, struct xnvme_be_upcie_ctrlr *ctrlr,
		 const char *bdf)
{
	/* A served controller has no PCI function here; the BAR the server
	 * mapped is what this process was given. */
	void *bar0 = g_upcie_rte.connection.alive ? ctrlr->bar0 : ctrlr->ctrl->func.bars[0].region;
	size_t bar0_nbytes =
		g_upcie_rte.connection.alive ? ctrlr->bar0_nbytes : ctrlr->ctrl->func.bars[0].size;

	if (g_upcie_rte.connection.alive) {
		/* Registered again for this controller. The server recognises
		 * the region and hands back the description it already gave,
		 * so what the process resolves against does not change. */
		const struct hostmem_shared_desc *desc = NULL;
		int err;

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

	if (_cuda_doorbells_init(slot, bar0, bar0_nbytes, bdf)) {
		XNVME_DEBUG("FAILED: no doorbell mapping the GPU can reach");
		memset(slot, 0, sizeof(*slot));
		return -ENOTSUP;
	}

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
			    XNVME_BE_UPCIE_GPU_CTRLRS_MAX);
		return -ENOSPC;
	}

	/* One heap for the process, but every controller needs its own way in:
	 * a description the server made for it, or a doorbell page of its own.
	 * So a second controller runs the per-controller half again and leaves
	 * the heap alone. */
	if (g_upcie_cuda_rte.is_initialized) {
		return slot->db_base ? 0 : _cuda_ctrlr_init(slot, ctrlr, bdf);
	}

	if (!heap_size) {
		heap_size = XNVME_BE_UPCIE_DEFAULT_HEAP_SIZE;
	}

	err = cuInit(0);
	if (err) {
		XNVME_DEBUG("FAILED: cuInit(); err(%d)", err);
		return -ENODEV;
	}

	err = cuDeviceGet(&cu_dev, gpu_id);
	if (err) {
		XNVME_DEBUG("FAILED: cuDeviceGet(); err(%d)", err);
		return -ENODEV;
	}

	// CUDA 13 redefines cuCtxCreate -> cuCtxCreate_v4, which takes an extra
	// CUctxCreateParams* (NULL = the old default); CUDA 12 keeps the 3-arg form.
#if CUDA_VERSION >= 13000
	err = cuCtxCreate(&g_upcie_cuda_rte.cu_ctx, NULL, 0, cu_dev);
#else
	err = cuCtxCreate(&g_upcie_cuda_rte.cu_ctx, 0, cu_dev);
#endif
	if (err) {
		XNVME_DEBUG("FAILED: cuCtxCreate(); err(%d)", err);
		return -EIO;
	}
	err = cudamem_config_init(&g_upcie_cuda_rte.cuda_config, 0);
	if (err) {
		XNVME_DEBUG("FAILED: cudamem_config_init(); err(%d)", err);
		cuCtxDestroy(g_upcie_cuda_rte.cu_ctx);
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
		cuCtxDestroy(g_upcie_cuda_rte.cu_ctx);
		return err;
	}

	/* How the heap is described to a controller depends on what that
	 * controller consumes, and on whether this process owns it at all. A
	 * client owns none of them, so the server answers. Otherwise: physical
	 * addresses read the same from every controller, so one table serves
	 * them all; under an enforcing IOMMU they do not, and there are two
	 * ways to get addresses it will accept. iommufd maps the heap once for
	 * the process and needs no out-of-tree module, so it is preferred where
	 * the device is on vfio-cdev; where it cannot map, _cuda_dev_dmem_init()
	 * falls back to iommu-map-pa, which maps the heap per controller into
	 * whichever domain each is already in. */
	if (g_upcie_rte.connection.alive) {
		/* The controller belongs to the server, so the addresses it
		 * consumes are the server's to know. This process hands over
		 * the region and is told how it resolves. */
		const struct hostmem_shared_desc *desc = NULL;

		err = xnvme_be_upcie_cplane_register_client_mem(
			ctrlr, g_upcie_cuda_rte.cuda_heap.dmabuf.fd,
			g_upcie_cuda_rte.cuda_heap.size,
			(uint32_t)g_upcie_cuda_rte.cuda_config.device_pagesize, &desc,
			&slot->reg_offset);
		if (!err) {
			/* A kernel rings the doorbell itself, so the doorbells
			 * have to be somewhere the GPU can reach. */
			if (_cuda_doorbells_init(slot, ctrlr->bar0, ctrlr->bar0_nbytes, bdf)) {
				XNVME_DEBUG("FAILED: no doorbell mapping the GPU can reach");
			}
			err = dmamem_from_shared(
				&g_upcie_cuda_rte.dmem,
				(void *)(uintptr_t)g_upcie_cuda_rte.cuda_heap.vaddr, desc,
				xnvme_be_upcie_va_bits(), DMAMEM_BACKING_CUDAMEM);
		}
		if (err) {
			XNVME_DEBUG("FAILED: registering the CUDA heap with the server; err(%d)",
				    err);
			cudamem_heap_term(&g_upcie_cuda_rte.cuda_heap);
			cuCtxDestroy(g_upcie_cuda_rte.cu_ctx);
			return err;
		}
		g_upcie_cuda_rte.dmem_is_shared = 1;
	} else if (!xnvme_be_upcie_gpu_map_required()) {
		err = dmamem_from_cuda_registry(&g_upcie_cuda_rte.dmem,
						&g_upcie_cuda_rte.cuda_heap,
						xnvme_be_upcie_va_bits());
		if (err) {
			XNVME_DEBUG("FAILED: dmamem_from_cuda_registry(); err(%d)", err);
			cudamem_heap_term(&g_upcie_cuda_rte.cuda_heap);
			cuCtxDestroy(g_upcie_cuda_rte.cu_ctx);
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

	g_upcie_cuda_rte.is_initialized = 1;

	return 0;
}

/** Heap bytes to map, rounded as the registry rounds a registration */
static uint64_t
_cuda_slice_span(const struct cudamem_heap *heap)
{
	const uint64_t gran = DMAMEM_CUDA_REGISTRY_GRANULARITY;

	return ((heap->size + gran - 1) & ~(gran - 1)) + gran;
}

/** Point the device at the runtime's table, or build it one of its own */
static int
_cuda_dev_dmem_init(struct xnvme_dev *dev)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	struct xnvme_be_upcie_gpu_dmem *gpu;
	int err;

	/* Set where the runtime described the heap once for the whole process,
	 * whether in physical addresses or through iommufd. */
	if (g_upcie_cuda_rte.dmem_is_shared) {
		state->dmem = &g_upcie_cuda_rte.dmem;
		return 0;
	}

	gpu = calloc(1, sizeof(*gpu));
	if (!gpu) {
		return -ENOMEM;
	}

	err = xnvme_be_upcie_gpu_map_open(&gpu->map, dev->ident.uri,
					  _cuda_slice_span(&g_upcie_cuda_rte.cuda_heap));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_gpu_map_open(%s); err(%d)", dev->ident.uri,
			    err);
		free(gpu);
		return err;
	}

	err = dmamem_from_cuda_iommu_map_pa(&gpu->dmem, &g_upcie_cuda_rte.cuda_heap,
					    xnvme_be_upcie_va_bits(), &gpu->map.imp);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_from_cuda_iommu_map_pa(); err(%d)", err);
		xnvme_be_upcie_gpu_map_close(&gpu->map);
		free(gpu);
		return err;
	}

	state->gpu = gpu;
	state->dmem = &gpu->dmem;

	return 0;
}

static void
_cuda_dev_dmem_term(struct xnvme_dev *dev)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;

	state->dmem = NULL;

	if (!state->gpu) {
		return;
	}

	/* Unmap before ctrlr_term detaches and replaces the domain. */
	dmamem_destroy(&state->gpu->dmem);
	xnvme_be_upcie_gpu_map_close(&state->gpu->map);

	free(state->gpu);
	state->gpu = NULL;
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
	.ctrlr_term = xnvme_be_upcie_ctrlr_term,
#else
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};
