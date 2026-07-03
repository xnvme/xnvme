// SPDX-License-Identifier: BSD-3-Clause

/**
 * Registry of externally-allocated CUDA buffers for NVMe DMA
 * ==========================================================
 *
 * Where cudamem_heap pre-allocates a single dma-buf-backed range and serves
 * sub-allocations from it, cudamem_mapping registers buffers that the caller
 * already allocated (via cuMemAlloc, cuMemCreate/cuMemMap, or cudaMalloc) and
 * builds a physical address LUT for them via the same dma-buf interface.
 *
 * Chunk-cached design
 * -------------------
 *
 * The registry caches one dma-buf per `alloc_granularity`-aligned VA chunk
 * (typically 2 MiB on NVIDIA discrete GPUs, queried via
 * cuMemGetAllocationGranularity at config_init time). Each chunk's BAR1 IOVA
 * window is contiguous (BAR1 page-table large-page guarantee), so each chunk
 * has one `phys_base` (held in `lut_phys[chunk_idx]`), and per-page phys is
 * computed during virt_to_phys as
 *
 *     phys = lut_phys[chunk_idx] + (vaddr & (alloc_granularity - 1))
 *
 * `_add` walks chunks intersecting the floored user range, ref-bumps existing
 * entries, and populates new entries via cuMemGetHandleForAddressRange +
 * dmabuf_attach + dmabuf_get_lut. `_remove` decrements rc and frees the dma-buf
 * when rc reaches zero. Repeated overlapping registrations in the same chunk
 * amortize to one dma-buf cost, and resolve to identical phys for any VA they
 * share.
 *
 * Registrations and chunks form a many-to-many relationship: one registration
 * may span multiple chunks (large or chunk-boundary-crossing buffers), and one
 * chunk may be referenced by multiple registrations (sub-granularity
 * allocations packed by the driver). The chunk's `rc` counts the latter.
 *
 * Lookup table layout
 * -------------------
 *
 * Chunks are indexed directly by chunk_idx = vaddr >> alloc_granularity_shift,
 * over the full user virtual address space (CUDAMEM_MAPPING_VA_BITS, default
 * 48). Two parallel arrays cover the chunk_idx range:
 *
 *   lut_phys[chunk_idx] -> uint64_t phys_base (0 == unmapped)
 *   lut_meta[chunk_idx] -> { rc, dmabuf attach }  (cold path only)
 *
 * Both are MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE mmaps. The kernel
 * demand-pages individual host pages on first write, so the physical footprint
 * scales with live chunks rather than virtual capacity. With a 48-bit VA and
 * 2 MiB granularity that is 1 GiB (lut_phys) + 4 GiB (lut_meta) of virtual
 * reservation, but typically a few hundred KiB resident.
 *
 * The split keeps the hot path flat-array-of-u64: `_virt_to_phys` reads
 * `lut_phys[chunk_idx]` only and never touches `lut_meta`.
 *
 * Caveat: Hardware requirements
 * -----------------------------
 *
 * Same as cudamem_heap: requires a GPU with PCIe P2P DMA support and a BAR1
 * region at least as large as device memory. cudamem_config_init() verifies the
 * BAR1 precondition. Additionally, the chunk-cache assumes the BAR1 page-table
 * maps each alloc_granularity-sized export as one contiguous IOVA range; on
 * hardware violating this, _add returns -EOPNOTSUPP after detecting a
 * non-contiguous LUT.
 *
 * Caveat: Single-GPU registry
 * ---------------------------
 *
 * One registry describes one device. Multi-GPU usage requires one
 * cudamem_mapping_registry per GPU (and one cudamem_config, one cudamem_heap
 * each).
 *
 * Caveat: Virtual address width
 * -----------------------------
 *
 * The LUT capacity assumes user vaddrs fit in CUDAMEM_MAPPING_VA_BITS bits.
 * 48 bits covers x86-64 and ARM64 default page-table widths. Systems with
 * LA57 or 52-bit ARM64 user VA may return vaddrs above this bound; on those
 * systems the constant must be raised at compile time.
 *
 * Caveat: Virtual memory reservation
 * ----------------------------------
 *
 * The two LUTs reserve 1 GiB (lut_phys) + 4 GiB (lut_meta) of virtual
 * address space at 48-bit VA / 2 MiB granularity. They are MAP_NORESERVE,
 * so no swap is committed and physical RSS scales with live chunks. On
 * systems configured for strict accounting (vm.overcommit_memory=2) or
 * inside cgroups whose memory.max counts virtual reservation, registry_init
 * may return -ENOMEM. Workarounds are to relax the accounting policy or to
 * lower CUDAMEM_MAPPING_VA_BITS to bound the reservation.
 *
 * Sentinel: lut_phys[idx] == 0
 * ----------------------------
 *
 * `_virt_to_phys` treats `lut_phys[idx] == 0` as "chunk not registered".
 * Phys 0 is a theoretically valid BAR1 base, but PCIe BAR1 is allocated by
 * the host bridge into nonzero IOVA space in practice. Code paths that
 * mutate state (`_add`, `_remove`, `_clear`) consult `lut_meta[idx].rc`
 * directly and do not depend on this sentinel.
 * 
 * @file cudamem_mapping.h
 * @version 0.5.1
 */

