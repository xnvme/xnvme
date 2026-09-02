// SPDX-License-Identifier: BSD-3-Clause

/**
 * dmamem registry decorator: address device memory through iommu-map-pa
 * =====================================================================
 *
 * The device-memory registry backends (dmamem_cuda.h, dmamem_hip.h) put
 * physical addresses in the LUT. A device consumes those directly with
 * iommu=pt/off, and cannot use them at all once an IOMMU translates for it.
 *
 * iommufd would be the place to install the missing translation, but as of
 * kernel 6.19 IOMMU_IOAS_MAP_FILE rejects the dma-bufs CUDA and HIP export
 * (see iommufd.h). The out-of-tree iommu-map-pa module inserts the addresses
 * into the target device's domain instead; experimental/iommu_map_pa/README.md
 * covers why that is a proof of concept and not a safe API.
 *
 * This header decorates any registry backend: the inner populate enumerates as
 * before, then the mapping is installed and the LUT is rewritten to hold the
 * IOVAs. Translation is unchanged, so a backend serves both worlds with one
 * implementation: dmamem_from_cuda_registry() for iommu=pt/off,
 * dmamem_from_cuda_iommu_map_pa() for an enforcing IOMMU.
 *
 * A decorated LUT therefore holds IOVAs rather than the physical addresses
 * DMAMEM_XLATE_LUT usually implies; enum dmamem_translator says so too.
 *
 * IOVA window
 * -----------
 *
 * The module maps at an IOVA the caller picks without telling the domain's
 * owner, so overlapping what vfio or iommufd hands out later corrupts one of
 * the two mappings silently. The caller reserves a window and keeps the owner
 * out of it: dmamem_iommu_map_pa_reserve_window() under iommufd, or by
 * convention a window above installed RAM under a type1 container, which maps
 * host memory at iova == phys. Allocation within the window is first-fit, so a
 * register/unregister cycle reuses what it freed.
 *
 * ==========================================================================
 * EXPERIMENTAL DEPENDENCY
 * Requires the out-of-tree iommu-map-pa DKMS module, as
 * <upcie/experimental/iommu_map_pa.h> does. Without it every mapping attempt
 * fails with -ENOTSUP; UPCIE_HAVE_IOMMU_MAP_PA tells you which case you got.
 * ==========================================================================
 *
 * @file dmamem_iommu_map_pa.h
 * @version 0.8.0
 */
/** Room for "0000:00:00.0", matching the ioctl ABI's field. */
#define DMAMEM_IOMMU_MAP_PA_BDF_LEN 16

/**
 * Largest number of IOVA ranges an IOAS is expected to report.
 *
 * The count follows the reserved regions carved out of the aperture rather than
 * the number of attached devices, and those regions largely coincide, so it
 * stays small as devices are added. An IOAS reporting more ranges than this
 * fails the query with -EMSGSIZE and the count needed; nothing is truncated.
 */
#define DMAMEM_IOMMU_MAP_PA_MAX_IOVA_RANGES 16

/**
 * One installed mapping.
 *
 * Keyed by dma-buf fd because that is all release() receives to find it by.
 */
struct dmamem_iommu_map_pa_mapping {
	int dmabuf_fd;      ///< Key; the fd inner populate() attached to
	uint64_t iova_base; ///< Where in the window the mapping was placed
	uint64_t nbytes;    ///< Length of the mapping in bytes
	uint64_t map_handle;                      ///< Handle for IOMMU_UNMAP_PA
	struct dmamem_iommu_map_pa_mapping *next; ///< List linkage; owned here
};

/**
 * A decorated registry backend plus the iommu-map-pa state it needs.
 *
 * Serves one target device, and may back several dmamems, which all resolve
 * through the one inner backend bound on first use. It must outlive all of them:
 * releasing a backing unmaps through this handle.
 */
struct dmamem_iommu_map_pa {
	int fd;                                ///< /dev/iommu_map_pa; -1 when closed
	char bdf[DMAMEM_IOMMU_MAP_PA_BDF_LEN]; ///< Device whose domain is mapped into
	uint64_t window_base;                  ///< First IOVA this may hand out
	uint64_t window_size;                  ///< Length of the window in bytes
	dmamem_registry_range_fn range;        ///< Inner backend; may be NULL
	dmamem_registry_populate_fn populate;  ///< Inner backend, bound on first use
	dmamem_registry_release_fn release;    ///< Inner backend; may be NULL
	void *ctx;                             ///< Inner backend context; not owned
	struct dmamem_iommu_map_pa_mapping *maps; ///< Owned list of live mappings
};

