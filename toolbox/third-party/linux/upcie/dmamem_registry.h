// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Registry of externally-provided DMA-able regions
 * ================================================
 *
 * Where a dmamem describes one contiguous range, a registry describes an
 * arbitrary number of them, so a caller can hand over memory it already owns.
 *
 * Lookup is one load regardless of how many regions are registered:
 *
 *     chunk_idx = vaddr >> gran_shift
 *     phys      = lut_phys[chunk_idx] + (vaddr & gran_mask)
 *
 * `lut_phys` spans the whole chunk_idx range and is MAP_NORESERVE, so resident
 * cost tracks live chunks rather than virtual capacity. A registry belongs to
 * one dmamem, so a buffer must be registered into each one it is used from.
 *
 * Backings
 * --------
 *
 * What the device can address is the allocation a registration falls inside,
 * not the registration. ROCm accepts and discards the range arguments to an
 * export and returns the whole buffer object, so exporting per registration
 * re-exports the allocation and resolves a sub-range to the allocation base.
 * CUDA honours the range. Since the ROCm failure is silent, both use the shape
 * that is correct on both: recover the enclosing allocation, populate it once,
 * and let overlapping registrations share it, refcounted.
 * `tools/upcie_dmabuf_probe_{cuda,hip}` measures this.
 *
 * Sizing
 * ------
 *
 * The reservation is `(1 << va_bits) / granularity` slots of eight bytes, so it
 * scales inversely with granularity: at the default 47-bit VA, 512 MiB for a
 * 2 MiB granularity but 256 GiB for a 4 KiB one. Pass a smaller `va_bits`
 * bounded by the range actually used.
 *
 * Adoption
 * --------
 *
 * `dmamem_registry_adopt()` takes addresses a caller already knows, such as a
 * heap that enumerated them at init. An adopted backing is borrowed and never
 * released by the registry.
 *
 * @file dmamem_registry.h
 * @version 0.9.0
 */

/**
 * Default width of the address space the table spans, in bits.
 *
 * 47 is the whole user range on x86-64 without `la57`, device pointers
 * included, since unified virtual addressing carves them from the same space.
 * Under `la57` the range widens to 56 bits and both 47 and 48 rely on Linux
 * handing out addresses below 2^47 unless asked otherwise; a registration
 * above the span fails loudly at dmamem_registry_add().
 *
 * Lowering it shrinks the reservation but narrows what can be translated, and
 * the fast path bounds-checks by assert only: above the span is an
 * out-of-range read in a release build, not the zero that means unregistered.
 */
#define DMAMEM_REGISTRY_VA_BITS 47

/**
 * Recover the allocation that `va` falls inside.
 *
 * May be NULL, in which case a registration is taken to be its own allocation.
 * The base returned must be aligned to the registry's granularity, since chunk
 * indices are absolute and the LUT is filled from the base outwards.
 *
 * @return 0 on success, negative errno on failure.
 */
typedef int (*dmamem_registry_range_fn)(void *ctx, uint64_t va, uint64_t *base_out,
					size_t *size_out);

/**
 * Make an allocation addressable and fill one LUT entry per granule.
 *
 * Called once per backing. `lut_out` has `nlut` entries covering `[base, base +
 * nlut * granularity)`; anything to undo later goes in `attach_out`. On failure
 * both outputs are left untouched.
 *
 * @return 0 on success, negative errno on failure.
 */
typedef int (*dmamem_registry_populate_fn)(void *ctx, uint64_t base, size_t size,
					   uint64_t granularity, uint64_t *lut_out, size_t nlut,
					   struct dmabuf *attach_out);

/**
 * Undo what populate did for one backing. May be NULL when nothing is owned.
 */
typedef void (*dmamem_registry_release_fn)(void *ctx, struct dmabuf *attach);

/**
 * One allocation, shared by every registration that falls inside it.
 */
struct dmamem_registry_backing {
	uint64_t base;        ///< Allocation base; aligned to the granularity
	size_t size;          ///< Allocation length in bytes
	uint32_t rc;          ///< Registrations referring to this backing
	uint32_t borrowed;    ///< 1: addresses were adopted, there is nothing to release
	struct dmabuf attach; ///< Owned by the registry when !borrowed
	struct dmamem_registry_backing *next; ///< List linkage owned by the registry
};

/**
 * One registration, so removal can find the backing to drop.
 */
