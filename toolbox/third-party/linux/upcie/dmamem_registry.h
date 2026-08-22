// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Registry of externally-provided DMA-able regions
 * ================================================
 *
 * Where a dmamem describes one contiguous range, a registry describes an
 * arbitrary number of them, so a caller can hand over memory it already owns
 * and have the device DMA into it. That is the difference between "use the
 * buffers the library allocated" and "use the buffers the application
 * allocated", and the latter is what a framework holding its own GPU tensors
 * or hugepage arenas needs.
 *
 * Lookup is one load regardless of how many regions are registered:
 *
 *     chunk_idx = vaddr >> gran_shift
 *     phys      = lut_phys[chunk_idx] + (vaddr & gran_mask)
 *
 * `lut_phys` covers the whole chunk_idx range and is MAP_NORESERVE, so the
 * kernel demand-pages it and the resident cost tracks live chunks rather than
 * virtual capacity.
 *
 * A registry belongs to one dmamem, so a process holding several pays the
 * reservation once per dmamem and registers a buffer into each one it wants
 * that buffer addressable from. That suits a consumer with a single heap and
 * is worth weighing before giving every controller its own.
 *
 * Backings
 * --------
 *
 * A registration does not describe what the device can address; the
 * allocation it falls inside does. Vendor runtimes differ sharply on this. On
 * ROCm the range arguments to an export are accepted and discarded and the
 * whole buffer object comes back, so exporting per registration, or worse per
 * chunk, both re-exports the entire allocation and resolves a sub-range at a
 * non-zero offset to the base of the allocation. CUDA honours the range
 * exactly. Since the failure on ROCm is silent, the shape that is correct on
 * both is the one used here: recover the allocation a registration falls
 * inside, populate it once, and let overlapping registrations share it.
 *
 * A `backing` is that allocation. It is refcounted by the registrations
 * referring to it, populated when the first arrives and released when the last
 * leaves. `tools/upcie_dmabuf_probe_{cuda,hip}` measures the behaviour this
 * rests on.
 *
 * Sizing
 * ------
 *
 * The reservation is `(1 << va_bits) / granularity` slots of eight bytes, so
 * it scales inversely with granularity: at 48-bit VA that is 1 GiB for a 2 MiB
 * granularity but 512 GiB for a 4 KiB one. Callers with a small granularity
 * should pass a smaller `va_bits` bounded by the address range they actually
 * use.
 *
 * Adoption
 * --------
 *
 * A caller that already knows the addresses, such as a heap that enumerated
 * them at init, registers with `dmamem_registry_adopt()` rather than paying to
 * rediscover them. An adopted backing is borrowed and is never released by the
 * registry.
 *
 * @file dmamem_registry.h
 * @version 0.6.0
 */

/**
 * Default width of the virtual address space, in bits, used to size the LUT.
 *
 * 47 rather than 48 because that is the whole of it: on x86-64 without `la57`
 * the canonical split puts user addresses in [0, 2^47), so a table spanning 47
 * bits covers every address a process can be handed, and 48 would reserve
 * twice as much to index a half that cannot occur. Device pointers included,
 * since unified virtual addressing carves them from the same address space.
 *
 * Five-level paging widens the user range to 56 bits, where neither 47 nor 48
 * spans it and both rely on Linux handing out addresses below 2^47 unless the
 * caller asks for more. A registration above the span fails at
 * dmamem_registry_add(), loudly, rather than resolving wrongly.
 */
/**
 * Default width of the address space the table spans.
 *
 * 47 is the whole user address space on x86-64 without la57. A consumer may
 * lower it to shrink the reservation, but doing so narrows what can be
 * translated: the fast path bounds-checks by assert only, so a pointer above
 * the span is an out-of-range read in a release build rather than the zero
 * that means unregistered. Lower it only where every address handed to the
 * PRP builders is known to fit.
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
 * nlut * granularity)`, and anything that must be undone later goes in
 * `attach_out`; a flavour with nothing to release leaves it zeroed. On failure
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
 */
struct dmamem_registry {
	uint64_t *lut_phys;                       ///< chunk_idx -> chunk base address; mmap-backed
	int gran_shift;                           ///< log2(granularity)
	uint64_t gran_mask;                       ///< granularity - 1, for the intra-chunk offset
	size_t lut_capacity;                      ///< Number of slots in the LUT
	struct dmamem_registry_backing *backings; ///< Owned list of backings
	struct dmamem_registry_registration *list; ///< Owned list of registrations
	/* The callbacks run with the lock held, so none of them may register,
	 * remove, or translate through this registry. */
	dmamem_registry_range_fn range;       ///< Recovers an allocation; may be NULL
	dmamem_registry_populate_fn populate; ///< Makes an allocation addressable; may be NULL
	dmamem_registry_release_fn release;   ///< Undoes populate; may be NULL
	void *ctx;                            ///< Passed to the callbacks; not owned
	pthread_mutex_t lock;                 ///< Serialises registration; see below
};

