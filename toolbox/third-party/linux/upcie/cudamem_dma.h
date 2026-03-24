// SPDX-License-Identifier: BSD-3-Clause

/**
 * CUDA device memory allocator for NVMe DMA
 * ==========================================
 *
 * This header provides a malloc-like interface over cudamem_heap, mirroring
 * the hostmem_dma API. Allocations are backed by CUDA device memory and are
 * described by a physical address LUT (via dma-buf) for use in NVMe PRP
 * construction.
 *
 * Interface
 * ---------
 *
 *  - void *cudamem_dma_malloc(struct cudamem_heap *heap, size_t size);
 *    Allocate a block of device memory of the given size.
 *
 *  - void cudamem_dma_free(struct cudamem_heap *heap, void *ptr);
 *    Free a block previously returned by cudamem_dma_malloc().
 *
 *  - void *cudamem_dma_alloc_array(struct cudamem_heap *heap, size_t elem_count, size_t elem_size);
 *    Allocate an array of elements, guaranteed not to straddle device page boundaries.
 *
 *  - uint64_t cudamem_dma_v2p(struct cudamem_heap *heap, void *virt);
 *    Resolve a GPU virtual address to its physical address.
 *
 * Usage
 * -----
 *
 * You must call cudamem_heap_init() before any allocation and cudamem_heap_term()
 * after all memory has been freed.
 */

/**
 * Free device memory previously allocated by cudamem_dma_malloc() or
 * cudamem_dma_alloc_array().
 *
 * If ptr is NULL, no operation is performed.
 *
 * @param heap Pointer to the device memory heap the allocation belongs to.
 * @param ptr  Pointer previously returned by cudamem_dma_malloc() or cudamem_dma_alloc_array().
 */
static inline void
cudamem_dma_free(struct cudamem_heap *heap, void *ptr)
{
	cudamem_heap_block_free(heap, ptr);
}

/**
 * Allocate size bytes of device memory aligned to the host page size.
 *
 * @param heap Pointer to the device memory heap to allocate from.
 * @param size Number of bytes to allocate.
 * @return Pointer to the allocation on success, NULL with errno set on failure.
 */
static inline void *
cudamem_dma_malloc(struct cudamem_heap *heap, size_t size)
{
	if (!size) {
		errno = EINVAL;
		return NULL;
	}

	return cudamem_heap_block_alloc(heap, size);
}

/**
 * Allocate size bytes of device memory with a custom alignment.
 *
 * @param heap      Pointer to the device memory heap to allocate from.
 * @param size      Number of bytes to allocate.
 * @param alignment Boundary to align to.
 * @return Pointer to the allocation on success, NULL with errno set on failure.
 */
static inline void *
cudamem_dma_malloc_aligned(struct cudamem_heap *heap, size_t size, size_t alignment)
{
	return cudamem_heap_block_alloc_aligned(heap, size, alignment);
}

/**
 * Allocate elem_count * elem_size bytes of device memory.
 *
 * Elements are guaranteed not to straddle device page (64KB) boundaries,
 * matching the hugepage-boundary guarantee of hostmem_dma_alloc_array().
 *
 * elem_size must fit within a single device page. If the total size exceeds
 * one device page, elem_size must evenly divide the device page size.
 *
 * @param heap       Pointer to the device memory heap to allocate from.
 * @param elem_count Number of elements to allocate.
 * @param elem_size  Size in bytes of each element.
 * @return Pointer to the allocation on success, NULL with errno set on failure.
 */
static inline void *
cudamem_dma_alloc_array(struct cudamem_heap *heap, size_t elem_count, size_t elem_size)
{
	if (!elem_size || !elem_count) {
		errno = EINVAL;
		return NULL;
	}

	return cudamem_heap_block_alloc_array(heap, elem_count, elem_size);
}

/**
 * Resolve a GPU virtual address to its physical address.
 *
 * @param heap Pointer to the device memory heap the allocation belongs to.
 * @param virt Pointer previously returned by cudamem_dma_malloc() or cudamem_dma_alloc_array().
 * @return Physical address corresponding to the given virtual address.
 */
static inline uint64_t
cudamem_dma_v2p(struct cudamem_heap *heap, void *virt)
{
	return cudamem_heap_block_vtp(heap, virt);
}