/**
 * Width of the user virtual address space, in bits, used to size the chunk
 * LUTs.
 */
#define CUDAMEM_MAPPING_VA_BITS 48

/**
 * Per-chunk cache entry (cold-path only).
 *
 * Owns the dma-buf for one `alloc_granularity`-aligned VA chunk. `rc` counts
 * the number of registrations whose floored range intersects this chunk.
 * When rc transitions to zero the dma-buf is detached and the corresponding
 * `lut_phys` slot is cleared.
 *
 * The chunk's `phys_base` is held in `lut_phys[chunk_idx]`, not here, so the
 * hot path needs only one LUT load.
 */
struct cudamem_mapping_chunk_meta {
	uint32_t rc;          ///< Refcount of overlapping registrations
	struct dmabuf attach; ///< dma-buf attachment (valid when rc > 0)
};

/**
 * Per-registration metadata.
 *
 * Tracks one (vaddr, size) registration so `_remove(vaddr)` can locate the
 * chunks to deref. The chunk LUTs own the dma-bufs, not this struct.
 */
struct cudamem_mapping_registration {
	uint64_t vaddr;                            ///< Virtual address of the registered range
	size_t size;                               ///< Size of the registered range in bytes
	struct cudamem_mapping_registration *next; ///< List linkage owned by the registry
};

/**
 * Registry of all registrations for one CUDA device.
 *
 * Owns two demand-paged LUTs covering the full user VA space at chunk
 * granularity: lut_phys (1 GiB virtual at 48-bit VA / 2 MiB gran) for the hot
 * path and lut_meta (4 GiB virtual at 48-bit VA / 2 MiB gran) for the cold
 * path, plus the linked list of registration metadata.
 *
 * `gran_shift` and `gran_mask` are derived from `cudamem_config.alloc_granularity`
 * at init time and cached here so the LUT-only paths (`_virt_to_phys`,
 * `_remove`, `_clear`) need no config reference. `_add` still takes the config
 * as a separate argument because chunk population calls into CUDA.
 */
struct cudamem_mapping_registry {
	int gran_shift;                              ///< alloc_granularity expressed as a power of two
	uint64_t gran_mask;                          ///< alloc_granularity - 1, for chunk-offset masking
	size_t lut_capacity;                         ///< Number of slots in each LUT
	uint64_t *lut_phys;                          ///< chunk_idx -> phys_base; mmap-backed
	struct cudamem_mapping_chunk_meta *lut_meta; ///< chunk_idx -> meta; mmap-backed
	struct cudamem_mapping_registration *list;   ///< Owned list of registration metadata
};

