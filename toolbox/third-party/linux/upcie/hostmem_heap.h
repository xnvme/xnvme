// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Heap-based memory allocator backed by hugepages for DMA in user-space drivers
 * =============================================================================
 *
 * - hostmem_heap_init() / hostmem_heap_term()
 * - hostmem_heap_block_alloc() / hostmem_heap_block_alloc_aligned() / hostmem_heap_block_free()
 * - hostmem_heap_block_virt_to_phys()
 *
 * Caveat: system setup
 * --------------------
 *
 * The library makes use of memfd_create(MFD_HUGETLB), however, you still need to allocate them
 * yourself. That is, have a system setup step than makes hugepages available, such as:
 *
 *    echo 128 | tee -a /proc/sys/vm/nr_hugepages
 *    ulimit -l unlimited
 *
 * Thus, a utility for this similar to devbind.py is needed. This is what we have today with
 * 'xnvme-driver', however, we want something simpler.
 *
 * Caveat: CAP_SYS_ADMIN
 * ---------------------
 *
 * Reading /proc/self/pagemap requires CAP_SYS_ADMIN, so hostmem_virt_to_phys() cannot be used by
 * non-privileged users. Therefore, any process needing DMA via this allocator must run as root.
 *
 * Possible Workaround: Since the allocator uses MAP_SHARED, a privileged "allocator-daemon" could
 * handle virt_to_phys translations and share the results via shared memory with unprivileged
 * clients. This allows integration into the heap with minimal complexity. Example:
 *
 * After heap initialization, write the heap structure into hugepage memory. Because phys_lut[]
 * resolves all physical addresses of the backing hugepages, any process that imports the hugepage
 * also gains access to those physical addresses—without needing CAP_SYS_ADMIN.
 *
 * @file hostmem.h
 * @version 0.3.2
 */

/**
 * Representation of a memory-allocation as produced by
 * hostmem_buffer_alloc(...)
 */
struct hostmem_heap_block {
	size_t size;
	int free;
	struct hostmem_heap_block *next;
};

/**
 * A pre-allocated heap providing memory for a buffer-allocator
 */
struct hostmem_heap {
	struct hostmem_hugepage memory; ///< A hugepage-allocation; can span multiple hugepages
	struct hostmem_heap_block
		*freelist;             ///< Pointers to description of free memory in the heap
	struct hostmem_config *config; ///< Pointer to hugepage configuration
	size_t nphys;                  ///< Number of hugepages backing 'memory'
	uint64_t *phys_lut; ///< An array of physical addresses; on for each hugepage in 'memory'
};

static inline int
hostmem_heap_pp(struct hostmem_heap *heap)
{
	int wrtn = 0;

	wrtn += printf("hostmem_heap:");

	if (!heap) {
		wrtn += printf(" ~\n");
		return 0;
	}

	wrtn += printf("\n");

	wrtn += printf("  nphys: '%zu'\n", heap->nphys);
	wrtn += printf("  phys:\n");
	for (size_t i = 0; i < heap->nphys; ++i) {
		wrtn += printf("  - 0x%" PRIx64 "\n", heap->phys_lut[i]);
	}

	wrtn += printf("  freelist:\n");
	for (struct hostmem_heap_block *block = heap->freelist; block; block = block->next) {
		wrtn += printf("  - {size: %zu, free: %d}\n", block->size, block->free);
	}

	wrtn += hostmem_hugepage_pp(&heap->memory);

	return wrtn;
}

static inline void
hostmem_heap_term(struct hostmem_heap *heap)
{
	if (!heap) {
		return;
	}

	free(heap->phys_lut);
	hostmem_hugepage_free(&heap->memory);
}

/**
 * Initialize the given heap
 *
 * - Pre-allocate a va-space of 'size' bytes backend by hugepage(s)
 * - Setup the LUT / physical address for hugepage backing the va-space
 *
 * TODO: use the hugepage memory for the heap-description! By doing so, then a helper process can
 *       do all the hugepage lookup-work, and then anybody who imports it, will be able to, as a
 *       non-privileged user to know of the virt-to-phys mapping
 */
