// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * dmamem constructor: wrap an existing cudamem_heap (LUT translator)
 * ==================================================================
 *
 * Closes the "CPU-init + CUDA VRAM + iommu=pt/off" cell of the grid by
 * borrowing an already-populated cudamem_heap. The heap has already
 * done the heavy lifting: cuMemAlloc for the device VA range,
 * cuMemGetHandleForAddressRange to export the range as a dma-buf,
 * dmabuf_attach + dmabuf_get_lut to enumerate per-device-page PAs into
 * heap->phys_lut. This constructor just points the LUT-translator
 * fields on struct dmamem at the already-populated table and marks the
 * dmamem as wrapping (owned=0) so destroy does not touch the heap's
 * lifetime.
 *
 * The heap's device VA is NOT CPU-mappable; dmem->cpu_va is left NULL,
 * and callers compose PRPs via dmamem_offset_to_iova() with offsets
 * measured from heap->vaddr.
 *
 * @file dmamem_cuda.h
 * @version 0.5.2
 */

/**
 * Wrap an existing cudamem_heap as a LUT-translator dmamem.
 *
 * The cudamem_heap must have been initialised via cudamem_heap_init so
 * heap->phys_lut is already populated. No new CUDA calls are made
 * here; the dmamem borrows the LUT and every dmamem_offset_to_iova
 * lands as heap->phys_lut[offset >> shift] + intra-page offset.
 *
 * @param dmem  Pre-allocated dmamem descriptor to fill.
 * @param heap  Initialised cudamem_heap to borrow.
 *
 * @return 0 on success, negative errno on error.
 */
static inline int
dmamem_from_cuda_lut(struct dmamem *dmem, struct cudamem_heap *heap)
{
	int shift;

	if (!dmem || !heap || !heap->phys_lut || !heap->config) {
		return -EINVAL;
	}

	shift = dmamem_lut_pagesize_shift(heap->config->device_pagesize);
	if (shift < 0) {
		UPCIE_DEBUG("FAILED: unsupported device_pagesize(%zu)",
			    (size_t)heap->config->device_pagesize);
		return -EINVAL;
	}

	memset(dmem, 0, sizeof(*dmem));
	dmem->fd = -1;
	dmem->cpu_va = NULL; /* Device VA; not CPU-mappable. Callers use offsets. */
	dmem->size = heap->size;
	dmem->backing = DMAMEM_BACKING_CUDAMEM;
	dmem->translator = DMAMEM_XLATE_LUT;
	dmem->phys_lut = heap->phys_lut;
	dmem->hugepgsz = heap->config->device_pagesize;
	dmem->hugepgsz_shift = shift;
	dmem->owned = 0;

	return 0;
}
