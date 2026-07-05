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
 *   ARITHMETIC: iova = base_iova + offset. Used whenever a real IOMMU
 *   mapping (iommufd IOAS or vfio type1 container) installed a single
 *   contiguous IOVA range for the whole dmamem. The dmamem stashes
 *   base_iova at construction and the submit path adds. No lookup.
 *
 *   LUT: iova = phys_lut[offset >> hugepgsz_shift] + (offset & mask).
 *   Used when no IOMMU mapping was installed (uio_pci_generic with
 *   iommu=pt/off) and the device consumes physical addresses directly.
 *   The dmamem stashes a borrowed per-hugepage PA table and reads it on
 *   submit. One extra load and a shift compared to the arithmetic
 *   fastpath; identical shape to the retired hostmem_dma_v2p.
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
 * @version 0.5.2
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
 * How offsets resolve to DMA addresses at submission.
 *
 * ARITHMETIC = 0 by intent: a memset-to-zero dmamem is arithmetic by
 * default, which is what every current owned constructor produces
 * without extra code.
 */
enum dmamem_translator {
	DMAMEM_XLATE_ARITHMETIC = 0x0, ///< iova = base_iova + offset
	DMAMEM_XLATE_LUT = 0x1,        ///< iova = phys_lut[off >> shift] + (off & mask)
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
	int fd;                     ///< memfd or dma-buf when owned=1; -1 when wrapping
	void *cpu_va;               ///< CPU virtual address, NULL when not mappable
	size_t size;                ///< Size in bytes
	uint64_t base_iova;         ///< Base IOVA (ARITHMETIC translator only)
	uint32_t ioas_id;           ///< IOAS the mapping lives in (iommufd only)
	struct iommufd *iommufd;    ///< Not owned; caller lifetime. NULL for type1 and LUT.
	struct vfio_container *vfio_container; ///< Not owned; caller lifetime. NULL for iommufd and LUT.
	enum dmamem_backing backing;
	enum dmamem_translator translator; ///< How offsets resolve to DMA addresses
	const uint64_t *phys_lut;   ///< Borrowed per-hugepage PA table (LUT only)
	size_t hugepgsz;            ///< Hugepage size in bytes (LUT only)
	int hugepgsz_shift;         ///< log2(hugepgsz) (LUT only)
	int owned;                  ///< 1: dmamem owns fd + cpu_va; 0: wrapping caller memory
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
	wrtn += printf("  cpu_va: %p\n", dmem->cpu_va);
	wrtn += printf("  size: %zu\n", dmem->size);
	wrtn += printf("  base_iova: 0x%" PRIx64 "\n", dmem->base_iova);
	wrtn += printf("  ioas_id: %u\n", dmem->ioas_id);
	wrtn += printf("  iommufd.fd: %d\n", dmem->iommufd ? dmem->iommufd->fd : -1);
	wrtn += printf("  vfio_container: %s\n", dmem->vfio_container ? "set" : "none");
	wrtn += printf("  backing: %d\n", dmem->backing);
	wrtn += printf("  translator: %s\n",
		       dmem->translator == DMAMEM_XLATE_LUT          ? "LUT"
		       : dmem->translator == DMAMEM_XLATE_ARITHMETIC ? "ARITHMETIC"
								     : "?");
	if (DMAMEM_XLATE_LUT == dmem->translator) {
		wrtn += printf("  phys_lut: %p\n", (void *)dmem->phys_lut);
		wrtn += printf("  hugepgsz: %zu\n", dmem->hugepgsz);
		wrtn += printf("  hugepgsz_shift: %d\n", dmem->hugepgsz_shift);
	}
	wrtn += printf("  owned: %d\n", dmem->owned);

	return wrtn;
}

/**
 * Compute log2 of a supported LUT page size.
 *
 * Accepts 4 KiB, 2 MiB, and 1 GiB; returns -1 for anything else. Used
 * by the LUT-translator importers so the fastpath does a shift + AND
 * instead of a divide + modulo.
 */
static inline int
dmamem_lut_pagesize_shift(size_t pagesize)
{
	if (pagesize == 4096) {
		return 12;
	}
	if (pagesize == 2ULL * 1024 * 1024) {
		return 21;
	}
	if (pagesize == 1024ULL * 1024 * 1024) {
		return 30;
	}
	return -1;
}

/**
 * Convert an offset inside the dmamem to an IOVA.
 *
 * The submission-path function. One compare on dmem->translator,
 * then either a single addition (ARITHMETIC) or a shift + table
 * lookup + addition (LUT).
 */
static inline uint64_t
dmamem_offset_to_iova(struct dmamem *dmem, size_t offset)
{
	if (DMAMEM_XLATE_LUT == dmem->translator) {
		return dmem->phys_lut[offset >> dmem->hugepgsz_shift] +
		       (offset & (dmem->hugepgsz - 1));
	}

	return dmem->base_iova + offset;
}

/**
 * Convert a CPU VA inside the dmamem to an IOVA.
 *
 * Only usable when the backing exposes a CPU VA. Callers must assert
 * dmem->cpu_va != NULL before invoking; the fast path does not check.
 */
static inline uint64_t
dmamem_va_to_iova(struct dmamem *dmem, void *vaddr)
{
	assert(dmem->cpu_va);
	return dmamem_offset_to_iova(dmem, (size_t)((char *)vaddr - (char *)dmem->cpu_va));
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

	if (dmem->iommufd && dmem->size) {
		int err = iommufd_ioas_unmap(dmem->iommufd, dmem->ioas_id, dmem->base_iova,
					     dmem->size);
		if (err) {
			UPCIE_DEBUG("FAILED: iommufd_ioas_unmap(); err(%d)", err);
		}
	}

	if (dmem->vfio_container && dmem->size) {
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
