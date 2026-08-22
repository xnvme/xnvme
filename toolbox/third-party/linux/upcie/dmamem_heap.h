// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * dmamem_heap: sub-allocator over a dmamem range
 * ==============================================
 *
 * A minimal offset-space allocator over the (base_iova, size) window of a
 * dmamem. Block metadata lives out-of-band in host memory (a malloc'd
 * freelist), not embedded in the dmamem range itself, so the same
 * allocator works for backings that are not CPU-mappable (VRAM, MMIO).
 *
 * Convenience accessors return the CPU VA of an allocation when
 * available (memfd backing) and always return the IOVA (all backings).
 *
 * @file dmamem_heap.h
 * @version 0.6.0
 */

struct dmamem_heap_block {
	size_t offset;
	size_t size;
	int free;
	struct dmamem_heap_block *next;
};

struct dmamem_heap {
	struct dmamem *dmem;                 ///< Not owned; caller owns lifetime
	struct dmamem_heap_block *freelist;  ///< Head of the freelist (offset-ordered)
	size_t alignment;                    ///< Default allocation alignment in bytes
};

static inline int
dmamem_heap_pp(struct dmamem_heap *heap)
{
	int wrtn = 0;

	wrtn += printf("dmamem_heap:");

	if (!heap) {
		wrtn += printf(" ~\n");
		return 0;
	}

	wrtn += printf("\n");
	wrtn += printf("  alignment: %zu\n", heap->alignment);
	wrtn += printf("  freelist:\n");
	for (struct dmamem_heap_block *b = heap->freelist; b; b = b->next) {
		wrtn += printf("  - {offset: %zu, size: %zu, free: %d}\n", b->offset, b->size,
			       b->free);
	}

	return wrtn;
}

/**
 * Initialise a heap over the given dmamem.
 *
 * The dmamem must already be constructed and mapped. The heap does not
 * take ownership of dmem; the caller's dmamem_destroy is what releases
 * the memory.
 */
static inline int
dmamem_heap_init(struct dmamem_heap *heap, struct dmamem *dmem, size_t alignment)
{
	struct dmamem_heap_block *block;

	if (!heap || !dmem || !alignment) {
		return -EINVAL;
	}

	memset(heap, 0, sizeof(*heap));
	heap->dmem = dmem;
	heap->alignment = alignment;

	block = calloc(1, sizeof(*block));
	if (!block) {
		return -ENOMEM;
	}
	block->offset = 0;
	block->size = dmem->size;
	block->free = 1;
	block->next = NULL;

	heap->freelist = block;

	return 0;
}

/**
 * Release the freelist. The underlying dmamem is not touched.
 */
static inline void
dmamem_heap_term(struct dmamem_heap *heap)
{
	struct dmamem_heap_block *b;

	if (!heap) {
		return;
	}

	b = heap->freelist;
	while (b) {
		struct dmamem_heap_block *next = b->next;
		free(b);
		b = next;
	}

	memset(heap, 0, sizeof(*heap));
}

/**
 * The span within which the heap can promise contiguous DMA addresses, or 0
 * when the whole dmamem is one contiguous range.
 */
static inline size_t
dmamem_heap_granule(struct dmamem_heap *heap)
{
	if (!heap || !heap->dmem || DMAMEM_XLATE_LUT != heap->dmem->translator) {
		return 0;
	}

	return heap->dmem->registry.gran_mask + 1;
}

/**
 * Whether any single element would cross a granule boundary.
 */
static inline int
dmamem_heap_layout_tears(size_t start, size_t elem_count, size_t elem_size, size_t granule)
{
	size_t i;

	if (!granule) {
		return 0;
	}

	/* Fits in one granule, or sits on a grid the granule divides evenly. */
	if ((start & (granule - 1)) + (elem_count * elem_size) <= granule) {
		return 0;
	}
	if (!(granule % elem_size) && !(start % elem_size)) {
		return 0;
	}

	for (i = 0; i < elem_count; ++i) {
		const size_t off = (start + (i * elem_size)) & (granule - 1);

		if (off + elem_size > granule) {
			return 1;
		}
	}

	return 0;
}

/**
 * Allocate elem_count elements of elem_size such that no single element
 * crosses a granule boundary. The array itself may span granules.
 *
 * Ask for one element to get a region contiguous in device addresses.
 *
 * @return 0 on success; -ENOMEM if the heap is full or fragmented past the
 * requested size; -EINVAL on input error, or when the promise is impossible,
 * which is an element larger than a granule, or an array spanning granules
 * whose element size does not divide one.
 */
