// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * DMA memory abstraction
 * ======================
 *
 * A dmamem describes memory usable as a DMA source or destination,
 * regardless of which exporter produced it (memfd hugepage, CUDA VRAM,
 * HIP VRAM, libdrm BO, VFIO BAR) and regardless of which kernel API
 * installed the DMA mapping. Cousin of the kernel's dma-buf, at the
 * layer above.
 *
 * A dmamem holds an fd, an optional CPU virtual address (NULL for peer
 * memory that is not CPU-mappable), a size, and enough state to turn a
 * caller-chosen offset into the DMA address the device expects. Two
 * translation shapes live behind the same public call:
 *
 *   ARITHMETIC: iova = base_iova + offset. For a region that resolves as one
 *   contiguous range, such as an IOMMU mapping (iommufd IOAS or vfio type1
 *   container). The dmamem stashes base_iova at construction and the submit
 *   path adds. No lookup.
 *
 *   LUT: iova = lut_phys[va >> gran_shift] + (va & gran_mask), read from
 *   the registry the dmamem owns. For a region that resolves per granule, such
 *   as scattered hugepages. The dmamem stashes a borrowed table and reads
 *   it on submit. One extra load and a shift compared to the arithmetic
 *   fastpath; identical shape to hostmem_dma_v2p in hostmem_dma.h.
 *
 * Which one applies follows the shape of the region. A LUT usually holds
 * physical addresses (uio_pci_generic with iommu=pt/off), but its entries are
 * whatever the region resolves to.
 *
 * dmamem_va_to_iova branches once on the translator. When a caller
 * uses one translator throughout (which is the typical shape, e.g.
 * an xNVMe be_dmamem process runs exactly one), that branch has a
 * stable direction; branch-prediction cost is not measured here.
 *
 * Mapping context. Each dmamem stashes at most one owner pointer:
 * either an iommufd handle + IOAS id, or a legacy vfio type1 container,
 * or neither. dmamem_destroy dispatches on whichever is set (or does
 * nothing for LUT dmamems with no mapping installed).
 *
 * Constructors split into two families.
 *
 *   Owned constructors allocate the backing themselves and hold the
 *   fd / CPU view for the lifetime of the dmamem: dmamem_from_memfd()
 *   creates a hugepage-backed memfd internally; dmamem_from_dmabuf()
 *   takes an existing dma-buf fd from an exporter (VFIO BAR export,
 *   CUDA / HIP / libdrm export). Both set owned=1 so dmamem_destroy()
 *   munmaps and closes on the way out.
 *
 *   Wrapping constructors install a mapping (or install a translator
 *   without a mapping, for the LUT case) over memory the caller already
 *   owns; they set owned=0 so dmamem_destroy() only removes the mapping.
 *   dmamem_from_hostmem_iommufd(), _type1(), and _lut() live in
 *   dmamem_hostmem.h and wrap an existing hostmem_hugepage.
 *
 * @file dmamem.h
 * @version 0.9.0
 */

enum dmamem_backing {
	DMAMEM_BACKING_UNKNOWN = 0x0,
	DMAMEM_BACKING_MEMFD = 0x1,
	DMAMEM_BACKING_DMABUF = 0x2,
	DMAMEM_BACKING_HOSTMEM = 0x3, ///< wrapping an existing hostmem_hugepage
	DMAMEM_BACKING_CUDAMEM = 0x4, ///< wrapping an existing cudamem_heap
	DMAMEM_BACKING_HIPMEM = 0x5,  ///< wrapping an existing hipmem_heap
};

/**
 * How offsets resolve to the address the device puts on the bus.
 *
 * "iova" is used throughout for that address, as DPDK does. What it holds
 * depends on how the region was set up: a kernel-assigned IOVA where a mapping
 * was installed, a physical or bus address where none was. A translator says
 * only how to compute it from an offset; the two are orthogonal.
 *
 * ARITHMETIC = 0 by intent: a memset-to-zero dmamem is arithmetic by
 * default, which is what every current owned constructor produces
 * without extra code.
 *
 * LUT is indexed absolutely, so one dmamem spans every region its registry
 * holds and the fast path cannot tell a heap buffer from a registered one.
 * Resolution under it is defined only inside a registration: an unclaimed
 * granule resolves to 0, read as an error since nothing valid is based there,
 * which also makes a genuine bus address 0 unrepresentable. The unused
 * remainder of a granule claimed by a smaller allocation resolves to a wrong
 * address instead; use dmamem_registry_contains() where that distinction
 * matters.
 */
enum dmamem_translator {
	/** iova = base_iova + offset */
	DMAMEM_XLATE_ARITHMETIC = 0x0,
	/** iova = lut_phys[va >> gran_shift] + (va & gran_mask) */
	DMAMEM_XLATE_LUT = 0x1,
};

/* Forward-declare so dmamem.h stays independent of vfioctl.h include
 * order in the umbrella header. */
struct vfio_container;

