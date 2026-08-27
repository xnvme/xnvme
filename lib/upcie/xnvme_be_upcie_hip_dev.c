// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <stdatomic.h>
#include <stdlib.h>
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

	/* Physical addresses read the same from every controller, so one table
	 * serves them all; per-domain IOVAs do not. */
	if (!xnvme_be_upcie_gpu_map_required()) {
		err = dmamem_from_hip_registry(&g_upcie_hip_rte.dmem, &g_upcie_hip_rte.hip_heap,
					       xnvme_be_upcie_va_bits());
		if (err) {
			XNVME_DEBUG("FAILED: dmamem_from_hip_registry(); err(%d)", err);
			hipmem_heap_term(&g_upcie_hip_rte.hip_heap);
			return err;
		}
	}

	g_upcie_hip_rte.is_initialized = 1;

	return 0;
}

/** Heap bytes to map, rounded as the registry rounds a registration */
static uint64_t
_hip_slice_span(const struct hipmem_heap *heap)
{
	const uint64_t gran = DMAMEM_HIP_REGISTRY_GRANULARITY;

	return ((heap->size + gran - 1) & ~(gran - 1)) + gran;
}

/** Point the device at the runtime's table, or build it one of its own */
static int
_hip_dev_dmem_init(struct xnvme_dev *dev)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	struct xnvme_be_upcie_gpu_dmem *gpu;
	int err;

	if (!xnvme_be_upcie_gpu_map_required()) {
		state->dmem = &g_upcie_hip_rte.dmem;
		return 0;
	}

	gpu = calloc(1, sizeof(*gpu));
	if (!gpu) {
		return -ENOMEM;
	}

	err = xnvme_be_upcie_gpu_map_open(&gpu->map, dev->ident.uri,
					  _hip_slice_span(&g_upcie_hip_rte.hip_heap));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_gpu_map_open(%s); err(%d)", dev->ident.uri,
			    err);
		free(gpu);
		return err;
	}

	err = dmamem_from_hip_iommu_map_pa(&gpu->dmem, &g_upcie_hip_rte.hip_heap,
					   xnvme_be_upcie_va_bits(), &gpu->map.imp);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_from_hip_iommu_map_pa(); err(%d)", err);
		xnvme_be_upcie_gpu_map_close(&gpu->map);
		free(gpu);
		return err;
	}

	state->gpu = gpu;
	state->dmem = &gpu->dmem;

	return 0;
}

static void
_hip_dev_dmem_term(struct xnvme_dev *dev)
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
	err = _hip_dev_dmem_init(dev);
	if (err) {
		XNVME_DEBUG("FAILED: _hip_dev_dmem_init(); err(%d)", err);
		if (!atomic_load(&g_hip_ctrlr_count)) {
			_hip_rte_term();
		}
		return err;
	}

	atomic_fetch_add(&g_hip_ctrlr_count, 1);
	return 0;
}

static void
xnvme_be_upcie_hip_dev_close(struct xnvme_dev *dev)
{
	_hip_dev_dmem_term(dev);

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
	.ctrlr_init = xnvme_be_upcie_ctrlr_init,
	.ctrlr_term = xnvme_be_upcie_ctrlr_term,
#else
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};