/*
 * The lock covers registration and removal, never translation. Translation
 * reads one table slot and touches nothing else, so it stays lock-free and
 * runs concurrently with registration of other allocations, which is the
 * property the whole absolute-indexed design exists to provide.
 *
 * It is held across `populate`, which exports and attaches a dma-buf and is
 * therefore slow, so registrations of different allocations serialise. That is
 * affordable because there is one export per allocation rather than one per
 * granule: a consumer registering a handful of large buffers pays it a handful
 * of times. It would not have been affordable against a per-granule export.
 */

/**
 * Initialize a registry.
 *
 * Reserves the demand-paged LUT; no physical memory is committed until
 * something is registered. `granularity` must be a power of two, and every
 * allocation registered must be contiguous in bus-address terms across it,
 * which populate is expected to verify.
 *
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
	int err;

	if (!registry || !granularity || (granularity & (granularity - 1))) {
		return -EINVAL;
	}

	while (((size_t)1 << gran_shift) < granularity) {
		++gran_shift;
	}

	if (!va_bits) {
		va_bits = DMAMEM_REGISTRY_VA_BITS;
	}
	/* lut_capacity is 1 << (va_bits - gran_shift), so the difference has to
	 * stay a shift a size_t can take. */
	if ((va_bits <= gran_shift) || (va_bits > 64) || ((va_bits - gran_shift) >= 64)) {
		return -EINVAL;
	}

	memset(registry, 0, sizeof(*registry));

	/* pthread_mutex_init reports through its return value and leaves errno
	 * alone, so reading errno here would report success for a failure. */
	err = pthread_mutex_init(&registry->lock, NULL);
	if (err) {
		UPCIE_DEBUG("FAILED: pthread_mutex_init(); err(%d)", err);
		return -err;
	}
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
		pthread_mutex_destroy(&registry->lock);
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
 * A partly filled range is worse than an empty one: a slot carrying an address
 * that no registration owns resolves to a plausible wrong target, where an
 * untouched slot resolves to the zero callers read as an error.
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
 * A registration inside an allocation that is already populated shares it. A
 * range straddling two backings, or matching one only partially, is not a
 * match; that would mean the runtime reported inconsistent allocations, and
 * silently reusing the wrong one is how a DMA ends up in the wrong place.
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
 * The caller's table is finer than the granularity, so only every
 * `granularity >> lut_shift`-th entry ends up in the LUT. The entries skipped
 * are not ignored: each granule is verified to be one contiguous run, since
 * otherwise `base + offset` would resolve inside it to an address the caller
 * never gave us. Checking costs one pass at registration and turns a wrong
 * address into an error.
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
 * Backings are assumed never to overlap: the table is indexed per granule, so
 * two backings sharing one would each claim it, the second overwriting the
 * first's address, and releasing either would clear a granule the other still
 * owns. Full containment is fine and is what the refcount is for; a partial
 * overlap means the caller has described the same memory two incompatible
 * ways, which is refused rather than resolved.
 */