/**
 * Initialize a mapping registry for the given config.
 *
 * Reserves two demand-paged virtual ranges (lut_phys and lut_meta), each
 * sized to (1 << CUDAMEM_MAPPING_VA_BITS) / alloc_granularity slots. No
 * physical memory is committed until chunks are registered.
 *
 * Caches `gran_shift`, `gran_mask`, and `lut_capacity` derived from the
 * config; the registry retains no pointer to the config struct, so the
 * caller is free to discard it after init. `cudamem_mapping_add` takes the
 * config as a separate argument since populating a chunk needs the full
 * config (page size, etc.).
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
cudamem_mapping_registry_init(struct cudamem_mapping_registry *registry,
			      struct cudamem_config *config)
{
	size_t phys_bytes, meta_bytes;

	if (!registry || !config) {
		return -EINVAL;
	}

	registry->gran_shift = config->alloc_granularity_shift;
	registry->gran_mask = (uint64_t)config->alloc_granularity - 1;
	registry->lut_capacity = (1ULL << CUDAMEM_MAPPING_VA_BITS) >> registry->gran_shift;
	registry->list = NULL;

	phys_bytes = registry->lut_capacity * sizeof(*registry->lut_phys);
	registry->lut_phys = mmap(NULL, phys_bytes, PROT_READ | PROT_WRITE,
				  MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (registry->lut_phys == MAP_FAILED) {
		UPCIE_DEBUG("FAILED: mmap(lut_phys, %zu); errno: %d", phys_bytes, errno);
		registry->lut_phys = NULL;
		return -ENOMEM;
	}

	meta_bytes = registry->lut_capacity * sizeof(*registry->lut_meta);
	registry->lut_meta = mmap(NULL, meta_bytes, PROT_READ | PROT_WRITE,
				  MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (registry->lut_meta == MAP_FAILED) {
		UPCIE_DEBUG("FAILED: mmap(lut_meta, %zu); errno: %d", meta_bytes, errno);
		munmap(registry->lut_phys, phys_bytes);
		registry->lut_phys = NULL;
		registry->lut_meta = NULL;
		return -ENOMEM;
	}

	return 0;
}

/**
 * Decrement rc on chunks [chunk_first, chunk_first + chunk_cnt) and detach
 * the dma-buf for each chunk whose rc transitions to zero.
 *
 * Chunks with rc == 0 on entry are skipped, making the helper safe for
 * partial unwind paths (where some chunks in the range were never bumped).
 */
static inline void
cudamem_mapping_chunk_deref(struct cudamem_mapping_registry *registry, size_t chunk_first,
			    size_t chunk_cnt)
{
	for (size_t k = 0; k < chunk_cnt; ++k) {
		const size_t idx = chunk_first + k;
		struct cudamem_mapping_chunk_meta *cm = &registry->lut_meta[idx];

		if (cm->rc == 0) {
			continue;
		}
		cm->rc--;
		if (cm->rc == 0) {
			dmabuf_detach(&cm->attach);
			memset(&cm->attach, 0, sizeof(cm->attach));
			registry->lut_phys[idx] = 0;
		}
	}
}

/**
 * Detach all cached dma-bufs and free registration metadata.
 *
 * Walks the registration list and decrements each chunk's rc, releasing the
 * dma-buf and clearing the lut_phys slot when rc reaches zero. The LUT mmaps
 * stay reserved.
 */
static inline void
cudamem_mapping_clear(struct cudamem_mapping_registry *registry)
{
	struct cudamem_mapping_registration *next;

	if (!registry) {
		return;
	}

	const uint64_t mask = registry->gran_mask;
	const int gran_shift = registry->gran_shift;

	for (struct cudamem_mapping_registration *m = registry->list; m; m = next) {
		const size_t chunk_first = (size_t)(m->vaddr >> gran_shift);
		const size_t chunk_cnt =
			(size_t)(((m->vaddr & mask) + m->size + mask) >> gran_shift);

		cudamem_mapping_chunk_deref(registry, chunk_first, chunk_cnt);

		next = m->next;
		free(m);
	}
	registry->list = NULL;
}

/**
 * Tear down a registry, releasing the LUT mmaps and detaching all cached
 * dma-bufs.
 */
static inline void
cudamem_mapping_registry_term(struct cudamem_mapping_registry *registry)
{
	if (!registry) {
		return;
	}

	cudamem_mapping_clear(registry);

	if (registry->lut_phys) {
		munmap(registry->lut_phys, registry->lut_capacity * sizeof(*registry->lut_phys));
		registry->lut_phys = NULL;
	}
	if (registry->lut_meta) {
		munmap(registry->lut_meta, registry->lut_capacity * sizeof(*registry->lut_meta));
		registry->lut_meta = NULL;
	}
	registry->lut_capacity = 0;
}