static inline int
dmamem_heap_alloc_array_aligned(struct dmamem_heap *heap, size_t elem_count, size_t elem_size,
				size_t alignment, size_t *offset_out)
{
	const size_t granule = dmamem_heap_granule(heap);
	struct dmamem_heap_block *b, *tail;
	size_t size;

	if (!heap || !elem_count || !elem_size || !alignment || !offset_out) {
		return -EINVAL;
	}
	if (elem_count > SIZE_MAX / elem_size) {
		return -EINVAL;
	}

	size = elem_count * elem_size;

	/* Raising the alignment is what enforces the promise: on a grid the
	 * granule divides evenly no element can straddle, and failing that the
	 * whole array has to land inside one granule. */
	if (granule) {
		size_t required = alignment;

		if (elem_size > granule) {
			UPCIE_DEBUG("FAILED: elem_size(%zu) exceeds granule(%zu); no contiguity "
				    "beyond a granule can be promised",
				    elem_size, granule);
			return -EINVAL;
		} else if (!(granule % elem_size)) {
			required = elem_size;
		} else if (size <= granule) {
			required = 1;
			while (required < size) {
				required <<= 1;
			}
		} else {
			UPCIE_DEBUG("FAILED: elem_size(%zu) does not divide granule(%zu); an "
				    "array spanning granules would tear",
				    elem_size, granule);
			return -EINVAL;
		}

		if (required > alignment) {
			alignment = required;
		}
	}

	for (b = heap->freelist; b; b = b->next) {
		size_t aligned_offset, front_gap, remaining;

		if (!b->free) {
			continue;
		}

		aligned_offset = (b->offset + alignment - 1) & ~(alignment - 1);
		front_gap = aligned_offset - b->offset;

		if (b->size < front_gap + size) {
			continue;
		}

		remaining = b->size - front_gap - size;

		/* Carve off the front gap if any. */
		if (front_gap) {
			struct dmamem_heap_block *fg = calloc(1, sizeof(*fg));
			if (!fg) {
				return -ENOMEM;
			}
			fg->offset = b->offset;
			fg->size = front_gap;
			fg->free = 1;
			fg->next = b;
			/* Rewire predecessor into fg. */
			if (heap->freelist == b) {
				heap->freelist = fg;
			} else {
				struct dmamem_heap_block *p = heap->freelist;
				while (p->next != b) {
					p = p->next;
				}
				p->next = fg;
			}
			b->offset = aligned_offset;
			b->size -= front_gap;
		}

		/* Carve off the tail if any. */
		if (remaining) {
			tail = calloc(1, sizeof(*tail));
			if (!tail) {
				return -ENOMEM;
			}
			tail->offset = b->offset + size;
			tail->size = remaining;
			tail->free = 1;
			tail->next = b->next;
			b->next = tail;
			b->size = size;
		}

		b->free = 0;
		*offset_out = b->offset;

		assert(!dmamem_heap_layout_tears(*offset_out, elem_count, elem_size, granule));

		return 0;
	}

	return -ENOMEM;
}

/**
 * Allocate an array using the heap's default alignment.
 */
static inline int
dmamem_heap_alloc_array(struct dmamem_heap *heap, size_t elem_count, size_t elem_size,
			size_t *offset_out)
{
	return dmamem_heap_alloc_array_aligned(heap, elem_count, elem_size, heap->alignment,
					       offset_out);
}

/**
 * Allocate a sub-range of the heap with an explicit alignment.
 *
 * On success, *offset_out is the offset of the allocation within the
 * dmamem, aligned up to `alignment` bytes.
 *
 * NOTE: The range is taken as an array of heap-alignment sized elements, so it
 * may cross granule boundaries and is not necessarily contiguous in device
 * addresses. Memory a device walks from a single base must come from
 * dmamem_heap_alloc_array_aligned() instead.
 *
 * @return 0 on success; -ENOMEM if the heap is full or fragmented past
 * the requested size; negative errno on input error.
 */
static inline int
dmamem_heap_alloc_aligned(struct dmamem_heap *heap, size_t size, size_t alignment,
			  size_t *offset_out)
{
	size_t elem_size, elem_count;

	if (!heap || !size || !heap->alignment) {
		return -EINVAL;
	}

	elem_size = heap->alignment;
	elem_count = (size + elem_size - 1) / elem_size;

	return dmamem_heap_alloc_array_aligned(heap, elem_count, elem_size, alignment, offset_out);
}

/**
 * Allocate a sub-range of the heap using the heap's default alignment.
 *
 * @return 0 on success; -ENOMEM if the heap is full or fragmented past
 * the requested size; negative errno on input error.
 */
static inline int
dmamem_heap_alloc(struct dmamem_heap *heap, size_t size, size_t *offset_out)
{
	return dmamem_heap_alloc_aligned(heap, size, heap->alignment, offset_out);
}

/**
 * Free a previously-allocated range and coalesce with adjacent free blocks.
 */
static inline void
dmamem_heap_free(struct dmamem_heap *heap, size_t offset)
{
	struct dmamem_heap_block *b, *prev = NULL;

	if (!heap) {
		return;
	}

	for (b = heap->freelist; b; prev = b, b = b->next) {
		if (b->offset == offset) {
			break;
		}
	}
	if (!b) {
		UPCIE_DEBUG("FAILED: no block at offset(%zu)", offset);
		return;
	}

	b->free = 1;

	/* Coalesce with the successor if free and contiguous. */
	if (b->next && b->next->free && b->offset + b->size == b->next->offset) {
		struct dmamem_heap_block *n = b->next;
		b->size += n->size;
		b->next = n->next;
		free(n);
	}

	/* Coalesce with the predecessor if free and contiguous. */
	if (prev && prev->free && prev->offset + prev->size == b->offset) {
		prev->size += b->size;
		prev->next = b->next;
		free(b);
	}
}

/**
 * Return the address of an allocation in the space the dmamem describes.
 *
 * A CPU pointer for host-backed dmamems, a device pointer for GPU-backed
 * ones; dereference only when dmem->cpu_va is set.
 */
static inline void *
dmamem_heap_at_va(struct dmamem_heap *heap, size_t offset)
{
	if (!heap || !heap->dmem || !heap->dmem->base_va) {
		return NULL;
	}
	return (char *)heap->dmem->base_va + offset;
}

/**
 * Return the IOVA of an allocation.
 *
 * Delegates to dmamem_offset_to_iova so the LUT translator resolves
 * correctly. In the arithmetic case this remains base_iova + offset.
 */
static inline uint64_t
dmamem_heap_at_iova(struct dmamem_heap *heap, size_t offset)
{
	return dmamem_offset_to_iova(heap->dmem, offset);
}
