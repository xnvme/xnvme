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
 * needs a translator that returns the borrowed phys_lut PA of the
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
 *   dmamem_from_hostmem_lut(...) installs no mapping. Borrows the
 *   hugepage's phys_lut for translation so PRPs are PAs. Used with
 *   uio_pci_generic + iommu=pt/off. translator = LUT.
 *
 * All three carry owned=0 so dmamem_destroy() only undoes the DMA
 * mapping (or nothing, for the LUT case); the hostmem_hugepage stays
 * with its original owner.
 *
 * @file dmamem_hostmem.h
 * @version 0.5.2
 */

/**
 * Wrap an existing hostmem_hugepage as a dmamem via iommufd.
 *
 * Installs one IOMMU_IOAS_MAP over the hugepage VA range and records the
 * kernel-picked base IOVA in dmem->base_iova.
 */
static inline int
dmamem_from_hostmem_iommufd(struct dmamem *dmem, struct iommufd *iommufd, uint32_t ioas_id,
			    struct hostmem_hugepage *hp)
{
	int err;

	if (!dmem || !iommufd || iommufd->fd < 0 || !hp || !hp->virt || !hp->size) {
		return -EINVAL;
	}

	memset(dmem, 0, sizeof(*dmem));
	dmem->fd = -1;
	dmem->cpu_va = hp->virt;
	dmem->size = hp->size;
	dmem->iommufd = iommufd;
	dmem->ioas_id = ioas_id;
	dmem->backing = DMAMEM_BACKING_HOSTMEM;
	dmem->translator = DMAMEM_XLATE_ARITHMETIC;
	dmem->owned = 0;

	err = iommufd_ioas_map(iommufd, ioas_id, (uint64_t)(uintptr_t)dmem->cpu_va, dmem->size,
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
 * Wrap an existing hostmem_hugepage as a LUT-translator dmamem.
 *
 * For uio_pci_generic + iommu=pt / iommu=off: no IOMMU mapping is
 * installed and DMA addresses are physical, resolved at submission
 * through the hugepage's phys_lut. No ioctl here, nothing for
 * dmamem_destroy to undo; a pure translator adapter over borrowed
 * state. Requires hp->phys_lut != NULL, which means the pagemap read
 * at hostmem_hugepage_alloc time succeeded (i.e. the process has
 * CAP_SYS_ADMIN).
 */
static inline int
dmamem_from_hostmem_lut(struct dmamem *dmem, struct hostmem_hugepage *hp)
{
	int shift;

	if (!dmem || !hp || !hp->virt || !hp->size || !hp->phys_lut || !hp->config) {
		return -EINVAL;
	}

	shift = dmamem_lut_pagesize_shift(hp->config->hugepgsz);
	if (shift < 0) {
		UPCIE_DEBUG("FAILED: unsupported hugepgsz(%zu)", hp->config->hugepgsz);
		return -EINVAL;
	}

	memset(dmem, 0, sizeof(*dmem));
	dmem->fd = -1;
	dmem->cpu_va = hp->virt;
	dmem->size = hp->size;
	dmem->backing = DMAMEM_BACKING_HOSTMEM;
	dmem->translator = DMAMEM_XLATE_LUT;
	dmem->phys_lut = hp->phys_lut;
	dmem->hugepgsz = hp->config->hugepgsz;
	dmem->hugepgsz_shift = shift;
	dmem->owned = 0;

	return 0;
}
