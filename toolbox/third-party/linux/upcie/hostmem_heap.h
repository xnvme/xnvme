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
 * @version 0.9.0
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

	/*
	 * heap->phys_lut is a borrowed pointer to heap->memory.phys_lut;
	 * hostmem_hugepage_free releases the backing array.
	 */
	heap->phys_lut = NULL;
	heap->nphys = 0;
	hostmem_hugepage_free(&heap->memory);
}

/**
 * The heap's own translation table, at its offset zero
 *
 * Whoever allocates the memory can read the physical addresses; whoever
 * imports it often cannot, since pagemap wants CAP_SYS_ADMIN. Writing the
 * table into the memory it describes means an importer inherits the work
 * rather than repeating it, and it gives a client somewhere to find the
 * runtime record without being told an offset out of band.
 */
struct hostmem_heap_hdr {
	uint32_t magic;         ///< HOSTMEM_HEAP_HDR_MAGIC
	uint32_t version;       ///< HOSTMEM_HEAP_HDR_VERSION
	uint64_t nbytes;        ///< Size of the whole heap, header included
	uint64_t data_offset;   ///< First allocatable byte
	uint64_t record_offset; ///< Set by the server; zero when it has set nothing
	uint32_t nphys;         ///< Entries in phys[]
	uint32_t _rsvd;
	uint64_t phys[]; ///< Physical base of each hugepage backing the heap
};

#define HOSTMEM_HEAP_HDR_MAGIC 0x50414548U ///< "HEAP"
#define HOSTMEM_HEAP_HDR_VERSION 1U

/**
 * Where allocation starts, given a header of this many entries
 */
static inline size_t
hostmem_heap_hdr_nbytes(size_t nphys, size_t alignment)
{
	size_t nbytes = sizeof(struct hostmem_heap_hdr) + (nphys * sizeof(uint64_t));

	return ((nbytes + alignment - 1) / alignment) * alignment;
}

/**
 * The heap's header
 */
static inline struct hostmem_heap_hdr *
hostmem_heap_hdr_get(const struct hostmem_heap *heap)
{
	return (struct hostmem_heap_hdr *)heap->memory.virt;
}

/**
 * The descriptor backing the heap, for handing to another process
 */
static inline int
hostmem_heap_fd(const struct hostmem_heap *heap)
{
	return heap ? heap->memory.fd : -1;
}

/**
 * Initialize the given heap
 *
 * - Pre-allocate a va-space of 'size' bytes backend by hugepage(s)
 * - Setup the LUT / physical address for hugepage backing the va-space
 * - Write the header, so a process that attaches can translate without pagemap
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

	{
		struct hostmem_heap_hdr *hdr = (struct hostmem_heap_hdr *)heap->memory.virt;
		size_t hdr_nbytes;

		if (!heap->memory.phys_lut) {
			hostmem_heap_term(heap);
			return -EPERM;
		}

		hdr_nbytes = hostmem_heap_hdr_nbytes(heap->memory.nphys, config->pagesize);
		if (hdr_nbytes >= size) {
			hostmem_heap_term(heap);
			return -ENOMEM;
		}

		memset(hdr, 0, hdr_nbytes);
		hdr->magic = HOSTMEM_HEAP_HDR_MAGIC;
		hdr->version = HOSTMEM_HEAP_HDR_VERSION;
		hdr->nbytes = size;
		hdr->data_offset = hdr_nbytes;
		hdr->nphys = (uint32_t)heap->memory.nphys;
		memcpy(hdr->phys, heap->memory.phys_lut, heap->memory.nphys * sizeof(*hdr->phys));

		// A single free block spanning everything after the header
		heap->freelist =
			(struct hostmem_heap_block *)((char *)heap->memory.virt + hdr_nbytes);
		heap->freelist->size = size - hdr_nbytes;
		heap->freelist->free = 1;
		heap->freelist->next = NULL;
	}

	/*
	 * The LUT is now populated on hostmem_hugepage by hostmem_hugepage_alloc.
	 * heap->phys_lut is a borrowed pointer for backwards compatibility with
	 * existing hostmem_dma_v2p callers.
	 */
	if (!heap->memory.phys_lut) {
		hostmem_heap_term(heap);
		return -EPERM;
	}
	heap->nphys = heap->memory.nphys;
	heap->phys_lut = heap->memory.phys_lut;

	if (heap->memory.phys != heap->phys_lut[0]) {
		hostmem_heap_term(heap);
		return -ENOMEM;
	}

	return 0;
}

/**
 * Attach to a heap another process allocated, by descriptor
 *
 * The attached heap can translate and can resolve offsets; it cannot allocate,
 * because the free list is a chain of addresses in the server's mapping and has
 * no lock. Allocation stays with whoever created the memory.
 *
 * @param heap Pre-allocated heap to fill in
 * @param fd Descriptor from hostmem_heap_fd() in the owning process
 * @param config Pointer to the hugepage configuration
 *
 * @return 0 on success, negative errno on error
 */