static inline int
hostmem_heap_init(struct hostmem_heap *heap, size_t size, struct hostmem_config *config)
{
	int err;

	if (!heap) {
		return -EINVAL;
	}

	memset(heap, 0, sizeof(*heap));
	heap->config = config;

	err = hostmem_hugepage_alloc(size, &heap->memory, config);
	if (err) {
		return err;
	}

	// Initialize a single free block spanning the entire heap
	heap->freelist = (struct hostmem_heap_block *)heap->memory.virt;
	heap->freelist->size = size;
	heap->freelist->free = 1;
	heap->freelist->next = NULL;

	// Setup the LUT
	heap->nphys = size / heap->config->hugepgsz;
	heap->phys_lut = calloc(heap->nphys, sizeof(uint64_t));

	for (size_t i = 0; i < heap->nphys; ++i) {
		void *vaddr = (char *)heap->memory.virt + i * heap->config->hugepgsz;

		err = hostmem_pagemap_virt_to_phys(vaddr, &heap->phys_lut[i]);
		if (err) {
			hostmem_heap_term(heap);
			return err;
		}
	}

	if (heap->memory.phys != heap->phys_lut[0]) {
		hostmem_heap_term(heap);
		return -ENOMEM;
	}

	return 0;
}

static inline void
hostmem_heap_block_free(struct hostmem_heap *heap, void *ptr)
{
	size_t alignment = heap->config->pagesize;
	struct hostmem_heap_block *block = NULL;

	if (!ptr) {
		return;
	}

	block = (struct hostmem_heap_block *)((char *)ptr - alignment);
	block->free = 1;

	block = heap->freelist;
	while (block && block->next) {
		if (block->free && block->next->free) {
			block->size += alignment + block->next->size;
			block->next = block->next->next;
		} else {
			block = block->next;
		}
	}
}

static inline void *
hostmem_heap_block_alloc_aligned(struct hostmem_heap *heap, size_t size, size_t alignment)
{
	struct hostmem_heap_block *block = heap->freelist;

	assert(sizeof(*block) < alignment);

	size = (size + alignment - 1) & ~(alignment - 1);

	while (block) {
		if (block->free && block->size >= (size + alignment)) {
			size_t remaining = block->size - size - alignment;

			if (remaining > sizeof(*block)) {
				struct hostmem_heap_block *newblock;

				newblock = (void *)((char *)block + alignment + size);
				newblock->size = remaining;
				newblock->free = 1;
				newblock->next = block->next;

				block->next = newblock;
				block->size = size;
			}

			block->free = 0;

			return (char *)block + alignment;
		}
		block = block->next;
	}

	errno = ENOMEM;
	return NULL;
}

static inline void *
hostmem_heap_block_alloc(struct hostmem_heap *heap, size_t size)
{
	return hostmem_heap_block_alloc_aligned(heap, size, heap->config->pagesize);
}

static inline int
hostmem_heap_block_virt_to_phys(struct hostmem_heap *heap, void *virt, uint64_t *phys)
{
	size_t offset, hpage_idx, in_hpage_offset;

	if (!heap || !heap->phys_lut || !virt || !phys) {
		return -EINVAL;
	}

	if ((char *)virt < (char *)heap->memory.virt ||
	    (char *)virt >= (char *)heap->memory.virt + heap->memory.size) {
		return -EINVAL;
	}

	// Compute byte offset from base of heap
	offset = (char *)virt - (char *)heap->memory.virt;

	// Determine which hugepage this address falls into
	hpage_idx = offset / heap->config->hugepgsz;

	// Offset within that hugepage
	in_hpage_offset = offset % heap->config->hugepgsz;

	if (hpage_idx >= heap->nphys) {
		return -EINVAL;
	}

	*phys = heap->phys_lut[hpage_idx] + in_hpage_offset;

	return 0;
}

/**
 * Same as hostmem_buffer_virt_to_phys() but without any error-handling, thus return the phys
 * address instead of error
 */
static inline uint64_t
hostmem_heap_block_vtp(struct hostmem_heap *heap, void *virt)
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