struct dmamem_registry_registration {
	uint64_t vaddr;                            ///< Start of the registered range
	size_t size;                               ///< Length of the registered range in bytes
	struct dmamem_registry_backing *backing;   ///< Allocation it falls inside; not owned
	struct dmamem_registry_registration *next; ///< List linkage owned by the registry
};

/**
 * A registry is embedded in the dmamem that owns it, rather than pointed at,
 * so translation reads its fields at a fixed offset instead of chasing a
 * pointer first. The three the hot path touches lead, to keep them on the same
 * cache line as the dmamem fields around them; everything below is cold.
 *
 * Registration and removal are not thread-safe; a consumer that performs them
 * from more than one thread serialises them itself, as it does for the other
 * memory abstractions here. Translation is lock-free regardless, reading one
 * table slot, so the IO path is unaffected. The callbacks run underneath
 * registration, so none of them may register, remove, or translate through
 * this registry.
 */
struct dmamem_registry {
	uint64_t *lut_phys;                       ///< chunk_idx -> chunk base address; mmap-backed
	int gran_shift;                           ///< log2(granularity)
	uint64_t gran_mask;                       ///< granularity - 1, for the intra-chunk offset
	size_t lut_capacity;                      ///< Number of slots in the LUT
	struct dmamem_registry_backing *backings; ///< Owned list of backings
	struct dmamem_registry_registration *list; ///< Owned list of registrations
	dmamem_registry_range_fn range;            ///< Recovers an allocation; may be NULL
	dmamem_registry_populate_fn populate; ///< Makes an allocation addressable; may be NULL
	dmamem_registry_release_fn release;   ///< Undoes populate; may be NULL
	void *ctx;                            ///< Passed to the callbacks; not owned
};

/**
 * Initialize a registry.
 *
 * Reserves the demand-paged LUT; no physical memory is committed until
 * something is registered. Every allocation registered must be contiguous in
 * bus-address terms across `granularity`, which populate verifies.
 *
 * @param registry    Caller-allocated registry to initialise
 * @param granularity Chunk size in bytes; a power of two
 * @param va_bits     Width of the address range to cover; 0 selects the default
 * @param range       Recovers the allocation a pointer falls inside; may be NULL
 * @param populate    Makes an allocation addressable; NULL for adopt-only
 * @param release     Undoes populate; NULL when nothing is owned
 * @param ctx         Opaque flavour context handed to the callbacks
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
dmamem_registry_init(struct dmamem_registry *registry, size_t granularity, int va_bits,
		     dmamem_registry_range_fn range, dmamem_registry_populate_fn populate,
		     dmamem_registry_release_fn release, void *ctx)
{
	size_t phys_bytes;
	int gran_shift = 0;

	if (!registry || !granularity || (granularity & (granularity - 1))) {
		return -EINVAL;
	}

	while (((size_t)1 << gran_shift) < granularity) {
		++gran_shift;
	}

	if (!va_bits) {
		va_bits = DMAMEM_REGISTRY_VA_BITS;
	}
	if ((va_bits <= gran_shift) || (va_bits > 64) || ((va_bits - gran_shift) >= 64)) {
		return -EINVAL;
	}

	memset(registry, 0, sizeof(*registry));

	registry->gran_shift = gran_shift;
	registry->gran_mask = (uint64_t)granularity - 1;
	registry->lut_capacity = (size_t)1 << (va_bits - gran_shift);
	registry->range = range;
	registry->populate = populate;
	registry->release = release;
	registry->ctx = ctx;

	phys_bytes = registry->lut_capacity * sizeof(*registry->lut_phys);
	registry->lut_phys = mmap(NULL, phys_bytes, PROT_READ | PROT_WRITE,
				  MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (registry->lut_phys == MAP_FAILED) {
		UPCIE_DEBUG("FAILED: mmap(lut_phys, %zu); errno: %d", phys_bytes, errno);
		registry->lut_phys = NULL;
		return -ENOMEM;
	}

	return 0;
}

/**
 * Number of chunks a backing occupies in the LUT.
 */
static inline size_t
dmamem_registry_backing_nlut(struct dmamem_registry *registry, size_t size)
{
	return (size + registry->gran_mask) >> registry->gran_shift;
}

/**
 * Clear the table slots an allocation occupies.
 *
 * A slot carrying an address no registration owns resolves to a plausible
 * wrong target, where an untouched one resolves to the zero read as an error.
 */