/**
 * Populate one chunk from CUDA.
 *
 * Calls cuMemGetHandleForAddressRange for the chunk's full alloc_granularity
 * extent, attaches the dma-buf, fetches the host-page LUT, verifies that the
 * LUT is contiguous (BAR1 large-page assumption), and returns the chunk's
 * phys_base via *phys_base_out and the attachment via *attach_out. On any
 * failure both outputs are left untouched and an errno is returned.
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
cudamem_mapping_chunk_populate(uint64_t *phys_base_out, struct dmabuf *attach_out,
			       uint64_t chunk_va, struct cudamem_config *config)
{
	const size_t pagesize = (size_t)config->pagesize;
	const size_t gran = config->alloc_granularity;
	const size_t nphys = gran >> config->pagesize_shift;
	uint64_t *tmp = NULL;
	int dmabuf_fd = -1;
	struct dmabuf attach = {0};
	int err;
	CUresult cr;

	tmp = calloc(nphys, sizeof(*tmp));
	if (!tmp) {
		return -ENOMEM;
	}

	cr = cuMemGetHandleForAddressRange(&dmabuf_fd, (CUdeviceptr)chunk_va, gran,
					   CU_MEM_RANGE_HANDLE_TYPE_DMA_BUF_FD, 0);
	if (cr != CUDA_SUCCESS) {
		UPCIE_DEBUG("FAILED: cuMemGetHandleForAddressRange(0x%" PRIx64 ", %zu), cr: %d",
			    chunk_va, gran, cr);
		err = -EIO;
		goto err_free;
	}

	err = dmabuf_attach(dmabuf_fd, &attach);
	if (err) {
		UPCIE_DEBUG("FAILED: dmabuf_attach(), err: %d", err);
		close(dmabuf_fd);
		goto err_free;
	}

	err = dmabuf_get_lut(&attach, nphys, tmp, (int)pagesize);
	if (err) {
		UPCIE_DEBUG("FAILED: dmabuf_get_lut(), err: %d", err);
		goto err_detach;
	}

	for (size_t i = 1; i < nphys; ++i) {
		if (tmp[i] != tmp[0] + i * pagesize) {
			UPCIE_DEBUG("FAILED: chunk LUT not contiguous at i=%zu; phys[0]=0x%" PRIx64
				    " phys[%zu]=0x%" PRIx64,
				    i, tmp[0], i, tmp[i]);
			err = -EOPNOTSUPP;
			goto err_detach;
		}
	}

	*phys_base_out = tmp[0];
	*attach_out = attach;
	free(tmp);
	return 0;

err_detach:
	dmabuf_detach(&attach);
err_free:
	free(tmp);
	return err;
}

/**
 * Register an externally-allocated CUDA range with the given registry.
 *
 * Walks the alloc_granularity-aligned chunks intersecting [vaddr, vaddr +
 * nbytes), ref-bumps existing entries and populates new ones via
 * cuMemGetHandleForAddressRange. Repeated registrations whose floored ranges
 * overlap reuse the cached dma-buf and resolve to identical phys.
 *
 * `vaddr` and `nbytes` may have arbitrary byte alignment; the chunk cache
 * resolves at byte granularity. Note that downstream consumers may impose
 * stricter alignment, e.g., NVMe PRP construction requires host-page-aligned
 * buffer addresses (asserted in nvme_request_prep_command_prps_*_cuda_mapped).
 *
 * `config` must describe the same device the registry was initialized with;
 * it is consulted only to populate new chunks (host page size for the
 * dma-buf LUT and granularity for the CUDA handle range). It is not retained.
 *
 * If non-NULL, `*out` is set to the newly-allocated registration node on
 * success.
 *
 * NOTE: Set up CUDA Driver (cuInit()) and CUDA Context (cuCtxCreate())
 * before calling this function.
 *
 * @return 0 on success, negative errno on failure. -EINVAL if the vaddr
 *         range exceeds the LUT's chunk_idx capacity (raise
 *         CUDAMEM_MAPPING_VA_BITS). -EOPNOTSUPP if the BAR1 contiguity
 *         assumption is violated for some chunk.
 */