static inline int
hostmem_heap_attach(struct hostmem_heap *heap, int fd, struct hostmem_config *config)
{
	struct hostmem_heap_hdr *hdr;
	int err;

	if (!heap || (fd < 0)) {
		return -EINVAL;
	}

	memset(heap, 0, sizeof(*heap));
	heap->config = config;

	err = hostmem_hugepage_import_fd(fd, &heap->memory, config, 0);
	if (err) {
		return err;
	}

	hdr = hostmem_heap_hdr_get(heap);
	if (hdr->magic != HOSTMEM_HEAP_HDR_MAGIC) {
		UPCIE_DEBUG("FAILED: heap magic(0x%x)", hdr->magic);
		hostmem_hugepage_free(&heap->memory);
		return -EPROTO;
	}
	if (hdr->version != HOSTMEM_HEAP_HDR_VERSION) {
		UPCIE_DEBUG("FAILED: heap version(%u), expected(%u)", hdr->version,
			    HOSTMEM_HEAP_HDR_VERSION);
		hostmem_hugepage_free(&heap->memory);
		return -EPROTO;
	}
	if (hdr->nbytes != heap->memory.size) {
		UPCIE_DEBUG("FAILED: heap says %llu bytes, mapping is %zu",
			    (unsigned long long)hdr->nbytes, heap->memory.size);
		hostmem_hugepage_free(&heap->memory);
		return -EINVAL;
	}

	/* The table describes the memory it lives in, so it is valid in this
	 * mapping too, and nothing here reads pagemap. */
	heap->nphys = hdr->nphys;
	heap->phys_lut = hdr->phys;
	heap->memory.phys = hdr->phys[0];
	heap->freelist = NULL; ///< Not ours to allocate from

	return 0;
}

/**
 * Release a heap attached with hostmem_heap_attach()
 *
 * @param heap A heap from hostmem_heap_attach
 */
static inline void
hostmem_heap_detach(struct hostmem_heap *heap)
{
	if (!heap) {
		return;
	}

	if (heap->memory.virt) {
		munmap(heap->memory.virt, heap->memory.size);
	}
	if (heap->memory.fd >= 0) {
		close(heap->memory.fd);
	}
	memset(heap, 0, sizeof(*heap));
}

/**
 * Point clients at a record the server placed in the heap
 *
 * @param heap A heap from hostmem_heap_init
 * @param offset Offset of the record, or zero to clear it
 */
static inline void
hostmem_heap_record_set(struct hostmem_heap *heap, uint64_t offset)
{
	hostmem_heap_hdr_get(heap)->record_offset = offset;
}

/**
 * The offset the server recorded, or zero when it recorded none
 *
 * @param heap A heap from hostmem_heap_init or hostmem_heap_attach
 */
static inline uint64_t
hostmem_heap_record_get(const struct hostmem_heap *heap)
{
	return hostmem_heap_hdr_get(heap)->record_offset;
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
hostmem_heap_block_alloc_array_aligned(struct hostmem_heap *heap, size_t elem_count,
				       size_t elem_size, size_t alignment)
{
	struct hostmem_heap_block *block = heap->freelist;
	size_t total_size;

	if (elem_count > SIZE_MAX / elem_size) {
		UPCIE_DEBUG("FAILED: Cannot allocate memory; elem_size(%ld) * elem_count(%ld) too "
			    "large",
			    elem_size, elem_count);
		errno = EINVAL;
		return NULL;
	}

	total_size = elem_count * elem_size;

	assert(sizeof(*block) < alignment);

	if (elem_size > (size_t)heap->config->hugepgsz ||
	    (total_size > (size_t)heap->config->hugepgsz &&
	     (size_t)heap->config->hugepgsz % elem_size != 0)) {
		UPCIE_DEBUG("FAILED: Cannot allocate memory; elem_size(%ld) must be aligned to "
			    "hugepage size(%d)",
			    elem_size, heap->config->hugepgsz);
		errno = ENOMEM;
		return NULL;
	}

	while (block) {
		if (block->free && block->size >= (total_size + alignment)) {
			struct hostmem_heap_block *newblock;
			size_t offset, in_hpage_offset, hpage_remaining, disalignment, remaining;

			offset = (char *)block - (char *)heap->memory.virt;
			in_hpage_offset = offset % heap->config->hugepgsz;
			hpage_remaining = heap->config->hugepgsz - in_hpage_offset;
			disalignment = (hpage_remaining - alignment) % elem_size;

			if (hpage_remaining < (total_size + alignment) && disalignment) {
				// Not enough room in hpage and allocating here will cause
				// disalignment with hpages - create a block to ensure alignment
				newblock = (void *)((char *)block + alignment + disalignment);
				newblock->size = block->size - alignment - disalignment;
				newblock->free = 1;
				newblock->next = block->next;

				block->next = newblock;
				block->size = disalignment;

				// Skip forward
				block = block->next;
				continue;
			}

			remaining = block->size - total_size - alignment;

			if (remaining > sizeof(*block)) {
				newblock = (void *)((char *)block + alignment + total_size);
				newblock->size = remaining;
				newblock->free = 1;
				newblock->next = block->next;

				block->next = newblock;
				block->size = total_size;
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
hostmem_heap_block_alloc_array(struct hostmem_heap *heap, size_t elem_count, size_t elem_size)
{
	return hostmem_heap_block_alloc_array_aligned(heap, elem_count, elem_size,
						      heap->config->pagesize);
}

static inline void *
hostmem_heap_block_alloc_aligned(struct hostmem_heap *heap, size_t size, size_t alignment)
{
	size_t elem_count, total_size, elem_size;

	total_size = (size + heap->config->pagesize - 1) & ~(heap->config->pagesize - 1);
	elem_count = total_size / heap->config->pagesize;
	elem_size = heap->config->pagesize;

	return hostmem_heap_block_alloc_array_aligned(heap, elem_count, elem_size, alignment);
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
	assert(hpage_idx < heap->nphys);

	// Offset within that hugepage
	in_hpage_offset = offset % heap->config->hugepgsz;

	return heap->phys_lut[hpage_idx] + in_hpage_offset;
}