static inline void
dmamem_registry_lut_clear(struct dmamem_registry *registry, uint64_t base, size_t nlut)
{
	memset(&registry->lut_phys[base >> registry->gran_shift], 0,
	       nlut * sizeof(*registry->lut_phys));
}

/**
 * Drop a reference on a backing, releasing it when the last one goes.
 */
static inline void
dmamem_registry_backing_deref(struct dmamem_registry *registry,
			      struct dmamem_registry_backing *backing)
{
	struct dmamem_registry_backing **prev = &registry->backings;

	if (!backing || !backing->rc) {
		return;
	}

	backing->rc--;
	if (backing->rc) {
		return;
	}

	dmamem_registry_lut_clear(registry, backing->base,
				  dmamem_registry_backing_nlut(registry, backing->size));

	if (!backing->borrowed && registry->release) {
		registry->release(registry->ctx, &backing->attach);
	}

	for (struct dmamem_registry_backing *b = *prev; b; prev = &b->next, b = b->next) {
		if (b == backing) {
			*prev = b->next;
			break;
		}
	}
	free(backing);
}

/**
 * Find the backing covering `[base, base + size)`, if one is already live.
 *
 * A range straddling two backings, or matching one only partially, is not a
 * match: that means the runtime reported inconsistent allocations, and reusing
 * the wrong one is how a DMA ends up in the wrong place.
 */
static inline struct dmamem_registry_backing *
dmamem_registry_backing_find(struct dmamem_registry *registry, uint64_t base, size_t size)
{
	for (struct dmamem_registry_backing *b = registry->backings; b; b = b->next) {
		if ((base >= b->base) && ((base + size) <= (b->base + b->size))) {
			return b;
		}
	}

	return NULL;
}

/**
 * Fill the LUT for an adopted range, checking each granule is contiguous.
 *
 * Only every `granularity >> lut_shift`-th entry reaches the LUT, but the
 * skipped ones are verified contiguous: otherwise `base + offset` resolves
 * inside a granule to an address the caller never gave.
 *
 * @return 0 on success, -EOPNOTSUPP when a granule is not contiguous.
 */
static inline int
dmamem_registry_adopt_fill(struct dmamem_registry *registry, uint64_t base, size_t size,
			   const uint64_t *lut, int lut_shift, size_t nlut)
{
	uint64_t *dst = &registry->lut_phys[base >> registry->gran_shift];
	const size_t fine_step = (size_t)1 << lut_shift;
	const size_t fine_per_gran = (size_t)1 << (registry->gran_shift - lut_shift);
	const size_t nfine = (size + fine_step - 1) >> lut_shift;

	for (size_t k = 0; k < nlut; ++k) {
		const size_t first = k * fine_per_gran;
		size_t span = nfine - first;

		if (span > fine_per_gran) {
			span = fine_per_gran;
		}

		for (size_t i = 1; i < span; ++i) {
			if (lut[first + i] != lut[first] + (uint64_t)i * fine_step) {
				UPCIE_DEBUG(
					"FAILED: adopted granule(%zu) not contiguous at i(%zu)", k,
					i);
				return -EOPNOTSUPP;
			}
		}

		dst[k] = lut[first];
	}

	return 0;
}

/**
 * Whether an allocation partially overlaps one already known.
 *
 * Two backings sharing a granule would each claim it, the second overwriting
 * the first, and releasing either would clear a granule the other still owns.
 * A range inside a live backing is fine and is what the refcount is for, since
 * the lookup reuses the backing enclosing it. Everything else is refused: a
 * partial overlap, and a range enclosing a live backing, which cannot share it
 * because the backing covers only part of the range.
 */
static inline int
dmamem_registry_backing_overlaps(struct dmamem_registry *registry, uint64_t base, size_t size)
{
	for (struct dmamem_registry_backing *b = registry->backings; b; b = b->next) {
		const int disjoint = ((base + size) <= b->base) || (base >= (b->base + b->size));
		const int inside = (base >= b->base) && ((base + size) <= (b->base + b->size));

		if (!disjoint && !inside) {
			return 1;
		}
	}

	return 0;
}

/**
 * Shared body of add/adopt: attach the range to a backing, populating or
 * adopting one when it is not already live, and record the registration.
 *
 * When `adopt_lut` is non-NULL the addresses are taken from it, indexed by
 * (chunk_va - base) >> adopt_shift, and the backing is marked borrowed.
 *
 * A range inside a live backing refcounts it and does not read the caller's
 * table, so a second caller describing the same allocation differently is
 * ignored rather than merged.
 */