static inline int
cudamem_mapping_add(struct cudamem_mapping_registry *registry, struct cudamem_config *config,
		    void *vaddr, size_t nbytes, struct cudamem_mapping_registration **out)
{
	uint64_t va;
	size_t chunk_first, chunk_cnt, bumped_cnt = 0;
	struct cudamem_mapping_registration *m = NULL;
	int err;

	if (!registry || !config || !vaddr || !nbytes) {
		return -EINVAL;
	}

	const uint64_t mask = registry->gran_mask;
	const int gran_shift = registry->gran_shift;

	va = (uint64_t)vaddr;
	chunk_first = (size_t)(va >> gran_shift);
	chunk_cnt = (size_t)(((va & mask) + nbytes + mask) >> gran_shift);

	if (chunk_first + chunk_cnt > registry->lut_capacity) {
		UPCIE_DEBUG(
			"FAILED: vaddr range exceeds LUT capacity; raise CUDAMEM_MAPPING_VA_BITS");
		return -EINVAL;
	}

	m = calloc(1, sizeof(*m));
	if (!m) {
		return -ENOMEM;
	}
	m->vaddr = va;
	m->size = nbytes;

	for (size_t k = 0; k < chunk_cnt; ++k) {
		const size_t idx = chunk_first + k;
		struct cudamem_mapping_chunk_meta *cm = &registry->lut_meta[idx];

		if (cm->rc == 0) {
			const uint64_t chunk_va = (uint64_t)idx << gran_shift;
			err = cudamem_mapping_chunk_populate(&registry->lut_phys[idx], &cm->attach,
							     chunk_va, config);
			if (err) {
				goto err_unwind;
			}
		}
		cm->rc++;
		bumped_cnt++;
	}

	m->next = registry->list;
	registry->list = m;
	if (out) {
		*out = m;
	}

	return 0;

err_unwind:
	cudamem_mapping_chunk_deref(registry, chunk_first, bumped_cnt);

	free(m);
	return err;
}

/**
 * Remove a registration from the registry.
 *
 * Finds the registration with the given vaddr in the list, decrements rc on
 * each chunk it covered, and detaches the dma-buf for chunks whose rc
 * reaches zero.
 *
 * @return 0 on success, -EINVAL if no registration with that vaddr exists.
 */
static inline int
cudamem_mapping_remove(struct cudamem_mapping_registry *registry, void *vaddr)
{
	if (!registry) {
		return -EINVAL;
	}

	const uint64_t key = (uint64_t)vaddr;
	const int gran_shift = registry->gran_shift;
	const uint64_t mask = registry->gran_mask;

	for (struct cudamem_mapping_registration **prev = &registry->list, *m = registry->list; m;
	     prev = &m->next, m = m->next) {
		if (m->vaddr == key) {
			const size_t chunk_first = (size_t)(m->vaddr >> gran_shift);
			const size_t chunk_cnt =
				(size_t)(((m->vaddr & mask) + m->size + mask) >> gran_shift);

			cudamem_mapping_chunk_deref(registry, chunk_first, chunk_cnt);

			*prev = m->next;
			free(m);
			return 0;
		}
	}

	return -EINVAL;
}

/**
 * Resolve a CUDA virtual address registered with the registry.
 *
 * O(1): one LUT load.
 *
 * @return 0 on success, -EINVAL if `virt` is not in a registered chunk.
 */
static inline int
cudamem_mapping_virt_to_phys(struct cudamem_mapping_registry *registry, void *virt, uint64_t *phys)
{
	if (!registry || !virt || !phys) {
		return -EINVAL;
	}

	const uint64_t va = (uint64_t)virt;
	const int gran_shift = registry->gran_shift;
	const uint64_t mask = registry->gran_mask;
	const size_t idx = (size_t)(va >> gran_shift);
	uint64_t base;

	if (idx >= registry->lut_capacity) {
		return -EINVAL;
	}

	base = registry->lut_phys[idx];
	if (base == 0) {
		return -EINVAL;
	}

	*phys = base + (va & mask);
	return 0;
}
