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
 * @version 0.3.2
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
	const size_t prps_nbytes = NVME_REQUEST_POOL_LEN * heap->config->pagesize;

	pool->prps = hostmem_dma_malloc(heap, prps_nbytes);
	if (!pool->prps) {
		UPCIE_DEBUG("FAILED: hostmem_dma_alloc(%zu)", prps_nbytes);
		return -ENOMEM;
	}

	for (uint16_t i = 0; i < NVME_REQUEST_POOL_LEN; ++i) {
		pool->reqs[i].prp = ((uint8_t *)pool->prps) + (i * heap->config->pagesize);
		pool->reqs[i].prp_addr = hostmem_dma_v2p(heap, pool->reqs[i].prp);
	}

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
 * Prepare the PRP list for a command with a contiguous data buffer.
 *
 * This function initializes the Physical Region Page (PRP) entries in the given NVMe command
 * (`cmd`) using the provided request and a contiguous data buffer (in VA-space).
 * It sets up the PRP1 and PRP2 fields in the command to describe the physical memory backing the
 * `data` buffer, allowing the NVMe controller to access the buffer during command execution.
 *
 * Caveats
 * -------
 *
 * - Assumes that the memory backing `dbuf` in `heap` is physically contiguous.
 * - Does *not* support PRP list chaining; only a single list page is constructed.
 *
 * @param request Pointer to the NVMe request context used for tracking and metadata.
 * @param heap Pointer to the hostmemory heap that dbuf is allocated within.
 * @param dbuf Pointer to the contiguous data buffer to be described by PRPs.
 * @param dbuf_nbytes Size in bytes of the data buffer.
 * @param cmd Pointer to the NVMe command to be prepared with PRP entries.
 */
static inline void
nvme_request_prep_command_prps_contig(struct nvme_request *request, struct hostmem_heap *heap,
				      void *dbuf, size_t dbuf_nbytes, struct nvme_command *cmd)
{
	const uint64_t npages = dbuf_nbytes >> heap->config->pagesize_shift;
	const uint64_t pagesize = heap->config->pagesize;

	/* Chaining is not supported, thus assert that the given dbuf fits. */
	assert(npages <= 1 + 512);

	cmd->prp1 = hostmem_dma_v2p(heap, dbuf);

	if (npages == 1) {
		return;
	} else if (npages == 2) {
		cmd->prp2 = hostmem_dma_v2p(heap, dbuf + pagesize);
	} else {
		uint64_t *prp_list = request->prp;

		cmd->prp2 = request->prp_addr;
		for (uint64_t i = 1; i < npages; ++i) {

			prp_list[i - 1] = cmd->prp1 + (i << heap->config->pagesize_shift);
		}
	}
}