/**
 * Open the helper and claim an IOVA window on it.
 *
 * The inner backend is left unset; a flavour constructor such as
 * dmamem_from_cuda_iommu_map_pa() fills it in.
 *
 * @param imp         Caller-allocated handle to initialise
 * @param bdf         Target device in "0000:bb:dd.f" form
 * @param window_base First IOVA the decorator may hand out; non-zero, since 0
 *                    means "no room left"
 * @param window_size Length of the window in bytes
 *
 * @return 0 on success, negative errno on failure. -ENOTSUP when built without
 *         the iommu-map-pa UAPI, -ENOENT when the module is not loaded.
 */
static inline int
dmamem_iommu_map_pa_open(struct dmamem_iommu_map_pa *imp, const char *bdf, uint64_t window_base,
			 uint64_t window_size)
{
	int fd;

	if (!imp || !bdf || !window_base || !window_size) {
		return -EINVAL;
	}
	if (strlen(bdf) >= sizeof(imp->bdf)) {
		UPCIE_DEBUG("FAILED: bdf('%s') does not fit the ioctl ABI", bdf);
		return -EINVAL;
	}
	if ((window_base + window_size) < window_base) {
		return -EOVERFLOW;
	}

	memset(imp, 0, sizeof(*imp));
	imp->fd = -1;

	fd = iommu_map_pa_open();
	if (fd < 0) {
		UPCIE_DEBUG("FAILED: iommu_map_pa_open(); err(%d)", fd);
		return fd;
	}

	imp->fd = fd;
	strncpy(imp->bdf, bdf, sizeof(imp->bdf) - 1);
	imp->window_base = window_base;
	imp->window_size = window_size;

	return 0;
}

/**
 * Close the helper.
 *
 * Destroy every dmamem built on it first, since closing the fd drops the
 * mappings. Any still recorded here are reported.
 */
static inline void
dmamem_iommu_map_pa_close(struct dmamem_iommu_map_pa *imp)
{
	struct dmamem_iommu_map_pa_mapping *next;

	if (!imp) {
		return;
	}

	for (struct dmamem_iommu_map_pa_mapping *m = imp->maps; m; m = next) {
		UPCIE_DEBUG("FAILED: closing with a live mapping at iova(0x%" PRIx64
			    "); destroy the dmamem first",
			    m->iova_base);
		next = m->next;
		free(m);
	}
	imp->maps = NULL;

	if (imp->fd >= 0) {
		iommu_map_pa_close(imp->fd);
	}

	memset(imp->bdf, 0, sizeof(imp->bdf));
	imp->fd = -1;
	imp->window_base = 0;
	imp->window_size = 0;
}

/**
 * Keep the IOAS out of the decorator's IOVA window.
 *
 * Allows every IOVA the IOAS can use except the window.
 *
 * Call it after the target device is attached: before that the kernel does not
 * know the device's reserved regions, and an allowed range overlapping one
 * makes the attach fail with -EADDRINUSE.
 *
 * iommufd only. A type1 container has no equivalent; see the IOVA window notes
 * above.
 *
 * @param imp Open handle whose window is to be reserved
 * @param ctx iommufd handle with the target device attached to its IOAS
 *
 * @return 0 on success, negative errno on failure. -ERANGE when the window does
 *         not fit inside one usable range, or leaves the IOAS nothing.
 */
static inline int
dmamem_iommu_map_pa_reserve_window(struct dmamem_iommu_map_pa *imp, struct iommufd *ctx)
{
	struct iommu_iova_range usable[DMAMEM_IOMMU_MAP_PA_MAX_IOVA_RANGES];
	/* Twice as many: carving the window out can split a range in two. */
	struct iommu_iova_range allow[DMAMEM_IOMMU_MAP_PA_MAX_IOVA_RANGES * 2];
	uint32_t nusable = 0, nallow = 0;
	uint64_t win_start, win_last;
	int contained = 0;
	int err;

	if (!imp || !ctx || (imp->fd < 0) || !imp->window_size) {
		return -EINVAL;
	}

	win_start = imp->window_base;
	win_last = imp->window_base + imp->window_size - 1;

	err = iommufd_ioas_iova_ranges(ctx, usable, DMAMEM_IOMMU_MAP_PA_MAX_IOVA_RANGES, &nusable,
				       NULL);
	if (err) {
		UPCIE_DEBUG("FAILED: iommufd_ioas_iova_ranges(); err(%d); needed(%u)", err,
			    nusable);
		return err;
	}

	for (uint32_t i = 0; i < nusable; ++i) {
		const uint64_t start = usable[i].start;
		const uint64_t last = usable[i].last;

		if ((last < win_start) || (start > win_last)) {
			allow[nallow].start = start;
			allow[nallow].last = last;
			++nallow;
			continue;
		}

		/* Overlap is not enough: the part of the window outside this range
		 * is either a device's reserved region or past the aperture, and
		 * mapping into it fails later, far from here. */
		if ((start <= win_start) && (last >= win_last)) {
			contained = 1;
		}

		if (start < win_start) {
			allow[nallow].start = start;
			allow[nallow].last = win_start - 1;
			++nallow;
		}
		if (last > win_last) {
			allow[nallow].start = win_last + 1;
			allow[nallow].last = last;
			++nallow;
		}
	}

	if (!contained) {
		UPCIE_DEBUG("FAILED: window [0x%" PRIx64 "..0x%" PRIx64
			    "] does not fit inside a usable IOVA range",
			    win_start, win_last);
		return -ERANGE;
	}

	if (!nallow) {
		UPCIE_DEBUG("FAILED: window [0x%" PRIx64 "..0x%" PRIx64
			    "] leaves the IOAS no IOVAs at all",
			    win_start, win_last);
		return -ERANGE;
	}

	err = iommufd_ioas_allow_iovas(ctx, allow, nallow);
	if (err) {
		UPCIE_DEBUG("FAILED: iommufd_ioas_allow_iovas(%u); err(%d)", nallow, err);
		return err;
	}

	return 0;
}