static inline int
dmamem_registry_add_impl(struct dmamem_registry *registry, void *vaddr, size_t nbytes,
			 const uint64_t *adopt_lut, int adopt_shift,
			 struct dmamem_registry_registration **out)
{
	struct dmamem_registry_registration *m = NULL;
	struct dmamem_registry_backing *backing = NULL;
	uint64_t va, base;
	size_t size, nlut;
	int err;

	if (!registry || !registry->lut_phys || !vaddr || !nbytes) {
		return -EINVAL;
	}

	va = (uint64_t)vaddr;
	base = va;
	size = nbytes;

	if (!adopt_lut && registry->range) {
		err = registry->range(registry->ctx, va, &base, &size);
		if (err) {
			UPCIE_DEBUG("FAILED: range(0x%" PRIx64 "); err(%d)", va, err);
			return err;
		}
		if ((va < base) || ((va + nbytes) > (base + size))) {
			UPCIE_DEBUG("FAILED: range(0x%" PRIx64 ", %zu) outside allocation", va,
				    nbytes);
			return -EINVAL;
		}
	}

	if (base & registry->gran_mask) {
		UPCIE_DEBUG("FAILED: allocation base(0x%" PRIx64 ") is not granule aligned", base);
		return -EOPNOTSUPP;
	}

	nlut = dmamem_registry_backing_nlut(registry, size);
	if (((base >> registry->gran_shift) + nlut) > registry->lut_capacity) {
		UPCIE_DEBUG("FAILED: allocation exceeds LUT capacity; raise va_bits at init");
		return -EINVAL;
	}

	if (dmamem_registry_backing_overlaps(registry, base, size)) {
		UPCIE_DEBUG("FAILED: allocation(0x%" PRIx64 ", %zu) partially overlaps another",
			    base, size);
		return -EINVAL;
	}

	m = calloc(1, sizeof(*m));
	if (!m) {
		return -ENOMEM;
	}

	backing = dmamem_registry_backing_find(registry, base, size);
	if (!backing) {
		backing = calloc(1, sizeof(*backing));
		if (!backing) {
			free(m);
			return -ENOMEM;
		}
		backing->base = base;
		backing->size = size;

		if (adopt_lut) {
			err = dmamem_registry_adopt_fill(registry, base, size, adopt_lut,
							 adopt_shift, nlut);
			if (err) {
				dmamem_registry_lut_clear(registry, base, nlut);
				free(backing);
				free(m);
				return err;
			}
			backing->borrowed = 1;
		} else if (!registry->populate) {
			UPCIE_DEBUG("FAILED: registry has no populate; adopt-only");
			free(backing);
			free(m);
			return -EOPNOTSUPP;
		} else {
			err = registry->populate(registry->ctx, base, size,
						 registry->gran_mask + 1,
						 &registry->lut_phys[base >> registry->gran_shift],
						 nlut, &backing->attach);
			if (err) {
				UPCIE_DEBUG("FAILED: populate(0x%" PRIx64 ", %zu); err(%d)", base,
					    size, err);
				dmamem_registry_lut_clear(registry, base, nlut);
				free(backing);
				free(m);
				return err;
			}
		}

		backing->next = registry->backings;
		registry->backings = backing;
	}

	backing->rc++;

	m->vaddr = va;
	m->size = nbytes;
	m->backing = backing;
	m->next = registry->list;
	registry->list = m;

	if (out) {
		*out = m;
	}

	return 0;
}

/**
 * Register a range, discovering the allocation it belongs to.
 *
 * `vaddr` and `nbytes` may have any alignment; what must be granule-aligned is
 * the enclosing allocation, which the caller does not choose. Consumers may
 * impose more, e.g. NVMe PRP construction wants host-page-aligned buffers.
 *
 * @param registry Registry initialised with dmamem_registry_init()
 * @param vaddr    Start of the range to register
 * @param nbytes   Length of the range in bytes
 * @param out      Receives the registration; may be NULL
 *
 * @return 0 on success, negative errno on failure. -EINVAL when the allocation
 *         exceeds the LUT capacity chosen at init.
 */
static inline int
dmamem_registry_add(struct dmamem_registry *registry, void *vaddr, size_t nbytes,
		    struct dmamem_registry_registration **out)
{
	if (!registry) {
		return -EINVAL;
	}

