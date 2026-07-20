// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * NVMe Request Abstraction
 * ========================
 *
 * This header defines a minimal software abstraction for managing NVMe command identifiers (CIDs)
 * in user space. The abstraction uses a fixed-size pool of `struct nvme_request`, each assigned a
 * CID, along with a freelist-based allocator for constant-time allocation and release.
 *
 * This is not part of the NVMe specification, but is useful for tracking user-submitted commands
 * while they are in flight and associating user-defined metadata with each command.
 *
 * Caveat
 * ------
 *
 * assert() is used here. thus instead of a segfault, you will get a nice message like::
 *
 *   nvme_request_get: Assertion `cid < NVME_REQUEST_POOL_LEN' failed.
 *
 * Of course, this comes at a cost, so, make sure e.g. meson disables assert on release builds.
 *
 * The stack implementation has an upper-bound of NVME_REQUEST_POOL_LEN elements.
 *
 * @file nvme_request.h
 * @version 0.5.2
 */

#define NVME_REQUEST_POOL_LEN 1024

struct nvme_request {
	uint16_t cid; ///< The NVMe command identifier
	uint8_t rsvd[6];

	void *user;        ///< An arbitrary pointer for caller to pass on to completion
	uint64_t prp_addr; ///< Use this when constructing command.PRP2
	void *prp;         ///< Use this when constructing the PRP-list itself
};

struct nvme_request_pool {
	struct nvme_request reqs[NVME_REQUEST_POOL_LEN];
	uint16_t stack[NVME_REQUEST_POOL_LEN];
	size_t top;
	void *prps; ///< Pointer to pre-allocated memory directly mapped to each reqs.
};

/**
 * Initialize a request-pool
 *
 * When intending to use PRPs associated with the commands, then also use:
 *
 * - nvme_request_pool_{init,term}_prps()
 */
static inline void
nvme_request_pool_init(struct nvme_request_pool *pool)
{
	pool->top = NVME_REQUEST_POOL_LEN;
	for (uint16_t i = 0; i < NVME_REQUEST_POOL_LEN; ++i) {
		pool->reqs[i].cid = i;
		pool->stack[NVME_REQUEST_POOL_LEN - 1 - i] = i;
	}
}

static inline void
nvme_request_pool_term_prps(struct nvme_request_pool *pool, struct hostmem_heap *heap)
{
	hostmem_dma_free(heap, pool->prps);
}

static inline int
nvme_request_pool_init_prps(struct nvme_request_pool *pool, struct hostmem_heap *heap)
{
	pool->prps = hostmem_dma_alloc_array(heap, NVME_REQUEST_POOL_LEN, heap->config->pagesize);
	if (!pool->prps) {
		UPCIE_DEBUG("FAILED: hostmem_dma_alloc_array(prps); errno(%d)", errno);
		return -ENOMEM;
	}

	for (uint16_t i = 0; i < NVME_REQUEST_POOL_LEN; ++i) {
		pool->reqs[i].prp = ((uint8_t *)pool->prps) + (i * heap->config->pagesize);
		pool->reqs[i].prp_addr = hostmem_dma_v2p(heap, pool->reqs[i].prp);
	}

	return 0;
}

/**
 * Release the PRP-list scratch region held by a dmamem-backed request pool.
 *
 * The counterpart to nvme_request_pool_init_prps_dmamem. The caller
 * provides the same heap and the prp_offset returned by init; the pool
 * struct itself is caller-owned.
 */
static inline void
nvme_request_pool_term_prps_dmamem(struct nvme_request_pool *pool, struct dmamem_heap *heap,
				   size_t prp_offset)
{
	if (!pool || !pool->prps) {
		return;
	}
	dmamem_heap_free(heap, prp_offset);
	pool->prps = NULL;
}

/**
 * Populate a request pool with per-request PRP-list scratch from a dmamem_heap.
 *
 * Sibling of nvme_request_pool_init_prps: same layout (one 4 KiB page
 * per request, addressed through pool->reqs[i].prp / .prp_addr), but the
 * scratch region is carved from a single contiguous IOAS-mapped block so
 * translation is arithmetic rather than a per-page phys_lut walk.
 *
 * @param pool           Caller-owned request pool (already nvme_request_pool_init'd).
 * @param heap           dmamem_heap the scratch region is carved from.
 * @param prp_offset_out Heap offset of the scratch region, for later term.
 *
 * @return 0 on success, negative errno on allocation failure.
 */
static inline int
nvme_request_pool_init_prps_dmamem(struct nvme_request_pool *pool, struct dmamem_heap *heap,
				   size_t *prp_offset_out)
{
	const size_t pagesize = 4096;
	size_t prp_offset = 0;
	uint8_t *prps_va;
	uint64_t prps_iova;
	int err;

	err = dmamem_heap_alloc_aligned(heap, (size_t)NVME_REQUEST_POOL_LEN * pagesize, pagesize,
					&prp_offset);
	if (err) {
		UPCIE_DEBUG("FAILED: dmamem_heap_alloc_aligned(prps); err(%d)", err);
		return err;
	}

	prps_va = dmamem_heap_at_va(heap, prp_offset);
	prps_iova = dmamem_heap_at_iova(heap, prp_offset);

	pool->prps = prps_va;
	for (uint16_t i = 0; i < NVME_REQUEST_POOL_LEN; ++i) {
		pool->reqs[i].prp = prps_va + ((size_t)i * pagesize);
		pool->reqs[i].prp_addr = prps_iova + ((uint64_t)i * pagesize);
	}

	*prp_offset_out = prp_offset;
	return 0;
}

/**
 * Allocates a request object from the pool.
 *
 * The returned request has a valid CID and may be used for command submission.
 *
 * @param pool The request pool to allocate from.
 * @return On success, a pointer to a request is returned. On error, NULL is returned and errno set
 *         to indicate the error.
 */
static inline struct nvme_request *
nvme_request_alloc(struct nvme_request_pool *pool)
{
	uint16_t cid;

	assert(pool->top > 0);

	if (pool->top == 0) {
		errno = ENOMEM;
		return NULL;
	}

	cid = pool->stack[--pool->top];

	return &pool->reqs[cid];
}

/**
 * Free a request previously allocated with nvme_request_alloc().
 *
 * This marks the `cid` (command-identifier) as available for reuse.
 *
 * The `cid` must no longer be referenced in any submission or completion queue -- that is, the
 * associated command must be fully completed, and any processing of the completion must be done.
 * Only then is it safe to free the request.
 *
 * If you're using submit-on-completion (i.e., reusing the request immediately), there is no need
 * to call this function -- the `cid` is implicitly reused.
 *
 * @param pool The request pool the `cid` came from.
 * @param cid  The command identifier to mark as available again.
 */
static inline void
nvme_request_free(struct nvme_request_pool *pool, uint16_t cid)
{
	assert(pool->top < NVME_REQUEST_POOL_LEN);
	pool->stack[pool->top++] = cid;
}

/**
 * Retrieve the request object associated with the given 'cid'
 *
 * The intended purpose here is to obtain the request-object associated with a command upon its
 * completion.
 *
 * @param pool The request pool the CID belongs to.
 * @param cid The command identifier.
 * @return Pointer to the corresponding request object.
 */
static inline struct nvme_request *
nvme_request_get(struct nvme_request_pool *pool, uint16_t cid)
{
	assert(cid < NVME_REQUEST_POOL_LEN);
	return &pool->reqs[cid];
}

/**
 * Re-translate a PRP entry that crosses a hugepage boundary.
 *
 * Kept out-of-line so the rare boundary path does not inline a v2p into the hot
 * contig builder; the common in-hugepage case strides physically from PRP1, and
 * the inlined stride loop keeps large-I/O builds free of any per-entry call.
 */
static inline uint64_t __attribute__((noinline))
nvme_request_prp_retranslate(struct hostmem_heap *heap, void *virt)
{
	return hostmem_dma_v2p(heap, virt);
}

/**
 * Prepare the PRP entries for a command with a contiguous data buffer.
 *
 * Describes `dbuf` in the command's PRP1/PRP2 fields, building a PRP list in
 * `request` when the buffer spans more than two pages, so the controller can
 * access it while the command is in flight.
 *
 * Caveats
 * -------
 *
 * - `dbuf` must be allocated from `heap`.
 * - `dbuf` must be dword (4-byte) aligned.
 * - PRP list chaining is not supported: `dbuf` may span at most 513 pages
 *   (PRP1 plus a single 512-entry PRP list page).
 *
 * @param request Pointer to the NVMe request context used for tracking and metadata.
 * @param heap Pointer to the host memory heap that `dbuf` is allocated within.
 * @param dbuf Pointer to the (virtually) contiguous data buffer to be described by PRPs.
 * @param dbuf_nbytes Size in bytes of the data buffer.
 * @param cmd Pointer to the NVMe command to be prepared with PRP entries.
 */
static inline void
nvme_request_prep_command_prps_contig(struct nvme_request *request, struct hostmem_heap *heap,
				      void *dbuf, size_t dbuf_nbytes, struct nvme_command *cmd)
{
	const uint64_t pagesize = heap->config->pagesize;

	cmd->prp1 = hostmem_dma_v2p(heap, dbuf);

	/* Only PRP1 may carry a sub-page offset; the page count and every later
	 * entry are measured from the page floor. ceil((off+nbytes)/pagesize). Take
	 * the offset from the virtual address -- v2p preserves the sub-page offset --
	 * so the page count and the npages == 1 branch do not wait on the v2p load. */
	const uint64_t page_off = (uintptr_t)dbuf & (pagesize - 1);
	const uint64_t npages =
		(page_off + dbuf_nbytes + pagesize - 1) >> heap->config->pagesize_shift;

	/* Chaining is not supported, thus assert that the given dbuf fits. */
	assert(npages <= 1 + 512);

	if (npages == 1) {
		return;
	}

	/* The heap is virtually contiguous but physically contiguous only within a
	 * hugepage. Stride PRP entries physically from PRP1 while inside the same
	 * hugepage and re-translate when the running address crosses a hugepage
	 * boundary. A buffer that fits within a single hugepage (the common case)
	 * never crosses, so this reduces to the stride of a contiguous buffer. */
	uint8_t *vbase = (uint8_t *)dbuf - page_off;
	const uint64_t hp_off =
		(uint64_t)(vbase - (uint8_t *)heap->memory.virt) & (heap->config->hugepgsz - 1);
	uint64_t strides_left =
		((heap->config->hugepgsz - hp_off) >> heap->config->pagesize_shift) - 1;
	uint64_t page_phys = cmd->prp1 - page_off;

	if (npages == 2) {
		cmd->prp2 = strides_left ? page_phys + pagesize
					 : nvme_request_prp_retranslate(heap, vbase + pagesize);
		return;
	}

	uint64_t *prp_list = request->prp;

	cmd->prp2 = request->prp_addr;
	for (uint64_t i = 1; i < npages; ++i) {
		if (strides_left) {
			page_phys += pagesize;
			strides_left--;
		} else {
			page_phys = nvme_request_prp_retranslate(
				heap, vbase + (i << heap->config->pagesize_shift));
			strides_left = (heap->config->hugepgsz >> heap->config->pagesize_shift) - 1;
		}
		prp_list[i - 1] = page_phys;
	}
}

/**
 * Prepare the PRP list for a command with an iovec (scatter-gather) data buffer.
 *
 * This function initializes the Physical Region Page (PRP) entries in the given NVMe command
 * (`cmd`) using the provided request and an array of iovec entries. Each iovec entry is assumed to
 * be page-aligned and allocated from the given `heap`.
 *
 * Caveats
 * -------
 *
 * - Each iovec base must be page-aligned and allocated from `heap`.
 * - Does *not* support PRP list chaining; only a single list page is constructed.
 *
 * @param request Pointer to the NVMe request context used for tracking and metadata.
 * @param heap Pointer to the hostmemory heap that iovec buffers are allocated within.
 * @param dvec Array of iovec structures describing the data segments.
 * @param dvec_cnt Number of elements in the dvec array.
 * @param cmd Pointer to the NVMe command to be prepared with PRP entries.
 */
static inline void
nvme_request_prep_command_prps_iov(struct nvme_request *request, struct hostmem_heap *heap,
				   struct iovec *dvec, size_t dvec_cnt, struct nvme_command *cmd)
{
	const uint64_t pagesize = heap->config->pagesize;
	uint64_t *prp_list = request->prp;
	size_t prp_idx = 0;

	cmd->prp1 = hostmem_dma_v2p(heap, dvec[0].iov_base);

	for (size_t i = 0; i < dvec_cnt; ++i) {
		uint8_t *base = dvec[i].iov_base;
		size_t remaining = dvec[i].iov_len;
		size_t offset = 0;

		/* Skip the first page of the first iovec — it is PRP1 */
		if (i == 0) {
			offset = pagesize;
			remaining = (remaining > pagesize) ? remaining - pagesize : 0;
		}

		while (remaining > 0) {
			prp_list[prp_idx++] = hostmem_dma_v2p(heap, base + offset);

			offset += pagesize;
			remaining = (remaining > pagesize) ? remaining - pagesize : 0;
		}
	}

	if (prp_idx == 1) {
		cmd->prp2 = prp_list[0];
	} else if (prp_idx > 1) {
		cmd->prp2 = request->prp_addr;
	}
}

/**
 * Prepare the PRP entries for a command whose contiguous data buffer
 * lives inside a dmamem_heap.
 *
 * Sibling of nvme_request_prep_command_prps_contig for the dmamem
 * world. Behaviour depends on the underlying dmamem's translator:
 *
 *   ARITHMETIC: the heap sits on one contiguous IOAS mapping, so
 *   every subsequent page is at prp1 + i * pagesize. Single
 *   dmamem_va_to_iova at the start; the rest is arithmetic.
 *
 *   LUT: pages inside a hugepage still stride physically from prp1,
 *   but crossing a hugepage boundary requires re-translation. Same
 *   shape as the hostmem_heap variant above: stride within the
 *   hugepage, dmamem_va_to_iova (via phys_lut) at each boundary.
 *
 * The translator dispatch is one predictable compare; a process
 * runs one translator for its lifetime, so it predicts perfectly
 * after warmup.
 *
 * Caveats
 * -------
 *
 * - dbuf must be allocated from heap.
 * - dbuf must be dword (4-byte) aligned.
 * - PRP list chaining is not supported: dbuf may span at most 513
 *   pages (PRP1 plus a single 512-entry PRP list page).
 *
 * @param request      NVMe request context; the PRP list is written
 *                     into request->prp with iova request->prp_addr.
 * @param heap         dmamem_heap that dbuf was carved from.
 * @param dbuf         Virtually contiguous data buffer.
 * @param dbuf_nbytes  Size of dbuf in bytes.
 * @param cmd          Command to populate prp1 (and prp2 / PRP list)
 *                     on.
 */
static inline void
nvme_request_prep_command_prps_contig_dmamem(struct nvme_request *request,
					     struct dmamem_heap *heap, void *dbuf,
					     size_t dbuf_nbytes, struct nvme_command *cmd)
{
	const uint64_t pagesize = 4096;
	const int page_shift = 12;
	struct dmamem *dmem = heap->dmem;
	const uint64_t page_off = (uintptr_t)dbuf & (pagesize - 1);
	const uint64_t npages = (page_off + dbuf_nbytes + pagesize - 1) >> page_shift;

	cmd->prp1 = dmamem_va_to_iova(dmem, dbuf);

	/* Chaining is not supported, thus assert that the given dbuf fits. */
	assert(npages <= 1 + 512);

	if (npages == 1) {
		return;
	}

	if (DMAMEM_XLATE_ARITHMETIC == dmem->translator) {
		const uint64_t page_iova = cmd->prp1 - page_off;

		if (npages == 2) {
			cmd->prp2 = page_iova + pagesize;
			return;
		}

		uint64_t *prp_list = request->prp;

		cmd->prp2 = request->prp_addr;
		for (uint64_t i = 1; i < npages; ++i) {
			prp_list[i - 1] = page_iova + (i << page_shift);
		}
		return;
	}

	/* LUT: stride within the hugepage from prp1; re-translate on
	 * hugepage boundaries. Buffers that fit inside a single hugepage
	 * (the common case) never cross, so this reduces to the stride
	 * of a contiguous buffer. */
	uint8_t *vbase = (uint8_t *)dbuf - page_off;
	const uint64_t hp_off =
		(uint64_t)(vbase - (uint8_t *)dmem->cpu_va) & (dmem->hugepgsz - 1);
	uint64_t strides_left = ((dmem->hugepgsz - hp_off) >> page_shift) - 1;
	uint64_t page_phys = cmd->prp1 - page_off;

	if (npages == 2) {
		cmd->prp2 = strides_left ? page_phys + pagesize
					 : dmamem_va_to_iova(dmem, vbase + pagesize);
		return;
	}

	uint64_t *prp_list = request->prp;

	cmd->prp2 = request->prp_addr;
	for (uint64_t i = 1; i < npages; ++i) {
		if (strides_left) {
			page_phys += pagesize;
			strides_left--;
		} else {
			page_phys = dmamem_va_to_iova(dmem, vbase + (i << page_shift));
			strides_left = (dmem->hugepgsz >> page_shift) - 1;
		}
		prp_list[i - 1] = page_phys;
	}
}

/**
 * Prepare the PRP entries for a command with a scatter-gather data
 * buffer whose iovec elements live inside a dmamem_heap.
 *
 * Sibling of nvme_request_prep_command_prps_iov for the dmamem
 * world. Each iovec base is translated via dmamem_va_to_iova so
 * both ARITHMETIC and LUT dmamems work.
 *
 * Caveats
 * -------
 *
 * - Each iovec base must be page-aligned and allocated from heap.
 * - PRP list chaining is not supported; only a single list page is
 *   constructed.
 *
 * @param request   NVMe request context.
 * @param heap      dmamem_heap the iovec buffers were carved from.
 * @param dvec      Array of iovec structures.
 * @param dvec_cnt  Element count of dvec.
 * @param cmd       Command to populate prp1 (and prp2 / PRP list) on.
 */
static inline void
nvme_request_prep_command_prps_iov_dmamem(struct nvme_request *request,
					  struct dmamem_heap *heap, struct iovec *dvec,
					  size_t dvec_cnt, struct nvme_command *cmd)
{
	const uint64_t pagesize = 4096;
	struct dmamem *dmem = heap->dmem;
	uint64_t *prp_list = request->prp;
	size_t prp_idx = 0;

	cmd->prp1 = dmamem_va_to_iova(dmem, dvec[0].iov_base);

	for (size_t i = 0; i < dvec_cnt; ++i) {
		uint8_t *base = dvec[i].iov_base;
		size_t remaining = dvec[i].iov_len;
		size_t offset = 0;

		/* Skip the first page of the first iovec — it is PRP1 */
		if (i == 0) {
			offset = pagesize;
			remaining = (remaining > pagesize) ? remaining - pagesize : 0;
		}

		while (remaining > 0) {
			prp_list[prp_idx++] = dmamem_va_to_iova(dmem, base + offset);

			offset += pagesize;
			remaining = (remaining > pagesize) ? remaining - pagesize : 0;
		}
	}

	if (prp_idx == 1) {
		cmd->prp2 = prp_list[0];
	} else if (prp_idx > 1) {
		cmd->prp2 = request->prp_addr;
	}
}