/**
 * Find room for `nbytes` in the window, first-fit over the live mappings.
 *
 * @return The base IOVA, or 0 when the window has no room.
 */
static inline uint64_t
dmamem_iommu_map_pa_window_alloc(struct dmamem_iommu_map_pa *imp, uint64_t nbytes, uint64_t align)
{
	uint64_t cand = (imp->window_base + align - 1) & ~(align - 1);
	int moved = 1;

	while (moved) {
		moved = 0;
		for (struct dmamem_iommu_map_pa_mapping *m = imp->maps; m; m = m->next) {
			if ((cand < (m->iova_base + m->nbytes)) &&
			    ((cand + nbytes) > m->iova_base)) {
				cand = (m->iova_base + m->nbytes + align - 1) & ~(align - 1);
				moved = 1;
			}
		}
	}

	if ((cand < imp->window_base) ||
	    ((cand + nbytes) > (imp->window_base + imp->window_size))) {
		return 0;
	}

	return cand;
}

/**
 * Recover the allocation `va` falls inside, for a dmamem_registry.
 *
 * The registry's ctx is the handle, so the inner backend's own is unwrapped
 * here, as it is for populate and release. A backend without a range leaves the
 * outputs as the registry seeded them, which is what a NULL range means.
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
dmamem_iommu_map_pa_range(void *ctx, uint64_t va, uint64_t *base_out, size_t *size_out)
{
	struct dmamem_iommu_map_pa *imp = ctx;

	if (!imp) {
		return -EINVAL;
	}

	if (!imp->range) {
		return 0;
	}

	return imp->range(imp->ctx, va, base_out, size_out);
}

/**
 * Make one allocation addressable through the IOMMU, for a dmamem_registry.
 *
 * @return 0 on success, negative errno on failure. -ENOSPC when the window has
 *         no room for the allocation.
 */
static inline int
dmamem_iommu_map_pa_populate(void *ctx, uint64_t base, size_t size, uint64_t granularity,
			     uint64_t *lut_out, size_t nlut, struct dmabuf *attach_out)
{
	struct dmamem_iommu_map_pa *imp = ctx;
	struct dmamem_iommu_map_pa_mapping *m;
	uint64_t nbytes = (uint64_t)nlut * granularity;
	uint64_t iova_base;
	int err;

	if (!imp || (imp->fd < 0) || !imp->populate) {
		return -EINVAL;
	}
	if ((granularity > UINT32_MAX) || (nlut > UINT32_MAX)) {
		UPCIE_DEBUG("FAILED: granularity(%" PRIu64 ") or nlut(%zu) exceeds the ioctl ABI",
			    granularity, nlut);
		return -EOVERFLOW;
	}

	m = calloc(1, sizeof(*m));
	if (!m) {
		return -ENOMEM;
	}

	err = imp->populate(imp->ctx, base, size, granularity, lut_out, nlut, attach_out);
	if (err) {
		UPCIE_DEBUG("FAILED: inner populate(0x%" PRIx64 ", %zu); err(%d)", base, size, err);
		free(m);
		return err;
	}

	iova_base = dmamem_iommu_map_pa_window_alloc(imp, nbytes, granularity);
	if (!iova_base) {
		UPCIE_DEBUG("FAILED: no room for %" PRIu64 " bytes in the window at 0x%" PRIx64,
			    nbytes, imp->window_base);
		err = -ENOSPC;
		goto err_release;
	}

	/* The dma-buf fd goes along so the module holds a reference on the
	 * exporter for as long as the mapping lives. */
	err = iommu_map_pa_add(imp->fd, imp->bdf, attach_out->fd, iova_base, (uint32_t)granularity,
			       (uint32_t)nlut, lut_out,
			       IOMMU_MAP_PA_PROT_READ | IOMMU_MAP_PA_PROT_WRITE, &m->map_handle);
	if (err) {
		UPCIE_DEBUG("FAILED: iommu_map_pa_add(iova 0x%" PRIx64 ", %" PRIu64 "); err(%d)",
			    iova_base, nbytes, err);
		goto err_release;
	}

	/* The device now reaches these pages by IOVA, not by the address the
	 * mapping points at. */
	for (size_t i = 0; i < nlut; ++i) {
		lut_out[i] = iova_base + (uint64_t)i * granularity;
	}

	m->dmabuf_fd = attach_out->fd;
	m->iova_base = iova_base;
	m->nbytes = nbytes;
	m->next = imp->maps;
	imp->maps = m;

	return 0;

err_release:
	if (imp->release) {
		imp->release(imp->ctx, attach_out);
	}
	free(m);

	return err;
}

