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
#include <xnvme_dev.h>
#include <xnvme_be_upcie_cuda.h>

static _Atomic int g_cuda_ctrlr_count;

static void
_cuda_rte_term(void)
{
	if (!g_upcie_cuda_rte.is_initialized) {
		return;
	}

	dmamem_destroy(&g_upcie_cuda_rte.dmem);
	if (g_upcie_cuda_rte.imp_open) {
		/* After dmamem_destroy(): closing the descriptor drops every
		 * mapping built on it. */
		dmamem_iommu_map_pa_close(&g_upcie_cuda_rte.imp);
		g_upcie_cuda_rte.imp_open = 0;
	}
	if (g_upcie_cuda_rte.db_page) {
		cuMemHostUnregister(g_upcie_cuda_rte.db_page);
		g_upcie_cuda_rte.db_page = NULL;
	}
	if (g_upcie_cuda_rte.db_own_map) {
		munmap(g_upcie_cuda_rte.db_own_map, g_upcie_cuda_rte.db_own_nbytes);
		g_upcie_cuda_rte.db_own_map = NULL;
	}
	g_upcie_cuda_rte.db_base = NULL;
	if (g_upcie_cuda_rte.reg_offset) {
		/* Handed back before the memory behind it goes away, so the
		 * server is not left attached to a freed region. A server that
		 * has gone reclaims on the socket closing regardless, hence
		 * the unchecked return. */
		xnvme_be_upcie_cplane_unregister_client_mem(g_upcie_cuda_rte.reg_ctrlr,
							    g_upcie_cuda_rte.reg_offset);
		g_upcie_cuda_rte.reg_offset = 0;
		g_upcie_cuda_rte.reg_ctrlr = NULL;
	}
	cudamem_heap_term(&g_upcie_cuda_rte.cuda_heap);
	cuCtxDestroy(g_upcie_cuda_rte.cu_ctx);

	g_upcie_cuda_rte.is_initialized = 0;
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
 * @param bar0 This process's mapping of the controller's BAR0
 * @param bar0_nbytes How much of it is mapped
 * @param bdf The controller's address, for the sysfs fallback
 *
 * @return 0 on success, negative errno on failure
 */
#define NVME_DOORBELL_OFFSET 0x1000

static int
_cuda_doorbells_init(void *bar0, uint64_t bar0_nbytes, const char *bdf)
{
	long page_nbytes = sysconf(_SC_PAGESIZE);
	char path[256];
	void *mapped;
	int fd;

	if (!bar0 || !bar0_nbytes || !bdf) {
		return -EINVAL;
	}

	if (!cuMemHostRegister((char *)bar0 + NVME_DOORBELL_OFFSET, (size_t)page_nbytes,
			       CU_MEMHOSTREGISTER_IOMEMORY)) {
		g_upcie_cuda_rte.db_base = bar0;
		g_upcie_cuda_rte.db_page = (char *)bar0 + NVME_DOORBELL_OFFSET;

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

	if (cuMemHostRegister((char *)mapped + NVME_DOORBELL_OFFSET, (size_t)page_nbytes,
			      CU_MEMHOSTREGISTER_IOMEMORY)) {
		XNVME_DEBUG("FAILED: cuMemHostRegister(sysfs doorbells)");
		munmap(mapped, bar0_nbytes);
		return -ENOTSUP;
	}

	g_upcie_cuda_rte.db_base = mapped;
	g_upcie_cuda_rte.db_page = (char *)mapped + NVME_DOORBELL_OFFSET;
	g_upcie_cuda_rte.db_own_map = mapped;
	g_upcie_cuda_rte.db_own_nbytes = bar0_nbytes;

	return 0;
}

static int
_cuda_rte_init(size_t heap_size, uint32_t gpu_id, struct xnvme_be_upcie_ctrlr *ctrlr,
	       const char *bdf)
{
	CUdevice cu_dev;
	int err;

	if (g_upcie_cuda_rte.is_initialized) {
		return 0;
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

	/* Which addresses the controller consumes decides how the heap is
	 * described to it: physical where the IOMMU is out of the way, IOVAs
	 * where it is not. Choosing on the attachment mode rather than on the
	 * name of the bound driver means the answer comes from what the kernel
	 * does, not from what this once knew about it. */
	if (g_upcie_rte.connection.alive) {
		/* The controller belongs to the server, so the addresses it
		 * consumes are the server's to know. This process hands over
		 * the region and is told how it resolves. */
		const struct hostmem_shared_desc *desc = NULL;

		err = xnvme_be_upcie_cplane_register_client_mem(
			ctrlr, g_upcie_cuda_rte.cuda_heap.dmabuf.fd,
			g_upcie_cuda_rte.cuda_heap.size,
			(uint32_t)g_upcie_cuda_rte.cuda_config.device_pagesize, &desc,
			&g_upcie_cuda_rte.reg_offset);
		if (!err) {
			g_upcie_cuda_rte.reg_ctrlr = ctrlr;
			/* A kernel rings the doorbell itself, so the doorbells
			 * have to be somewhere the GPU can reach. */
			if (_cuda_doorbells_init(ctrlr->bar0, ctrlr->bar0_nbytes, bdf)) {
				XNVME_DEBUG("FAILED: no doorbell mapping the GPU can reach");
			}
			err = dmamem_from_shared(
				&g_upcie_cuda_rte.dmem,
				(void *)(uintptr_t)g_upcie_cuda_rte.cuda_heap.vaddr, desc,
				xnvme_be_upcie_va_bits(), DMAMEM_BACKING_CUDAMEM);
		}
	} else if (g_upcie_rte.mode == XNVME_BE_UPCIE_MODE_UIO_LUT) {
		err = dmamem_from_cuda_registry(&g_upcie_cuda_rte.dmem,
						&g_upcie_cuda_rte.cuda_heap,
						xnvme_be_upcie_va_bits());
	} else if (g_upcie_rte.mode == XNVME_BE_UPCIE_MODE_VFIO_CDEV) {
		/* The heap's addresses are physical ones, which an enforcing
		 * IOMMU will not take, and mapping the dma-buf instead is what
		 * IOMMU_IOAS_MAP_FILE refuses for a GPU exporter. So each
		 * allocation is inserted into the controller's domain and the
		 * table holds the IOVAs that came back. The window is claimed
		 * from the IOAS first, so what is handed out here cannot
		 * collide with what iommufd hands out later. */
		err = dmamem_iommu_map_pa_open(&g_upcie_cuda_rte.imp, bdf,
					       XNVME_BE_UPCIE_IMP_WINDOW_BASE,
					       XNVME_BE_UPCIE_IMP_WINDOW_SIZE);
		if (!err) {
			g_upcie_cuda_rte.imp_open = 1;
			err = dmamem_iommu_map_pa_reserve_window(&g_upcie_cuda_rte.imp,
								 &g_upcie_rte.cdev.iommufd);
		}
		if (!err) {
			err = dmamem_from_cuda_iommu_map_pa(
				&g_upcie_cuda_rte.dmem, &g_upcie_cuda_rte.cuda_heap,
				xnvme_be_upcie_va_bits(), &g_upcie_cuda_rte.imp);
		}
	} else {
		err = -ENOTSUP;
	}

	/* A controller this process opened rings its doorbells from the BAR the
	 * runtime already mapped, and a kernel issuing I/O needs that page in
	 * the GPU's address space just the same. */
	if (!err && !g_upcie_rte.connection.alive) {
		struct pci_func_bar *bar = &ctrlr->ctrl->func.bars[0];

		if (_cuda_doorbells_init(bar->region, bar->size, bdf)) {
			XNVME_DEBUG("FAILED: no doorbell mapping the GPU can reach");
		}
	}
	if (err) {
		XNVME_DEBUG("FAILED: describing the CUDA heap to the controller; err(%d)", err);
		cudamem_heap_term(&g_upcie_cuda_rte.cuda_heap);
		cuCtxDestroy(g_upcie_cuda_rte.cu_ctx);
		return err;
	}

	g_upcie_cuda_rte.is_initialized = 1;

	return 0;
}

static int
_check_driver(struct xnvme_dev *dev)
{
	char driver_name[sizeof(dev->ident.kernel_driver)] = {0};
	int err;

	err = xnvme_be_upcie_get_driver_name(dev->ident.uri, driver_name, sizeof(driver_name));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_get_driver_name(%s); err(%d)", dev->ident.uri,
			    err);
		return err;
	}
	snprintf(dev->ident.kernel_driver, sizeof(dev->ident.kernel_driver), "%s", driver_name);

	/* No check on which driver is bound: whether a controller behind an
	 * IOMMU can DMA into VRAM is the kernel's answer to give, and refusing
	 * here would go on being wrong after it stops being true. */

	return 0;
}

/**
 * Initialize the NVMe controller for a uPCIe CUDA device.
 *
 * Delegates to xnvme_be_upcie_ctrlr_init,
 * which opens the NVMe controller and allocates the host hugepage runtime.
 * CUDA runtime initialization is done in dev_open.
 */
void *
xnvme_be_upcie_cuda_ctrlr_init(struct xnvme_dev *dev)
{
	int err;
	err = _check_driver(dev);
	if (err) {
		errno = -err;
		return NULL;
	}

	return xnvme_be_upcie_ctrlr_init(dev);
}

int
xnvme_be_upcie_cuda_ctrlr_term(void *handle)
{
	return xnvme_be_upcie_ctrlr_term(handle);
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
 * Consequently, both the host hugepage runtime (256 MiB) and the CUDA heap
 * (1 GiB) are initialized when the first upcie-cuda device is opened.
 */
static int
xnvme_be_upcie_cuda_dev_open(struct xnvme_dev *dev)
{
	int err;

	err = _check_driver(dev);
	if (err) {
		return err;
	}

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
	{
		struct xnvme_be_upcie_state *state = (void *)dev->be.state;

		state->dmem = &g_upcie_cuda_rte.dmem;
	}

	atomic_fetch_add(&g_cuda_ctrlr_count, 1);
	return 0;
}

static void
xnvme_be_upcie_cuda_dev_close(struct xnvme_dev *dev)
{
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
	.ctrlr_init = xnvme_be_upcie_cuda_ctrlr_init,
	.ctrlr_term = xnvme_be_upcie_cuda_ctrlr_term,
#else
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};
