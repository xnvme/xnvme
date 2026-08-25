// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
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
	cudamem_heap_term(&g_upcie_cuda_rte.cuda_heap);
	cuCtxDestroy(g_upcie_cuda_rte.cu_ctx);

	g_upcie_cuda_rte.is_initialized = 0;
}

static int
_cuda_rte_init(size_t heap_size, uint32_t gpu_id)
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
		return -ENOMEM;
	}

	/* Which addresses the controller consumes decides how the heap is
	 * described to it: physical where the IOMMU is out of the way, IOVAs
	 * where it is not. Choosing on the attachment mode rather than on the
	 * name of the bound driver means the answer comes from what the kernel
	 * does, not from what this once knew about it. */
	if (g_upcie_rte.mode == XNVME_BE_UPCIE_MODE_UIO_LUT) {
		err = dmamem_from_cuda_registry(&g_upcie_cuda_rte.dmem,
						&g_upcie_cuda_rte.cuda_heap,
						xnvme_be_upcie_va_bits());
	} else if (g_upcie_rte.mode == XNVME_BE_UPCIE_MODE_VFIO_CDEV) {
		err = dmamem_from_cuda_iommufd(&g_upcie_cuda_rte.dmem, &g_upcie_cuda_rte.cuda_heap,
					       &g_upcie_rte.cdev.iommufd);
	} else {
		err = -ENOTSUP;
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

	err = _cuda_rte_init(dev->opts.device_heap_size, dev->opts.gpu_id);
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
