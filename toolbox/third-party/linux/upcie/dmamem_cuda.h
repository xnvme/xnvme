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
 * dmabuf_import_attach + dmabuf_get_lut to enumerate per-device-page PAs into
 * heap->phys_lut. This constructor just points the LUT-translator
 * fields on struct dmamem at the already-populated table and marks the
 * dmamem as wrapping (owned=0) so destroy does not touch the heap's
 * lifetime.
 *
 * The heap's device VA is NOT CPU-mappable; dmem->cpu_va is left NULL,
 * and callers compose PRPs via dmamem_offset_to_iova() with offsets
 * measured from heap->vaddr.
 *
 * dmamem_from_cuda_iommu_map_pa() covers the neighbouring cell, where an IOMMU
 * translates for the device. It reuses the backend below and differs only in
 * installing a mapping per allocation; see
 * <upcie/dmamem_iommu_map_pa.h>.
 *
 * @file dmamem_cuda.h
 * @version 0.8.0
 */

/**
 * Granularity for a CUDA-backed dmamem_registry.
 *
 * Matches the device's alloc_granularity, the BAR1 large-page size, which is
 * 2 MiB on the parts uPCIe targets. Sizing the LUT by it costs 512 MiB of
 * reservation over the default 47-bit address space.
 */
#define DMAMEM_CUDA_REGISTRY_GRANULARITY (2UL << 20)

/**
 * Recover the mapping that `va` falls inside, for a dmamem_registry.
 *
 * An export describes an allocation, not a range, so a registration has to be
 * placed at its offset within one. The allocation a runtime reports is the
 * caller's suballocation, whose base is arbitrary, while the LUT is indexed by
 * granule; asking for the mapping instead gives the block the driver placed
 * that suballocation in, which is granule aligned and shared by everything
 * packed into it. `ctx` is unused; the runtime knows.
 *
 * @return 0 on success, -EINVAL when `va` is not a known device address.
 */
static inline int
dmamem_cuda_registry_range(void *UPCIE_UNUSED(ctx), uint64_t va, uint64_t *base_out,
			   size_t *size_out)
{
	const uint64_t gran = DMAMEM_CUDA_REGISTRY_GRANULARITY;
	CUdeviceptr b = 0;
	size_t size = 0;
	CUresult cr;

#if CUDA_VERSION >= 11030
	if ((cuPointerGetAttribute(&b, CU_POINTER_ATTRIBUTE_MAPPING_BASE_ADDR, (CUdeviceptr)va) ==
	     CUDA_SUCCESS) &&
	    (cuPointerGetAttribute(&size, CU_POINTER_ATTRIBUTE_MAPPING_SIZE, (CUdeviceptr)va) ==
	     CUDA_SUCCESS)) {
		*base_out = (uint64_t)b;
		*size_out = size;

		return 0;
	}
#endif

	/* Widen to the enclosing allocation: a sub-granule buffer has no
	 * aligned base of its own to register. */
	cr = cuMemGetAddressRange(&b, &size, (CUdeviceptr)va);
	if (cr != CUDA_SUCCESS) {
		UPCIE_DEBUG("FAILED: cuMemGetAddressRange(0x%" PRIx64 "), cr: %d", va, cr);
		return -EINVAL;
	}

	*base_out = (uint64_t)b & ~(gran - 1);
	size += (uint64_t)b - *base_out;
	*size_out = (size + gran - 1) & ~(gran - 1);

	return 0;
}

/**
 * Make one CUDA allocation addressable, for a dmamem_registry.
 *
 * Exports the whole allocation as a dma-buf once, attaches it, and summarises
 * the scatter list into one address per granule. Why the allocation rather than
 * the registered range is in dmamem_registry.h, under Backings.
 *
 * @return 0 on success, negative errno on failure. -EOPNOTSUPP when a granule
 *         turns out not to be contiguous.
 */
static inline int
dmamem_cuda_registry_populate(void *ctx, uint64_t base, size_t size, uint64_t granularity,
			      uint64_t *lut_out, size_t nlut, struct dmabuf *attach_out)
{
	struct cudamem_config *config = ctx;
	const size_t pagesize = (size_t)config->pagesize;
	/* The export needs a page-aligned length; a runtime-reported size need
	 * not be one. Rounding up stays inside the page-backed allocation. */
	const size_t export_nbytes = (size + pagesize - 1) & ~(pagesize - 1);
	struct dmabuf attach = {0};
	int dmabuf_fd = -1;
	int err;
	CUresult cr;

	cr = cuMemGetHandleForAddressRange(&dmabuf_fd, (CUdeviceptr)base, export_nbytes,
					   CU_MEM_RANGE_HANDLE_TYPE_DMA_BUF_FD, 0);
	if (cr != CUDA_SUCCESS) {
		UPCIE_DEBUG("FAILED: cuMemGetHandleForAddressRange(0x%" PRIx64 ", %zu), cr: %d",
			    base, export_nbytes, cr);
		return -EIO;
	}

	/* NOTE: EXPERIMENTAL dependency, see <upcie/experimental/dmabuf_import.h> */
	err = dmabuf_import_attach(dmabuf_fd, &attach);
	if (err) {
		UPCIE_DEBUG("FAILED: dmabuf_import_attach(), err: %d", err);
		close(dmabuf_fd);
		return err;
	}

	err = dmabuf_get_granule_lut(&attach, lut_out, nlut, granularity);
	if (err) {
		UPCIE_DEBUG("FAILED: dmabuf_get_granule_lut(), err: %d", err);
		dmabuf_import_detach(&attach);
		return err;
	}

	*attach_out = attach;

	return 0;
}

