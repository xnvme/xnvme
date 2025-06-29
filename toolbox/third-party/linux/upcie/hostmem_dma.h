// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Hugepage-backed malloc-like allocator for DMA in userspace
 * ===========================================================
 *
 * This header provides a minimal, header-only allocator for use in user-space drivers
 * requiring DMA-capable memory. Allocations are backed by hugepages and are guaranteed
 * to be contiguous in both virtual and physical address space — up to hugepage granularity.
 *
 * Interface
 * ---------
 *
 *  - void *hostmem_dma_malloc(size_t size);
 *    Allocate a block of memory of the given size.
 *
 *  - void hostmem_dma_free(void *ptr);
 *    Frees a block previously returned by hostmem_dma_malloc().
 *
 *  - uint64_t hostmem_dma_v2p(void *virt);
 *    Resolve a virtual address to its corresponding physical address.
 *
 * Usage
 * -----
 *
 * You must call `hostmem_dma_init()` before any allocation is made, and `hostmem_dma_term()` after
 * all memory has been freed. The allocator does not support lazy initialization.
 *
 * Caveats
 * -------
 *
 * - Physical contiguity is guaranteed only up to the system's hugepage size. On most systems, this
 *   is 2MB.
 *
 * - Sub-hugepage allocations may span multiple hugepages, resulting in reduced physical
 *   contiguity. This may be addressed in a future update.
 *
 * - Alignment is currently to the system's page size (typically 4KB).
 *
 * Roadmap
 * -------
 *
 * Planned improvements include:
 *
 *   - hostmem_dma_calloc() for zero-initialized memory
 *   - Sub-hugepage contiguity enforcement
 *
 * @file hostmem_dma.h
 * @version 0.3.2
 */

/**
 * Free the DMA-capable memory pointed to by `ptr`
 *
 * If `ptr` is NULL, no operation is performed.
 *
 * @param ptr Pointer previously returned by hostmem_dma_malloc().
 */
static inline void
hostmem_dma_free(struct hostmem_heap *heap, void *ptr)
{
	hostmem_heap_block_free(heap, ptr);
}

/**
 * Allocate `size` bytes of DMA-capable memory
 *
 * @param size Number of bytes to allocate. Passing `size=0` is considered invalid-input by
 *             hostmem_dma_malloc().
 * @return On success, a pointer to the allocated memory is returned. On error, NULL is returned
 *         and `errno` set to indicate the error. or NULL on failure.
 */
static inline void *
hostmem_dma_malloc(struct hostmem_heap *heap, size_t size)
{
	if (!size) {
		errno = EINVAL;
		return NULL;
	}

	return hostmem_heap_block_alloc(heap, size);
}

/**
 * Allocate `size` bytes of DMA-capable memory aligned to given 'alignment'
 *
 * @param size Number of bytes to allocate.
 * @param alignment Boundary to align to.
 * @return Pointer to the allocated memory, or NULL on failure.
 */
static inline void *
hostmem_dma_malloc_aligned(struct hostmem_heap *heap, size_t size, size_t alignment)
{
	return hostmem_heap_block_alloc_aligned(heap, size, alignment);
}

/**
 * Resolve the physical address of a given virtual address.
 *
 * @param virt Pointer to memory previously allocated by hostmem_dma_malloc().
 * @return Physical address corresponding to the given virtual address.
 */
static inline uint64_t
hostmem_dma_v2p(struct hostmem_heap *heap, void *virt)
{
	size_t offset, hpage_idx, in_hpage_offset;

	// Compute byte offset from base of heap
	offset = (char *)virt - (char *)heap->memory.virt;

	// Determine which hugepage this address falls into
	hpage_idx = offset / heap->config->hugepgsz;

	// Offset within that hugepage
	in_hpage_offset = offset % heap->config->hugepgsz;

	return heap->phys_lut[hpage_idx] + in_hpage_offset;
}