	return dmamem_registry_add_impl(registry, vaddr, nbytes, NULL, 0, out);
}

/**
 * Register a range whose addresses the caller already knows.
 *
 * `lut` holds addresses from `vaddr`, which must be granule-aligned, one entry
 * per `1 << lut_shift` bytes. Nothing is discovered or released; the caller
 * keeps ownership and must outlive the registration.
 *
 * A LUT finer than the granularity is sampled, not checked, so the caller
 * asserts its region is contiguous across each granule.
 *
 * @param registry  Registry initialised with dmamem_registry_init()
 * @param vaddr     Start of the range, granule-aligned
 * @param nbytes    Length of the range in bytes
 * @param lut       Addresses covering the range, one per `1 << lut_shift` bytes
 * @param lut_shift log2 of the bytes each `lut` entry covers
 * @param out       Receives the registration; may be NULL
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
dmamem_registry_adopt(struct dmamem_registry *registry, void *vaddr, size_t nbytes,
		      const uint64_t *lut, int lut_shift,
		      struct dmamem_registry_registration **out)
{
	if (!registry || !lut || lut_shift > registry->gran_shift) {
		return -EINVAL;
	}

	/* Absolute chunk indices, so the caller's LUT only lines up from a
	 * boundary. */
	if ((uint64_t)vaddr & registry->gran_mask) {
		UPCIE_DEBUG("FAILED: vaddr(%p) is not granule aligned", vaddr);
		return -EINVAL;
	}

	return dmamem_registry_add_impl(registry, vaddr, nbytes, lut, lut_shift, out);
}

/**
 * Remove the registration starting at `vaddr`.
 *
 * @param registry Registry the range was registered with
 * @param vaddr    Start of the registered range
 *
 * @return 0 on success, -EINVAL when no registration starts there.
 */
static inline int
dmamem_registry_remove(struct dmamem_registry *registry, void *vaddr)
{
	if (!registry) {
		return -EINVAL;
	}

	const uint64_t key = (uint64_t)vaddr;
	int err = -EINVAL;

	for (struct dmamem_registry_registration **prev = &registry->list, *m = registry->list; m;
	     prev = &m->next, m = m->next) {
		if (m->vaddr != key) {
			continue;
		}

		*prev = m->next;
		dmamem_registry_backing_deref(registry, m->backing);
		free(m);
		err = 0;
		break;
	}

	return err;
}

/**
 * Drop every registration, releasing the backings they held. The LUT
 * reservation stays, so the registry remains usable.
 *
 * @param registry Registry to empty
 */
static inline void
dmamem_registry_clear(struct dmamem_registry *registry)
{
	struct dmamem_registry_registration *next;

	if (!registry) {
		return;
	}

	for (struct dmamem_registry_registration *m = registry->list; m; m = next) {
		next = m->next;
		dmamem_registry_backing_deref(registry, m->backing);
		free(m);
	}
	registry->list = NULL;
}

/**
 * Tear down a registry, releasing every backing and the LUT reservation.
 *
 * The registry itself is not freed; it is the caller's, wherever it was placed.
 *
 * @param registry Registry to tear down
 */
static inline void
dmamem_registry_term(struct dmamem_registry *registry)
{
	if (!registry) {
		return;
	}

	dmamem_registry_clear(registry);

	if (registry->lut_phys) {
		munmap(registry->lut_phys, registry->lut_capacity * sizeof(*registry->lut_phys));
		registry->lut_phys = NULL;
	}
	registry->lut_capacity = 0;
}

/**
 * Whether `[virt, virt + nbytes)` lies inside a live registration.
 *
 * Walks the backings, so cold-path only. Translation cannot answer it: the
 * table is per granule, so an address in the unused remainder of a granule
 * resolves to a plausible wrong address rather than to zero.
 *
 * @param registry Registry to ask
 * @param virt     Start of the range
 * @param nbytes   Length of the range in bytes
 *
 * @return 1 when contained, 0 otherwise.
 */
static inline int
dmamem_registry_contains(struct dmamem_registry *registry, void *virt, size_t nbytes)
{
	const uint64_t va = (uint64_t)virt;

	if (!registry || !virt || !nbytes) {
		return 0;
	}

	for (struct dmamem_registry_registration *m = registry->list; m; m = m->next) {
		if ((va >= m->vaddr) && ((va + nbytes) <= (m->vaddr + m->size))) {
			return 1;
		}
	}

	return 0;
}
