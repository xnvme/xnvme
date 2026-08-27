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

	dmamem_destroy(&g_upcie_hip_rte.dmem);
	hipmem_heap_term(&g_upcie_hip_rte.hip_heap);

	g_upcie_hip_rte.is_initialized = 0;
}

static int
_hip_rte_init(size_t heap_size, uint32_t gpu_id)
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

	err = hipSetDevice(gpu_id);
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

	/* Which addresses the controller consumes decides how the heap is
	 * described to it: physical where the IOMMU is out of the way, IOVAs
	 * where it is not. */
	if (g_upcie_rte.mode == XNVME_BE_UPCIE_MODE_UIO_LUT) {
		err = dmamem_from_hip_registry(&g_upcie_hip_rte.dmem, &g_upcie_hip_rte.hip_heap,
					       xnvme_be_upcie_va_bits());
	} else if (g_upcie_rte.mode == XNVME_BE_UPCIE_MODE_VFIO_CDEV) {
		err = dmamem_from_hip_iommufd(&g_upcie_hip_rte.dmem, &g_upcie_hip_rte.hip_heap,
					      &g_upcie_rte.cdev.iommufd);
	} else {
		err = -ENOTSUP;
	}
	if (err) {
		XNVME_DEBUG("FAILED: describing the HIP heap to the controller; err(%d)", err);
		hipmem_heap_term(&g_upcie_hip_rte.hip_heap);
		return err;
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

	/* No check on which driver is bound: whether a controller behind an
	 * IOMMU can DMA into VRAM is the kernel's answer to give, and refusing
	 * here would go on being wrong after it stops being true. */

	return 0;
}

/**
 * Initialize the NVMe controller for a uPCIe HIP device.
 *
 * Delegates to xnvme_be_upcie_ctrlr_init,
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

	err = _hip_rte_init(dev->opts.device_heap_size, dev->opts.gpu_id);
	if (err) {
		XNVME_DEBUG("FAILED: _hip_rte_init(); err(%d)", err);
		return err;
	}

	/* Data buffers live in device memory for this backend; the control path
	 * (queues, PRP lists) stays on the host heap set by the base dev_open. */
	{
		struct xnvme_be_upcie_state *state = (void *)dev->be.state;

		state->dmem = &g_upcie_hip_rte.dmem;
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