/**
 * A DMA-capable memory region.
 *
 * At most one of iommufd or vfio_container is set, identifying the
 * kernel API that installed the mapping and, on destroy, the unmap
 * ioctl to invoke. LUT-translator dmamems have neither set; no mapping
 * was installed and destroy has nothing to undo.
 */
struct dmamem {
	int fd;             ///< memfd or dma-buf when owned=1; -1 when wrapping
	void *base_va;      ///< Base of the address space offsets are measured from
	void *cpu_va;       ///< CPU virtual address, NULL when not mappable
	size_t size;        ///< Size in bytes
	uint64_t base_iova; ///< Base IOVA (ARITHMETIC translator only)
	struct iommufd
		*iommufd; ///< Not owned; caller lifetime; carries the IOAS id. NULL for type1/LUT.
	struct vfio_container
		*vfio_container; ///< Not owned; caller lifetime. NULL for iommufd and LUT.
	enum dmamem_backing backing;
	enum dmamem_translator translator; ///< How offsets resolve to DMA addresses
	struct dmamem_registry registry;   ///< Owned region registry (LUT only)
	int owned; ///< 1: dmamem owns fd + cpu_va; 0: wrapping caller memory
};

/**
 * Print information about the given dmamem
 */
static inline int
dmamem_pp(struct dmamem *dmem)
{
	int wrtn = 0;

	wrtn += printf("dmamem:");

	if (!dmem) {
		wrtn += printf(" ~\n");
		return 0;
	}

	wrtn += printf("\n");
	wrtn += printf("  fd: %d\n", dmem->fd);
	wrtn += printf("  base_va: %p\n", dmem->base_va);
	wrtn += printf("  cpu_va: %p\n", dmem->cpu_va);
	wrtn += printf("  size: %zu\n", dmem->size);
	wrtn += printf("  base_iova: 0x%" PRIx64 "\n", dmem->base_iova);
	wrtn += printf("  ioas_id: %u\n", dmem->iommufd ? dmem->iommufd->ioas_id : 0);
	wrtn += printf("  iommufd.fd: %d\n", dmem->iommufd ? dmem->iommufd->fd : -1);
	wrtn += printf("  vfio_container: %s\n", dmem->vfio_container ? "set" : "none");
	wrtn += printf("  backing: %d\n", dmem->backing);
	wrtn += printf("  translator: %s\n", dmem->translator == DMAMEM_XLATE_LUT ? "LUT"
					     : dmem->translator == DMAMEM_XLATE_ARITHMETIC
						     ? "ARITHMETIC"
						     : "?");
	if (DMAMEM_XLATE_LUT == dmem->translator) {
		wrtn += printf("  lut_phys: %p\n", (void *)dmem->registry.lut_phys);
		wrtn += printf("  granularity: %" PRIu64 "\n", dmem->registry.gran_mask + 1);
		wrtn += printf("  lut_capacity: %zu\n", dmem->registry.lut_capacity);
	}
	wrtn += printf("  owned: %d\n", dmem->owned);

	return wrtn;
}

/**
 * Compute log2 of a LUT page size.
 *
 * Accepts any power-of-two page granularity of at least 4 KiB: the 4 KiB
 * base page, the 64 KiB CUDA/HIP device page, and 2 MiB / 1 GiB hugepages.
 * Returns -1 for sub-page or non-power-of-two sizes. Used by the
 * LUT-translator importers so the fastpath does a shift + AND instead of a
 * divide + modulo.
 */
static inline int
dmamem_lut_pagesize_shift(size_t pagesize)
{
	if (pagesize < 4096 || (pagesize & (pagesize - 1)) != 0) {
		return -1;
	}
	return (int)upcie_util_shift_from_size(pagesize);
}

/**
 * Forward-declared because the two are mutually recursive: a registry indexes
 * absolutely, so an offset becomes an address first, while the others measure
 * from the base and go the other way. Each call takes exactly one step.
 */
static inline uint64_t
dmamem_va_to_iova(struct dmamem *dmem, void *vaddr);

/**
 * Convert an offset inside the dmamem to an IOVA.
 *
 * The submission-path function. One compare on dmem->translator,
 * then either a single addition (ARITHMETIC) or a shift + table
 * lookup + addition (LUT).
 *
 * The result is a kernel-assigned IOVA where a mapping was installed for the
 * region and a physical or bus address where none was, whichever translator
 * computed it; see enum dmamem_translator.
 */
static inline uint64_t
dmamem_offset_to_iova(struct dmamem *dmem, size_t offset)
{
	if (DMAMEM_XLATE_LUT == dmem->translator) {
		return dmamem_va_to_iova(dmem, (char *)dmem->base_va + offset);
	}

	return dmem->base_iova + offset;
}

/**
 * Convert a CPU VA inside the dmamem to an IOVA.
 *
 * Only usable when the backing exposes a CPU VA. Callers must assert
 * dmem->base_va != NULL before invoking; the fast path does not check.
 *
 * 'vaddr' lies in whatever space the dmamem describes: a CPU mapping for
 * host memory, a device pointer for GPU memory.
 */
