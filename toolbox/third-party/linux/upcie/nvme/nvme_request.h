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
 * @version 0.10.0
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
 * Allocate a request-pool, zeroed, on cache lines of its own
 *
 * The pool's top is written on every submission and completion. A pool from
 * plain calloc() ends in the line the next allocation begins in, and when a
 * process creates its queues back to back that is the next queue's own hot
 * state, which another thread writes per I/O; the two then bounce the line
 * between their cores and each loses a good part of its throughput. Release
 * with free().
 *
 * @return The pool on success, NULL with errno set on failure
 */
static inline struct nvme_request_pool *
nvme_request_pool_alloc(void)
{
	const size_t line = 64;
	const size_t nbytes = (sizeof(struct nvme_request_pool) + line - 1) & ~(line - 1);
	void *pool = NULL;
	int err;

	err = posix_memalign(&pool, line, nbytes);
	if (err) {
		errno = err;
		return NULL;
	}
	memset(pool, 0, nbytes);

	return pool;
}

/**
 * Initialize a request-pool
 *
 * When intending to use PRPs associated with the commands, then also use:
 *
 * - nvme_request_pool_{init,term}_prps_dmamem()
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
 * per request, addressed through pool->reqs[i].prp / .prp_addr). The
 * scratch region is virtually contiguous, so the per-request VA is a
 * plain stride, but its IOVA is resolved per page via dmamem_heap_at_iova:
 * on a LUT dmamem the NVME_REQUEST_POOL_LEN * 4 KiB scratch spans several
 * hugepages whose physical pages are not contiguous, so translating the
 * base once and adding a stride would corrupt every entry past the first
 * hugepage boundary. Arithmetic dmamems resolve to the same linear result.
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
	int err;

	err = dmamem_heap_alloc_array_aligned(heap, NVME_REQUEST_POOL_LEN, pagesize, pagesize,
					      &prp_offset);
	if (err) {
		UPCIE_DEBUG("FAILED: dmamem_heap_alloc_array_aligned(prps); err(%d)", err);
		return err;
	}

	prps_va = dmamem_heap_at_va(heap, prp_offset);

	pool->prps = prps_va;
	for (uint16_t i = 0; i < NVME_REQUEST_POOL_LEN; ++i) {
		const size_t off = prp_offset + ((size_t)i * pagesize);

		pool->reqs[i].prp = prps_va + ((size_t)i * pagesize);
		pool->reqs[i].prp_addr = dmamem_heap_at_iova(heap, off);
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
 *   but crossing a hugepage boundary requires re-translation: stride
 *   within the granule, dmamem_va_to_iova (via the registry table) at
 *   each boundary.
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
 * @param dmem         dmamem describing the range dbuf lies in.
 * @param dbuf         Virtually contiguous data buffer.
 * @param dbuf_nbytes  Size of dbuf in bytes.
 * @param cmd          Command to populate prp1 (and prp2 / PRP list)
 *                     on.
 */
static inline int
nvme_request_prep_command_prps_contig_dmamem(struct nvme_request *request, struct dmamem *dmem,
					     void *dbuf, size_t dbuf_nbytes,
					     struct nvme_command *cmd)
{
	const uint64_t pagesize = 4096;
	const int page_shift = 12;
	const uint64_t page_off = (uintptr_t)dbuf & (pagesize - 1);
	const uint64_t npages = (page_off + dbuf_nbytes + pagesize - 1) >> page_shift;

	const int lut = DMAMEM_XLATE_LUT == dmem->translator;

	cmd->prp1 = dmamem_va_to_iova(dmem, dbuf);
	if (lut && !cmd->prp1) {
		UPCIE_DEBUG("FAILED: dbuf(%p) is not in a registered region", dbuf);
		return -EINVAL;
	}

