// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * dmamem constructors: wrap an existing hostmem_hugepage
 * ======================================================
 *
 * The hostmem side already owns hugepage allocation (hostmem_hugepage)
 * and the buffer allocator (hostmem_heap). What the vfio + iommufd
 * world needs on top is a single contiguous IOVA mapping over the whole
 * hostmem region so submission computes PRPs as
 * iova = base_iova + (va - hostmem_va); the uio + iommu=pt/off world
 * needs a translator that returns the enumerated PA of the
 * hosting hugepage plus the intra-hugepage offset. Both live behind the
 * same dmamem_va_to_iova call, discriminated by the translator field.
 *
 * Three flavours, one per (engine, translator) shape:
 *
 *   dmamem_from_hostmem_iommufd(...) uses IOMMU_IOAS_MAP against the
 *   hostmem VA range and records the kernel-picked base IOVA.
 *   translator = ARITHMETIC.
 *
 *   dmamem_from_hostmem_type1(...) uses VFIO_IOMMU_MAP_DMA against a
 *   caller-chosen base IOVA (type1 has no "kernel picks" mode).
 *   translator = ARITHMETIC.
 *
 *   dmamem_from_hostmem_registry(...) installs no mapping. Adopts the
 *   hugepage's per-page addresses into a registry so PRPs are PAs. Used
 *   with uio_pci_generic + iommu=pt/off. translator = LUT.
 *
 * All three carry owned=0 so dmamem_destroy() only undoes the DMA
 * mapping (or nothing, for the LUT case); the hostmem_hugepage stays
 * with its original owner.
 *
 * @file dmamem_hostmem.h
 * @version 0.7.0
 */

/**
 * Wrap an existing hostmem_hugepage as a dmamem via iommufd.
 *
 * Installs one IOMMU_IOAS_MAP over the hugepage VA range and records the
 * kernel-picked base IOVA in dmem->base_iova.
 */
static inline int
dmamem_from_hostmem_iommufd(struct dmamem *dmem, struct iommufd *iommufd,
			    struct hostmem_hugepage *hp)
{
	int err;

	if (!dmem || !iommufd || iommufd->fd < 0 || !hp || !hp->virt || !hp->size) {
		return -EINVAL;
	}

	memset(dmem, 0, sizeof(*dmem));
	dmem->fd = -1;
	dmem->cpu_va = hp->virt;
	dmem->base_va = dmem->cpu_va;
	dmem->size = hp->size;
	dmem->iommufd = iommufd;
	dmem->backing = DMAMEM_BACKING_HOSTMEM;
	dmem->translator = DMAMEM_XLATE_ARITHMETIC;
	dmem->owned = 0;

	err = iommufd_ioas_map(iommufd, (uint64_t)(uintptr_t)dmem->cpu_va, dmem->size,
			       IOMMU_IOAS_MAP_READABLE | IOMMU_IOAS_MAP_WRITEABLE,
			       &dmem->base_iova);
	if (err) {
		UPCIE_DEBUG("FAILED: iommufd_ioas_map(); err(%d)", err);
		memset(dmem, 0, sizeof(*dmem));
		dmem->fd = -1;
		return err;
	}

	return 0;
}

/**
 * Wrap an existing hostmem_hugepage as a dmamem via a vfio type1
 * container.
 *
 * Installs one VFIO_IOMMU_MAP_DMA over the hugepage VA range at the
 * caller-chosen base IOVA. type1 has no "kernel picks IOVA" mode.
 */
static inline int
dmamem_from_hostmem_type1(struct dmamem *dmem, struct vfio_container *container, uint64_t base_iova,
			  struct hostmem_hugepage *hp)
{
	struct vfio_iommu_type1_dma_map map = {0};
	int err;

	if (!dmem || !container || !hp || !hp->virt || !hp->size) {
		return -EINVAL;
	}

	memset(dmem, 0, sizeof(*dmem));
	dmem->fd = -1;
	dmem->cpu_va = hp->virt;
	dmem->base_va = dmem->cpu_va;
	dmem->size = hp->size;
	dmem->base_iova = base_iova;
	dmem->vfio_container = container;
	dmem->backing = DMAMEM_BACKING_HOSTMEM;
	dmem->translator = DMAMEM_XLATE_ARITHMETIC;
	dmem->owned = 0;

	map.argsz = sizeof(map);
	map.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE;
	map.vaddr = (uintptr_t)dmem->cpu_va;
	map.iova = base_iova;
	map.size = dmem->size;
	if (vfio_iommu_map_dma(container, &map) < 0) {
		err = -errno;
		UPCIE_DEBUG("FAILED: vfio_iommu_map_dma(); errno(%d)", errno);
		memset(dmem, 0, sizeof(*dmem));
		dmem->fd = -1;
		return err;
	}

	return 0;
}

/**
 * Wrap an existing hostmem_hugepage as a registry-translating dmamem.
 *
 * For the uio_pci_generic + iommu=pt/off case, where the device consumes
 * physical addresses. The hugepage's per-page table is adopted at the hugepage
 * granularity, so translation is one absolute-indexed load.
 *
 * No populate callback, so it adopts and never discovers: further host memory
 * via dmamem_register() would mean reading /proc/self/pagemap, which this does
 * not do.
 *
 * `va_bits` bounds the LUT reservation to that much address space; 0 selects
 * the default, which covers the 47-bit user range. Lower it where the
 * reservation is not affordable, under `ulimit -v` or `vm.overcommit_memory=2`,
 * bearing in mind that a region mapped above the bound cannot be registered.
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
dmamem_from_hostmem_registry(struct dmamem *dmem, struct hostmem_hugepage *hp, int va_bits)
{
	int shift, err;

	if (!dmem || !hp || !hp->virt || !hp->size || !hp->phys_lut || !hp->config) {
		return -EINVAL;
	}

	shift = dmamem_lut_pagesize_shift(hp->config->hugepgsz);
	if (shift < 0) {
		UPCIE_DEBUG("FAILED: unsupported hugepgsz(%zu)", hp->config->hugepgsz);
		return -EINVAL;
	}

	memset(dmem, 0, sizeof(*dmem));

	err = dmamem_registry_init(&dmem->registry, hp->config->hugepgsz, va_bits, NULL, NULL,
				   NULL, NULL);
	if (err) {
		UPCIE_DEBUG("FAILED: dmamem_registry_init(); err(%d)", err);
		return err;
	}

	err = dmamem_registry_adopt(&dmem->registry, hp->virt, hp->size, hp->phys_lut, shift,
				    NULL);
	if (err) {
		UPCIE_DEBUG("FAILED: dmamem_registry_adopt(); err(%d)", err);
		dmamem_registry_term(&dmem->registry);
		return err;
	}

	dmem->fd = -1;
	dmem->cpu_va = hp->virt;
	dmem->base_va = dmem->cpu_va;
	dmem->size = hp->size;
	dmem->backing = DMAMEM_BACKING_HOSTMEM;
	dmem->translator = DMAMEM_XLATE_LUT;
	dmem->owned = 0;

	return 0;
}