/**
 * Release one allocation made addressable by dmamem_iommu_map_pa_populate().
 */
static inline void
dmamem_iommu_map_pa_release(void *ctx, struct dmabuf *attach)
{
	struct dmamem_iommu_map_pa *imp = ctx;
	struct dmamem_iommu_map_pa_mapping **prev;

	if (!imp || !attach) {
		return;
	}

	/* Unmap before letting the exporter go: the module holds the pages
	 * through the dma-buf, and the domain must not outlive them. */
	prev = &imp->maps;
	for (struct dmamem_iommu_map_pa_mapping *m = *prev; m; prev = &m->next, m = m->next) {
		int err;

		if (m->dmabuf_fd != attach->fd) {
			continue;
		}

		err = iommu_map_pa_del(imp->fd, m->map_handle);
		if (err) {
			/* The module may still have it mapped, so the IOVA cannot go
			 * back to the window: something placed there next would land
			 * on top of a live mapping. Keep the record to hold the range,
			 * and clear its key so no later release matches it. */
			UPCIE_DEBUG("FAILED: iommu_map_pa_del(iova 0x%" PRIx64
				    "); err(%d); leaving the window reserved",
				    m->iova_base, err);
			m->dmabuf_fd = -1;
			break;
		}

		*prev = m->next;
		free(m);
		break;
	}

	if (imp->release) {
		imp->release(imp->ctx, attach);
	}
}

/**
 * Build a registry-translating dmamem whose backings are mapped for the IOMMU.
 *
 * The generic half of the flavour constructors. A flavour passes its backend
 * here, then fills in `dmem->base_va`, `dmem->size` and `dmem->backing` after.
 *
 * The backend is bound to the handle on first use, since that is what the
 * decorator's populate reaches through. Every dmamem on one handle therefore
 * shares it, and binding a second, different one is refused rather than applied
 * under the dmamems already resolving through the first.
 *
 * The dmamem owns the registry; dmamem_destroy() unmaps every backing. `imp` is
 * borrowed and must outlive it. `va_bits` bounds the LUT reservation, as in
 * dmamem_from_cuda_registry().
 *
 * @return 0 on success, negative errno on failure. -EBUSY when the handle is
 *         already bound to a different backend.
 */
static inline int
dmamem_from_iommu_map_pa(struct dmamem *dmem, struct dmamem_iommu_map_pa *imp, size_t granularity,
			 int va_bits, dmamem_registry_range_fn range,
			 dmamem_registry_populate_fn populate, dmamem_registry_release_fn release,
			 void *ctx)
{
	int err;

	if (!dmem || !imp || (imp->fd < 0) || !populate) {
		return -EINVAL;
	}

	if (imp->populate && (imp->ctx != ctx)) {
		UPCIE_DEBUG("FAILED: handle already bound to a different inner backend");
		return -EBUSY;
	}

	memset(dmem, 0, sizeof(*dmem));

	err = dmamem_registry_init(&dmem->registry, granularity, va_bits,
				   dmamem_iommu_map_pa_range, dmamem_iommu_map_pa_populate,
				   dmamem_iommu_map_pa_release, imp);
	if (err) {
		UPCIE_DEBUG("FAILED: dmamem_registry_init(); err(%d)", err);
		return err;
	}

	imp->range = range;
	imp->populate = populate;
	imp->release = release;
	imp->ctx = ctx;

	dmem->fd = -1;
	dmem->cpu_va = NULL;
	dmem->translator = DMAMEM_XLATE_LUT;
	dmem->owned = 0;

	return 0;
}