	/* Chaining is not supported. */
	if (npages > 1 + (pagesize / sizeof(uint64_t))) {
		UPCIE_DEBUG("FAILED: dbuf(%p) spans %" PRIu64 " pages; chaining is unsupported",
			    dbuf, npages);
		return -EINVAL;
	}

	if (npages == 1) {
		return 0;
	}

	if (DMAMEM_XLATE_ARITHMETIC == dmem->translator) {
		const uint64_t page_iova = cmd->prp1 - page_off;

		if (npages == 2) {
			cmd->prp2 = page_iova + pagesize;
			return 0;
		}

		uint64_t *prp_list = request->prp;

		cmd->prp2 = request->prp_addr;
		for (uint64_t i = 1; i < npages; ++i) {
			prp_list[i - 1] = page_iova + (i << page_shift);
		}
		return 0;
	}

	/* Stride within the granule, re-translating when crossing out of it. */
	uint8_t *vbase = (uint8_t *)dbuf - page_off;
	const uint64_t span = dmem->registry.gran_mask + 1;
	const uint64_t span_off = (uint64_t)vbase & (span - 1);
	uint64_t strides_left = ((span - span_off) >> page_shift) - 1;
	uint64_t page_phys = cmd->prp1 - page_off;

	if (npages == 2) {
		if (strides_left) {
			cmd->prp2 = page_phys + pagesize;
			return 0;
		}
		cmd->prp2 = dmamem_va_to_iova(dmem, vbase + pagesize);
		if (!cmd->prp2) {
			UPCIE_DEBUG("FAILED: dbuf(%p) leaves the registered region", dbuf);
			return -EINVAL;
		}
		return 0;
	}

	uint64_t *prp_list = request->prp;

	cmd->prp2 = request->prp_addr;
	for (uint64_t i = 1; i < npages; ++i) {
		if (strides_left) {
			page_phys += pagesize;
			strides_left--;
		} else {
			page_phys = dmamem_va_to_iova(dmem, vbase + (i << page_shift));
			if (!page_phys) {
				UPCIE_DEBUG("FAILED: dbuf(%p) leaves the registered region", dbuf);
				return -EINVAL;
			}
			strides_left = (span >> page_shift) - 1;
		}
		prp_list[i - 1] = page_phys;
	}

	return 0;
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
 * @param dmem      dmamem describing the range the iovec buffers lie in.
 * @param dvec      Array of iovec structures.
 * @param dvec_cnt  Element count of dvec.
 * @param cmd       Command to populate prp1 (and prp2 / PRP list) on.
 */
static inline int
nvme_request_prep_command_prps_iov_dmamem(struct nvme_request *request, struct dmamem *dmem,
					  struct iovec *dvec, size_t dvec_cnt,
					  struct nvme_command *cmd)
{
	const uint64_t pagesize = 4096;
	uint64_t *prp_list = request->prp;
	size_t prp_idx = 0;

	const int lut = DMAMEM_XLATE_LUT == dmem->translator;

	cmd->prp1 = dmamem_va_to_iova(dmem, dvec[0].iov_base);
	if (lut && !cmd->prp1) {
		UPCIE_DEBUG("FAILED: iov_base(%p) is not in a registered region",
			    dvec[0].iov_base);
		return -EINVAL;
	}

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
			const uint64_t iova = dmamem_va_to_iova(dmem, base + offset);

			/* Zero is the unregistered sentinel under the LUT only;
			 * an arithmetic mapping may sit at IOVA 0. */
			if (lut && !iova) {
				UPCIE_DEBUG("FAILED: %p is not in a registered region",
					    base + offset);
				return -EINVAL;
			}
			prp_list[prp_idx++] = iova;

			offset += pagesize;
			remaining = (remaining > pagesize) ? remaining - pagesize : 0;
		}
	}

	if (prp_idx == 1) {
		cmd->prp2 = prp_list[0];
	} else if (prp_idx > 1) {
		cmd->prp2 = request->prp_addr;
	}

	return 0;
}