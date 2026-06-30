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

	cudamem_heap_term(&g_upcie_cuda_rte.cuda_heap);
	cuCtxDestroy(g_upcie_cuda_rte.cu_ctx);

	g_upcie_cuda_rte.is_initialized = 0;
}

static int
_cuda_rte_init(size_t heap_size)
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

	err = cuDeviceGet(&cu_dev, 0); // GPU ID 0
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

	if (strcmp(driver_name, "uio_pci_generic")) {
		XNVME_DEBUG(
			"FAILED: unsupported driver '%s', upcie-cuda requires 'uio_pci_generic'",
			driver_name);
		return -ENOTSUP;
	}

	return 0;
}

/**
 * Initialize the NVMe controller for a uPCIe CUDA device.
 *
 * Validates that the device is bound to uio_pci_generic (vfio-pci is not
 * supported for CUDA P2P DMA) and delegates to xnvme_be_upcie_ctrlr_init,
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

	err = _cuda_rte_init(dev->opts.device_heap_size);
	if (err) {
		XNVME_DEBUG("FAILED: _cuda_rte_init(); err(%d)", err);
		return err;
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
