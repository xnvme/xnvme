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
_cuda_rte_init(void)
{
	CUdevice cu_dev;
	int err;

	if (g_upcie_cuda_rte.is_initialized) {
		return 0;
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

	err = cuCtxCreate(&g_upcie_cuda_rte.cu_ctx, 0, cu_dev);
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

	err = cudamem_heap_init(&g_upcie_cuda_rte.cuda_heap, 1024 * 1024 * 1024,
				&g_upcie_cuda_rte.cuda_config);
	if (err) {
		XNVME_DEBUG("FAILED: cudamem_heap_init(); err(%d)", err);
		cuCtxDestroy(g_upcie_cuda_rte.cu_ctx);
		return -ENOMEM;
	}

	g_upcie_cuda_rte.is_initialized = 1;

	return 0;
}

int
xnvme_be_upcie_cuda_ctrlr_term(void *handle)
{
	xnvme_be_upcie_ctrlr_term(handle);

	if (--g_cuda_ctrlr_count == 0) {
		_cuda_rte_term();
	}

	return 0;
}

/**
 * Initialize a uPCIe CUDA controller.
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
void *
xnvme_be_upcie_cuda_ctrlr_init(struct xnvme_dev *dev)
{
	struct xnvme_be_upcie_ctrlr *ctrlr;
	char driver_name[sizeof(dev->ident.kernel_driver)] = {0};
	int err;

	err = xnvme_be_upcie_get_driver_name(dev->ident.uri, driver_name, sizeof(driver_name));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_get_driver_name(%s); err(%d)", dev->ident.uri,
			    err);
		errno = -err;
		return NULL;
	}
	snprintf(dev->ident.kernel_driver, sizeof(dev->ident.kernel_driver), "%s", driver_name);

	if (!strcmp(driver_name, "vfio-pci")) {
		XNVME_DEBUG("FAILED: upcie-cuda does not support vfio-pci");
		errno = ENOTSUP;
		return NULL;
	} else if (strcmp(driver_name, "uio_pci_generic")) {
		XNVME_DEBUG("FAILED: unsupported driver '%s'", driver_name);
		errno = ENOTSUP;
		return NULL;
	}

	err = _cuda_rte_init();
	if (err) {
		XNVME_DEBUG("FAILED: _cuda_rte_init(); err(%d)", err);
		errno = -err;
		return NULL;
	}

	ctrlr = xnvme_be_upcie_ctrlr_init(dev);
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_ctrlr_init(); err(%d)", errno);
		if (g_cuda_ctrlr_count == 0) {
			_cuda_rte_term();
		}
		return NULL;
	}

	g_cuda_ctrlr_count++;

	return ctrlr;
}

#endif

struct xnvme_be_dev g_xnvme_be_upcie_cuda_dev = {
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
	.enumerate = xnvme_be_upcie_enumerate,
	.dev_open = xnvme_be_upcie_dev_open,
	.dev_close = xnvme_be_upcie_dev_close,
	.id = "upcie-cuda",
	.ctrlr_init = xnvme_be_upcie_cuda_ctrlr_init,
	.ctrlr_term = xnvme_be_upcie_cuda_ctrlr_term,
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};