static inline uint64_t
dmamem_va_to_iova(struct dmamem *dmem, void *vaddr)
{
	if (DMAMEM_XLATE_LUT == dmem->translator) {
		const uint64_t va = (uint64_t)vaddr;
		uint64_t base;

		assert((va >> dmem->registry.gran_shift) < dmem->registry.lut_capacity);

		base = dmem->registry.lut_phys[va >> dmem->registry.gran_shift];

		assert(!dmem->registry.list ||
		       dmamem_registry_contains(&dmem->registry, vaddr, 1));

		return base ? base + (va & dmem->registry.gran_mask) : 0;
	}

	assert(dmem->base_va);
	return dmamem_offset_to_iova(dmem, (size_t)((char *)vaddr - (char *)dmem->base_va));
}

/**
 * Unmap the dmamem via whichever kernel API installed the mapping and,
 * if the dmamem owns its memfd + CPU view, munmap and close it.
 * Wrapping dmamems (owned=0) leave fd + cpu_va alone since another
 * owner manages their lifetime. LUT-translator dmamems have no mapping
 * to undo and (typically) do not own memory either, so destroy reduces
 * to the trailing memset.
 */
static inline void
dmamem_destroy(struct dmamem *dmem)
{
	if (!dmem) {
		return;
	}

	if (DMAMEM_XLATE_LUT == dmem->translator) {
		dmamem_registry_term(&dmem->registry);
	}

	assert(!(dmem->iommufd && dmem->vfio_container));

	if (dmem->iommufd && dmem->size) {
		int err = iommufd_ioas_unmap(dmem->iommufd, dmem->base_iova, dmem->size);
		if (err) {
			UPCIE_DEBUG("FAILED: iommufd_ioas_unmap(); err(%d)", err);
		}
	} else if (dmem->vfio_container && dmem->size) {
		struct vfio_iommu_type1_dma_unmap unmap = {0};
		unmap.argsz = sizeof(unmap);
		unmap.iova = dmem->base_iova;
		unmap.size = dmem->size;
		if (vfio_iommu_unmap_dma(dmem->vfio_container, &unmap) < 0) {
			UPCIE_DEBUG("FAILED: vfio_iommu_unmap_dma(); errno(%d)", errno);
		}
	}

	if (dmem->owned && dmem->cpu_va && dmem->size) {
		munmap(dmem->cpu_va, dmem->size);
	}

	if (dmem->owned && dmem->fd >= 0) {
		close(dmem->fd);
	}

	memset(dmem, 0, sizeof(*dmem));
	dmem->fd = -1;
}

/**
 * Hand memory the caller allocated to a registry-translating dmamem.
 *
 * The allocation `vaddr` falls inside is made addressable, once, and shared
 * with any other registration inside it. `vaddr` and `nbytes` may have any
 * byte alignment; consumers may want more, e.g. NVMe PRP construction wants
 * host-page-aligned buffers.
 *
 * Registering while I/O runs against other buffers is safe: translation reads
 * one table slot, and this writes only the slots of the allocation being
 * registered. Registering from two threads at once is not; the caller
 * serialises that.
 *
 * Registration claims whole granules, so an allocation smaller than one, or
 * one whose base is not granule aligned, is refused with -EOPNOTSUPP. Vendor
 * allocators hand out sub-granule buffers from inside a single granule, so
 * registering many small device allocations largely does not work. Note also
 * that translating an address in the unused remainder of a claimed granule
 * yields a wrong address rather than an error; do I/O only on ranges that
 * registered successfully.
 *
 * @return 0 on success, -EOPNOTSUPP when the dmamem does not translate through
 *         a registry, other negative errno on failure.
 */
static inline int
dmamem_register(struct dmamem *dmem, void *vaddr, size_t nbytes)
{
	if (!dmem || (DMAMEM_XLATE_LUT != dmem->translator)) {
		return -EOPNOTSUPP;
	}

	return dmamem_registry_add(&dmem->registry, vaddr, nbytes, NULL);
}

/**
 * Drop a registration made by dmamem_register().
 *
 * `vaddr` must be the address that was registered, not one inside the range.
 * The allocation behind it is released once the last registration referring to
 * it is gone.
 *
 * The caller must quiesce I/O over the range first. Releasing an allocation
 * tears its mapping down, so a command already submitted against it has the
 * memory pulled from under it, and the controller will either fault or, with
 * no IOMMU installed, write to a page the kernel has since reused. That is a
 * stronger requirement than freeing a heap buffer, where the memory stays
 * mapped and the damage is confined to the caller's own arena.
 *
 * @return 0 on success, -EOPNOTSUPP when the dmamem does not translate through
 *         a registry, -EINVAL when nothing was registered at `vaddr`.
 */
static inline int
dmamem_unregister(struct dmamem *dmem, void *vaddr)
{
	if (!dmem || (DMAMEM_XLATE_LUT != dmem->translator)) {
		return -EOPNOTSUPP;
	}

	return dmamem_registry_remove(&dmem->registry, vaddr);
}
