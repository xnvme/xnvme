// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <stdatomic.h>
#include <xnvme_dev.h>
#include <xnvme_be_upcie_hip.h>

static _Atomic int g_hip_ctrlr_count;

static void
_hip_rte_term(void)
{
	if (!g_upcie_hip_rte.is_initialized) {
		return;
	}

	hipmem_heap_term(&g_upcie_hip_rte.hip_heap);

	g_upcie_hip_rte.is_initialized = 0;
}

static int
_hip_rte_init(size_t heap_size)
{
	int err;

	if (g_upcie_hip_rte.is_initialized) {
		return 0;
	}

	if (!heap_size) {
		heap_size = XNVME_BE_UPCIE_DEFAULT_HEAP_SIZE;
	}

	err = hipInit(0);
	if (err) {
		XNVME_DEBUG("FAILED: hipInit(); err(%d)", err);
		return -ENODEV;
	}

	err = hipSetDevice(0); // GPU ID 0
	if (err) {
		XNVME_DEBUG("FAILED: hipSetDevice(); err(%d)", err);
		return -ENODEV;
	}

	err = hipmem_config_init(&g_upcie_hip_rte.hip_config, 0);
	if (err) {
		XNVME_DEBUG("FAILED: hipmem_config_init(); err(%d)", err);
		return err;
	}

	// AMD's dma-buf export (hipMemGetHandleForAddressRange) requires the VRAM
	// range to be 2 MiB aligned/sized; rounding only to device_pagesize (4 KiB)
	// makes hipmem_heap_init() fail with -EINVAL on ROCm.
	const size_t dmabuf_gran = 2UL << 20;
	heap_size = ((heap_size + dmabuf_gran - 1) / dmabuf_gran) * dmabuf_gran;

	err = hipmem_heap_init(&g_upcie_hip_rte.hip_heap, heap_size, &g_upcie_hip_rte.hip_config);
	if (err) {
		XNVME_DEBUG("FAILED: hipmem_heap_init(); err(%d)", err);
		return -ENOMEM;
	}

	g_upcie_hip_rte.is_initialized = 1;

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
			"FAILED: unsupported driver '%s', upcie-hip requires 'uio_pci_generic'",
			driver_name);
		return -ENOTSUP;
	}

	return 0;
}

/**
 * Initialize the NVMe controller for a uPCIe HIP device.
 *
 * Validates that the device is bound to uio_pci_generic (vfio-pci is not
 * supported for HIP P2P DMA) and delegates to xnvme_be_upcie_ctrlr_init,
 * which opens the NVMe controller and allocates the host hugepage runtime.
 * HIP runtime initialization is done in dev_open.
 */
void *
xnvme_be_upcie_hip_ctrlr_init(struct xnvme_dev *dev)
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
xnvme_be_upcie_hip_ctrlr_term(void *handle)
{
	return xnvme_be_upcie_ctrlr_term(handle);
}

/**
 * Open a uPCIe HIP device handle.
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
 *  - Data buffers (xnvme_buf_alloc) are allocated from the HIP device heap
 *    (g_upcie_hip_rte).  The NVMe controller accesses these directly via
 *    PCIe P2P DMA, bypassing host DRAM entirely.
 *
 * Consequently, both the host hugepage runtime and the HIP device heap are
 * initialized from the deploy's host_heap_size/device_heap_size when the first
 * upcie-hip device is opened.
 */
static int
xnvme_be_upcie_hip_dev_open(struct xnvme_dev *dev)
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

	err = _hip_rte_init(dev->opts.device_heap_size);
	if (err) {
		XNVME_DEBUG("FAILED: _hip_rte_init(); err(%d)", err);
		return err;
	}

	atomic_fetch_add(&g_hip_ctrlr_count, 1);
	return 0;
}

static void
xnvme_be_upcie_hip_dev_close(struct xnvme_dev *dev)
{
	if (atomic_fetch_sub(&g_hip_ctrlr_count, 1) == 1) {
		_hip_rte_term();
	}
	xnvme_be_upcie_dev_close(dev);
}

#endif

struct xnvme_be_dev g_xnvme_be_upcie_hip_dev = {
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
	.dev_open = xnvme_be_upcie_hip_dev_open,
	.dev_close = xnvme_be_upcie_hip_dev_close,
	.id = "upcie-hip",
	.ctrlr_init = xnvme_be_upcie_hip_ctrlr_init,
	.ctrlr_term = xnvme_be_upcie_hip_ctrlr_term,
#else
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};