/**
 * Release one allocation made addressable by dmamem_cuda_registry_populate().
 */
static inline void
dmamem_cuda_registry_release(void *UPCIE_UNUSED(ctx), struct dmabuf *attach)
{
	dmabuf_import_detach(attach);
}

/**
 * Build a registry-translating dmamem around a cudamem_heap.
 *
 * The dmamem owns the registry: it is initialised here, seeded with the heap,
 * and torn down by dmamem_destroy(). The heap is adopted rather than
 * rediscovered, since it enumerated its own addresses at init, and it then
 * resolves through the same translator as every buffer handed over with
 * dmamem_register(). That is what lets one dmamem serve both, so the command
 * paths need no notion of where a buffer came from.
 *
 * The heap is borrowed and must outlive the dmamem.
 *
 * `va_bits` bounds the LUT reservation to that much address space; 0 selects
 * the default, which covers the 47-bit user range. Lower it where the
 * reservation is not affordable, under `ulimit -v` or `vm.overcommit_memory=2`,
 * bearing in mind that a region mapped above the bound cannot be registered.
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
dmamem_from_cuda_registry(struct dmamem *dmem, struct cudamem_heap *heap, int va_bits)
{
	int err;

	if (!dmem || !heap || !heap->phys_lut || !heap->config) {
		return -EINVAL;
	}

	memset(dmem, 0, sizeof(*dmem));

	err = dmamem_registry_init(&dmem->registry, DMAMEM_CUDA_REGISTRY_GRANULARITY, va_bits,
				   dmamem_cuda_registry_range, dmamem_cuda_registry_populate,
				   dmamem_cuda_registry_release, heap->config);
	if (err) {
		UPCIE_DEBUG("FAILED: dmamem_registry_init(), err: %d", err);
		return err;
	}

	err = dmamem_registry_adopt(&dmem->registry, (void *)(uintptr_t)heap->vaddr, heap->size,
				    heap->phys_lut, heap->config->device_pagesize_shift, NULL);
	if (err) {
		UPCIE_DEBUG("FAILED: dmamem_registry_adopt(heap), err: %d", err);
		dmamem_registry_term(&dmem->registry);
		return err;
	}

	dmem->fd = -1;
	dmem->base_va = (void *)(uintptr_t)heap->vaddr;
	dmem->cpu_va = NULL;
	dmem->size = heap->size;
	dmem->backing = DMAMEM_BACKING_CUDAMEM;
	dmem->translator = DMAMEM_XLATE_LUT;
	dmem->owned = 0;

	return 0;
}

/**
 * Build a registry-translating dmamem around a cudamem_heap, addressed through
 * an enforcing IOMMU.
 *
 * The counterpart to dmamem_from_cuda_registry(): same backend, same
 * dmamem_register() for buffers handed over later, except that each allocation
 * is also inserted into the device's IOMMU domain and the LUT holds the
 * resulting IOVAs. See <upcie/dmamem_iommu_map_pa.h>.
 *
 * dmamem_from_cuda_registry() adopts the heap's existing address table. This
 * cannot: that table holds physical addresses, which is what the IOMMU rejects.
 * The heap is registered instead, so every backing gets a mapping and an IOVA.
 *
 * `imp` is borrowed and must be closed only after dmamem_destroy().
 *
 * @param dmem    Pre-allocated dmamem descriptor to fill
 * @param heap    Borrowed cudamem_heap; must outlive the dmamem
 * @param va_bits Bounds the LUT reservation; 0 selects the default
 * @param imp     Open iommu-map-pa handle naming the target device
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
dmamem_from_cuda_iommu_map_pa(struct dmamem *dmem, struct cudamem_heap *heap, int va_bits,
			      struct dmamem_iommu_map_pa *imp)
{
	int err;

	if (!dmem || !heap || !heap->config || !imp) {
		return -EINVAL;
	}

	err = dmamem_from_iommu_map_pa(dmem, imp, DMAMEM_CUDA_REGISTRY_GRANULARITY, va_bits,
				       dmamem_cuda_registry_range, dmamem_cuda_registry_populate,
				       dmamem_cuda_registry_release, heap->config);
	if (err) {
		return err;
	}

	dmem->base_va = (void *)(uintptr_t)heap->vaddr;
	dmem->size = heap->size;
	dmem->backing = DMAMEM_BACKING_CUDAMEM;

	err = dmamem_register(dmem, (void *)(uintptr_t)heap->vaddr, heap->size);
	if (err) {
		UPCIE_DEBUG("FAILED: dmamem_register(heap), err: %d", err);
		dmamem_destroy(dmem);
		return err;
	}

	return 0;
}
