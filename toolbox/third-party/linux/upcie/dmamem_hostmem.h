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
 * @version 0.9.0
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
dmamem_from_hostmem_type1(struct dmamem *dmem, struct vfio_container *container,
			  uint64_t base_iova, struct hostmem_hugepage *hp)
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
/**
 * A host heap described well enough for another process to translate through it
 *
 * Physical addresses come from pagemap, which wants CAP_SYS_ADMIN, so a
 * client of memory somebody else allocated cannot read them. The server can,
 * and does so once at allocation; putting the table where the client will map
 * it anyway means the work is inherited rather than repeated, and it is what
 * lets a client be unprivileged at all on the UIO path.
 *
 * The server allocates this from the heap like anything else and names the
 * offset in the handshake, so nothing has to be reserved at a fixed place and
 * neither allocator has to know it exists.
 */
enum hostmem_shared_kind {
	HOSTMEM_SHARED_LUT = 0x1,       ///< phys[] holds a base per granule
	HOSTMEM_SHARED_ARITHMETIC = 0x2 ///< the whole region resolves from base_addr
};

struct hostmem_shared_desc {
	uint32_t version;    ///< HOSTMEM_SHARED_DESC_VERSION
	uint32_t kind;       ///< One of enum hostmem_shared_kind
	uint64_t nbytes;     ///< Size of the region described
	uint64_t base_addr;  ///< ARITHMETIC: what offset zero resolves to
	uint32_t nphys;      ///< LUT: entries in phys[]
	uint32_t gran_shift; ///< LUT: log2 of the granule each entry covers
	uint64_t phys[];     ///< LUT: base of each granule, in order
};

#define HOSTMEM_SHARED_DESC_VERSION 1U

/**
 * Where an offset into a described region lands, as the device sees it
 *
 * The server side of a registration: a client names memory by offset into a
 * region it registered, never by address, so it can only ever name its own.
 * Turning that into an address is this, and it is also where a run that no
 * single address can cover is refused.
 *
 * @param desc The description, as the registering side left it
 * @param offset Byte offset from the region's base
 * @param nbytes How much has to be addressable in one run from there
 * @param addr Set to the DMA address on success
 *
 * @return 0 on success, negative errno on failure
 */
static inline int
hostmem_shared_desc_addr(const struct hostmem_shared_desc *desc, uint64_t offset, uint64_t nbytes,
			 uint64_t *addr)
{
	uint64_t gran, within;

	if (!desc || !addr || !nbytes) {
		return -EINVAL;
	}
	if ((offset > desc->nbytes) || (nbytes > (desc->nbytes - offset))) {
		return -ERANGE;
	}

	if (HOSTMEM_SHARED_ARITHMETIC == desc->kind) {
		*addr = desc->base_addr + offset;
		return 0;
	}

	gran = (uint64_t)1 << desc->gran_shift;
	within = offset & (gran - 1);

	/* A run leaving the granule it starts in is not one address to the
	 * device, since the granule after it can sit anywhere. */
	if ((within + nbytes) > gran) {
		return -ERANGE;
	}
	if ((offset >> desc->gran_shift) >= desc->nphys) {
		return -ERANGE;
	}

	*addr = desc->phys[offset >> desc->gran_shift] + within;

	return 0;
}

/**
 * What to allocate for a description of a region with this many granules
 */
static inline size_t
hostmem_shared_desc_nbytes(size_t nphys)
{
	return sizeof(struct hostmem_shared_desc) + (nphys * sizeof(uint64_t));
}

/**
 * Fill a description from a hugepage this process allocated
 *
 * @param desc Pre-allocated description, of hostmem_shared_desc_nbytes()
 * @param hp The hugepage backing the shared region
 *
 * @return 0 on success, negative errno on error
 */
static inline int
hostmem_shared_desc_fill(struct hostmem_shared_desc *desc, const struct hostmem_hugepage *hp)
{
	int shift;

	if (!desc || !hp || !hp->phys_lut || !hp->config) {
		return -EINVAL;
	}

	shift = dmamem_lut_pagesize_shift(hp->config->hugepgsz);
	if (shift < 0) {
		return -EINVAL;
	}

	desc->version = HOSTMEM_SHARED_DESC_VERSION;
	desc->kind = HOSTMEM_SHARED_LUT;
	desc->nphys = (uint32_t)hp->nphys;
	desc->nbytes = hp->size;
	desc->gran_shift = (uint32_t)shift;
	desc->base_addr = 0;
	memcpy(desc->phys, hp->phys_lut, hp->nphys * sizeof(*desc->phys));

	return 0;
}