static inline int
dmamem_registry_backing_overlaps(struct dmamem_registry *registry, uint64_t base, size_t size)
{
	for (struct dmamem_registry_backing *b = registry->backings; b; b = b->next) {
		const int disjoint = ((base + size) <= b->base) || (base >= (b->base + b->size));
		const int contains = (base >= b->base) && ((base + size) <= (b->base + b->size));
		const int contained = (b->base >= base) && ((b->base + b->size) <= (base + size));

		if (!disjoint && !contains && !contained) {
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
 * A range falling inside a backing that is already live refcounts that backing
 * and does not read the caller's table: the addresses are the allocation's,
 * not the registration's, so a second caller describing the same allocation
 * differently is ignored rather than merged.
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

	/* Adoption names its own range; otherwise ask the flavour which
	 * allocation this is part of, and fall back to the range itself when
	 * it cannot say. */
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
 * `vaddr` and `nbytes` may have any byte alignment; what must be granule
 * aligned is the allocation the range falls inside, which the caller does not
 * choose. Consumers may impose more, e.g. NVMe PRP construction wants
 * host-page-aligned buffers.
 *
 * @return 0 on success, negative errno on failure. -EINVAL when the allocation
 *         exceeds the LUT capacity chosen at init.
 */
static inline int
dmamem_registry_add(struct dmamem_registry *registry, void *vaddr, size_t nbytes,
		    struct dmamem_registry_registration **out)
{
	int err;

	if (!registry) {
		return -EINVAL;
	}

	pthread_mutex_lock(&registry->lock);
	err = dmamem_registry_add_impl(registry, vaddr, nbytes, NULL, 0, out);
	pthread_mutex_unlock(&registry->lock);

	return err;
}

/**
 * Register a range whose addresses the caller already knows.
 *
 * `lut` holds addresses for the range starting at `vaddr`, which must be
 * granule-aligned, one entry per `1 << lut_shift` bytes, no coarser than the
 * registry's granularity. Nothing is discovered and nothing is released; the
 * caller keeps ownership of whatever produced the addresses and must outlive
 * the registration.
 *
 * Note that a LUT finer than the granularity is sampled, not checked: only
 * every `granularity / (1 << lut_shift)`-th entry is read, so the caller is
 * asserting that its region is contiguous across each granule.
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
dmamem_registry_adopt(struct dmamem_registry *registry, void *vaddr, size_t nbytes,
		      const uint64_t *lut, int lut_shift,
		      struct dmamem_registry_registration **out)
{
	int err;

	if (!registry || !lut || lut_shift > registry->gran_shift) {
		return -EINVAL;
	}

	/* Chunks are indexed in absolute terms, so the adopted range must start
	 * on a chunk boundary for the caller's LUT to line up with them. */
	if ((uint64_t)vaddr & registry->gran_mask) {
		UPCIE_DEBUG("FAILED: vaddr(%p) is not granule aligned", vaddr);
		return -EINVAL;
	}

	pthread_mutex_lock(&registry->lock);
	err = dmamem_registry_add_impl(registry, vaddr, nbytes, lut, lut_shift, out);
	pthread_mutex_unlock(&registry->lock);

	return err;
}

/**
 * Remove the registration starting at `vaddr`.
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

	pthread_mutex_lock(&registry->lock);
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
	pthread_mutex_unlock(&registry->lock);

	return err;
}

/**
 * Drop every registration, releasing the backings they held. The LUT
 * reservation stays, so the registry remains usable.
 */
static inline void
dmamem_registry_clear(struct dmamem_registry *registry)
{
	struct dmamem_registry_registration *next;

	if (!registry) {
		return;
	}

	pthread_mutex_lock(&registry->lock);
	for (struct dmamem_registry_registration *m = registry->list; m; m = next) {
		next = m->next;
		dmamem_registry_backing_deref(registry, m->backing);
		free(m);
	}
	registry->list = NULL;
	pthread_mutex_unlock(&registry->lock);
}

/**
 * Tear down a registry, releasing every backing and the LUT reservation.
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

	pthread_mutex_destroy(&registry->lock);
}

/**
 * Whether `[virt, virt + nbytes)` lies inside a live registration.
 *
 * Walks the backings, so this is a cold-path check, not something the
 * translation path can afford. It exists because translation cannot answer
 * this: the table is indexed per granule, so an address in the unused
 * remainder of a granule claimed by a smaller allocation resolves to a
 * plausible wrong address rather than to zero. Callers that want to know
 * whether an address is really theirs ask here.
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

	pthread_mutex_lock(&registry->lock);
	for (struct dmamem_registry_registration *m = registry->list; m; m = m->next) {
		if ((va >= m->vaddr) && ((va + nbytes) <= (m->vaddr + m->size))) {
			pthread_mutex_unlock(&registry->lock);
			return 1;
		}
	}
	pthread_mutex_unlock(&registry->lock);

	return 0;
}

/**
 * Resolve a registered virtual address. One load, whatever is registered.
 *
 * @return 0 on success, -EINVAL when `virt` is not in a live chunk.
 */
static inline int
dmamem_registry_virt_to_phys(struct dmamem_registry *registry, void *virt, uint64_t *phys)
{
	if (!registry || !virt || !phys) {
		return -EINVAL;
	}

	const uint64_t va = (uint64_t)virt;
	const size_t idx = (size_t)(va >> registry->gran_shift);
	uint64_t base;

	if (idx >= registry->lut_capacity) {
		return -EINVAL;
	}

	base = registry->lut_phys[idx];
	if (base == 0) {
		return -EINVAL;
	}

	*phys = base + (va & registry->gran_mask);

	return 0;
}
