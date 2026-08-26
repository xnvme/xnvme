// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * dmamem constructor: wrap an existing hipmem_heap (LUT translator)
 * =================================================================
 *
 * Closes the "CPU-init + HIP VRAM + iommu=pt/off" cell of the grid by
 * borrowing an already-populated hipmem_heap. The heap has already
 * called hipMalloc for the device VA range,
 * hipMemGetHandleForAddressRange to export as a dma-buf,
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
 * @file dmamem_hip.h
 * @version 0.8.0
 */

/**
 * Granularity for a HIP-backed dmamem_registry.
 *
 * Deliberately not the device's alloc_granularity, which ROCm reports as 4096.
 * That number describes allocation, not physical layout: no run in a measured
 * export was shorter than 4 MiB, so 2 MiB windows never straddle a jump, while
 * sizing the LUT by 4096 would reserve 512 GiB of address space against 1 GiB
 * at 2 MiB. See `tools/upcie_dmabuf_probe_hip`.
 */
#define DMAMEM_HIP_REGISTRY_GRANULARITY (2UL << 20)

/**
 * Recover the HIP allocation that `va` falls inside, for a dmamem_registry.
 *
 * An export describes an allocation, not a range, so a registration has to be
 * placed at its offset within one. `ctx` is unused; the runtime knows.
 *
 * @return 0 on success, -EINVAL when `va` is not a known device address.
 */
static inline int
dmamem_hip_registry_range(void *UPCIE_UNUSED(ctx), uint64_t va, uint64_t *base_out,
			  size_t *size_out)
{
	void *b = NULL;
	hipError_t cr = hipMemGetAddressRange((hipDeviceptr_t *)&b, size_out, (hipDeviceptr_t)va);

	if (cr != hipSuccess) {
		UPCIE_DEBUG("FAILED: hipMemGetAddressRange(0x%" PRIx64 "), cr: %d", va, cr);
		return -EINVAL;
	}
	*base_out = (uint64_t)b;

	return 0;
}

/**
 * Make one HIP allocation addressable, for a dmamem_registry.
 *
 * Exports the whole allocation as a dma-buf once, attaches it, and summarises
 * the scatter list into one address per granule. Why the allocation rather than
 * the registered range is in dmamem_registry.h, under Backings.
 *
 * @return 0 on success, negative errno on failure. -EOPNOTSUPP when a granule
 *         turns out not to be contiguous.
 */
static inline int
dmamem_hip_registry_populate(void *ctx, uint64_t base, size_t size, uint64_t granularity,
			     uint64_t *lut_out, size_t nlut, struct dmabuf *attach_out)
{
	struct hipmem_config *config = ctx;
	const size_t pagesize = (size_t)config->pagesize;
	/* The export needs a page-aligned length; a runtime-reported size need
	 * not be one. Rounding up stays inside the page-backed allocation. */
	const size_t export_nbytes = (size + pagesize - 1) & ~(pagesize - 1);
	struct dmabuf attach = {0};
	int dmabuf_fd = -1;
	int err;
	hipError_t cr;

	cr = hipMemGetHandleForAddressRange(&dmabuf_fd, (hipDeviceptr_t)base, export_nbytes,
					    hipMemRangeHandleTypeDmaBufFd, 0);
	if (cr != hipSuccess) {
		UPCIE_DEBUG("FAILED: hipMemGetHandleForAddressRange(0x%" PRIx64 ", %zu), cr: %d",
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
 * Release one allocation made addressable by dmamem_hip_registry_populate().
 */
static inline void
dmamem_hip_registry_release(void *UPCIE_UNUSED(ctx), struct dmabuf *attach)
{
	dmabuf_import_detach(attach);
}

/**
 * Build a registry-translating dmamem around a hipmem_heap.
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
/**
 * Build a dmamem for a HIP heap that a device reaches through an IOMMU
 *
 * The registry constructor resolves to physical addresses, which is what a
 * device consumes with the IOMMU out of the way and nothing a device behind
 * one can use. This exports the heap and maps it into the address space the
 * device translates through instead.
 *
 * As of writing IOMMU_IOAS_MAP_FILE refuses dma-bufs exported by GPU runtimes,
 * so this returns -ENOTSUP on current kernels. It is written anyway: the path
 * is where it belongs, the failure names the call that refuses, and the day
 * that call accepts one, nothing here has to change.
 *
 * @param dmem Pre-allocated dmamem to fill
 * @param heap A HIP heap from hipmem_heap_init
 * @param iommufd The address space the device is attached to
 *
 * @return 0 on success, negative errno on failure
 */
static inline int
dmamem_from_hip_iommufd(struct dmamem *dmem, struct hipmem_heap *heap, struct iommufd *iommufd)
{
	int dmabuf_fd = -1;
	hipError_t cr;
	int err;

	if (!dmem || !heap || !iommufd) {
		return -EINVAL;
	}

	cr = hipMemGetHandleForAddressRange(&dmabuf_fd, (hipDeviceptr_t)heap->vaddr, heap->size,
					    hipMemRangeHandleTypeDmaBufFd, 0);
	if (cr != hipSuccess) {
		UPCIE_DEBUG("FAILED: hipMemGetHandleForAddressRange(); hipError_t(%d)", cr);
		return -EIO;
	}

	err = dmamem_from_dmabuf(dmem, iommufd, dmabuf_fd, heap->size);
	close(dmabuf_fd);
	if (err) {
		UPCIE_DEBUG("FAILED: dmamem_from_dmabuf(hip heap); err(%d)", err);
		return err;
	}

	dmem->backing = DMAMEM_BACKING_HIPMEM;

	return 0;
}

static inline int
dmamem_from_hip_registry(struct dmamem *dmem, struct hipmem_heap *heap, int va_bits)
{
	int err;

	if (!dmem || !heap || !heap->phys_lut || !heap->config) {
		return -EINVAL;
	}

	memset(dmem, 0, sizeof(*dmem));

	err = dmamem_registry_init(&dmem->registry, DMAMEM_HIP_REGISTRY_GRANULARITY, va_bits,
				   dmamem_hip_registry_range, dmamem_hip_registry_populate,
				   dmamem_hip_registry_release, heap->config);
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
	dmem->backing = DMAMEM_BACKING_HIPMEM;
	dmem->translator = DMAMEM_XLATE_LUT;
	dmem->owned = 0;

	return 0;
}