/**
 * Describe a region the device reaches at one base address
 *
 * Where the device translates through an address space rather than through
 * physical addresses, there is no table to carry: every offset resolves from
 * the base the region was mapped at.
 *
 * @param desc Pre-allocated description; sizeof(*desc) is enough for this kind
 * @param nbytes Size of the region
 * @param base_addr What offset zero in it resolves to for the device
 *
 * @return 0 on success, negative errno on error
 */
static inline int
hostmem_shared_desc_fill_arithmetic(struct hostmem_shared_desc *desc, size_t nbytes,
				    uint64_t base_addr)
{
	if (!desc || !nbytes) {
		return -EINVAL;
	}

	memset(desc, 0, sizeof(*desc));
	desc->version = HOSTMEM_SHARED_DESC_VERSION;
	desc->kind = HOSTMEM_SHARED_ARITHMETIC;
	desc->nbytes = nbytes;
	desc->base_addr = base_addr;

	return 0;
}

/**
 * Build a dmamem over memory another process allocated and described
 *
 * Nothing here reads pagemap: the addresses come from the description, and the
 * registry is populated for this process's own mapping, since the table is
 * indexed by address and one process's addresses are not another's. How the
 * memory was obtained does not enter into it, so host memory from a hugepage
 * and device memory from a GPU runtime arrive here the same way; what the
 * caller has to say is which it is.
 *
 * @param dmem Pre-allocated dmamem to fill
 * @param base This process's mapping of the shared region
 * @param desc The server's description, found at the offset it named
 * @param va_bits Bounds the LUT reservation; 0 selects the default
 * @param backing What the region actually is
 *
 * @return 0 on success, negative errno on failure
 */
static inline int
dmamem_from_shared(struct dmamem *dmem, void *base, const struct hostmem_shared_desc *desc,
		   int va_bits, enum dmamem_backing backing)
{
	int err;

	if (!dmem || !base || !desc) {
		return -EINVAL;
	}
	if (desc->version != HOSTMEM_SHARED_DESC_VERSION) {
		UPCIE_DEBUG("FAILED: description version(%u), expected(%u)", desc->version,
			    HOSTMEM_SHARED_DESC_VERSION);
		return -EPROTO;
	}

	memset(dmem, 0, sizeof(*dmem));

	if (desc->kind == HOSTMEM_SHARED_ARITHMETIC) {
		/* Nothing to index: the device resolves the whole region from
		 * one base, and this process's addresses do not enter into
		 * it. */
		dmem->fd = -1;
		/* Device memory has no CPU mapping to record; the caller's base
		 * is where the device's addresses start, not where a load or
		 * store would land. */
		dmem->cpu_va = (backing == DMAMEM_BACKING_HOSTMEM) ? base : NULL;
		dmem->base_va = base;
		dmem->base_iova = desc->base_addr;
		dmem->size = desc->nbytes;
		dmem->backing = backing;
		dmem->translator = DMAMEM_XLATE_ARITHMETIC;
		dmem->owned = 0;

		return 0;
	}

	err = dmamem_registry_init(&dmem->registry, (size_t)1 << desc->gran_shift, va_bits, NULL,
				   NULL, NULL, NULL);
	if (err) {
		UPCIE_DEBUG("FAILED: dmamem_registry_init(); err(%d)", err);
		return err;
	}

	err = dmamem_registry_adopt(&dmem->registry, base, desc->nbytes, desc->phys,
				    (int)desc->gran_shift, NULL);
	if (err) {
		UPCIE_DEBUG("FAILED: dmamem_registry_adopt(); err(%d)", err);
		dmamem_registry_term(&dmem->registry);
		return err;
	}

	dmem->fd = -1;
	dmem->cpu_va = (backing == DMAMEM_BACKING_HOSTMEM) ? base : NULL;
	dmem->base_va = base;
	dmem->size = desc->nbytes;
	dmem->backing = backing;
	dmem->translator = DMAMEM_XLATE_LUT;
	dmem->owned = 0;

	return 0;
}

/** Build a dmamem over shared host memory; see dmamem_from_shared() */
static inline int
dmamem_from_shared_hostmem(struct dmamem *dmem, void *base, const struct hostmem_shared_desc *desc,
			   int va_bits)
{
	return dmamem_from_shared(dmem, base, desc, va_bits, DMAMEM_BACKING_HOSTMEM);
}

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
