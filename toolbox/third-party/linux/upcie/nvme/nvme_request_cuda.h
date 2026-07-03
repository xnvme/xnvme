// SPDX-License-Identifier: BSD-3-Clause


/**
 * CUDA NVMe Request Extension
 * ===========================
 *
 * This header extends the functionality defined in the uPCIe NVMe Request header
 * `upcie/nvme/nvme_request.h` with a CUDA dependent PRP preparation function.
 * 
 * @file nvme_request_cuda.h
 * @version 0.5.1
 */

/**
 * Re-translate a PRP entry that crosses a device-page boundary.
 *
 * Kept out-of-line so the rare boundary path does not inline a vtp into the hot
 * contig builder; the common in-device-page case strides physically from PRP1,
 * and the inlined stride loop keeps large-I/O builds free of any per-entry call.
 */
static inline uint64_t __attribute__((noinline))
nvme_request_prp_retranslate_cuda(struct cudamem_heap *heap, void *virt)
{
	return cudamem_heap_block_vtp(heap, virt);
}

/**
 * Prepare the PRP entries for a command with a contiguous CUDA data buffer.
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
 * @param heap Pointer to the CUDA memory heap that `dbuf` is allocated within.
 * @param dbuf Pointer to the (virtually) contiguous data buffer to be described by PRPs.
 * @param dbuf_nbytes Size in bytes of the data buffer.
 * @param cmd Pointer to the NVMe command to be prepared with PRP entries.
 */
static inline void
nvme_request_prep_command_prps_contig_cuda(struct nvme_request *request, struct cudamem_heap *heap,
                                           void *dbuf, size_t dbuf_nbytes, struct nvme_command *cmd)
{
	const uint64_t pagesize = heap->config->pagesize;

	cmd->prp1 = cudamem_heap_block_vtp(heap, dbuf);

	/* Only PRP1 may carry a sub-page offset; the page count and every later
	 * entry are measured from the page floor. ceil((off+nbytes)/pagesize). Take
	 * the offset from the virtual address -- vtp preserves the sub-page offset --
	 * so the page count and the npages == 1 branch do not wait on the vtp load. */
	const uint64_t page_off = (uintptr_t)dbuf & (pagesize - 1);
	const uint64_t npages =
		(page_off + dbuf_nbytes + pagesize - 1) >> heap->config->pagesize_shift;

	/* Chaining is not supported, thus assert that the given dbuf fits. */
	assert(npages <= 1 + 512);

	if (npages == 1) {
		return;
	}

	/* The heap is virtually contiguous but physically contiguous only within a
	 * device page. Stride PRP entries physically from PRP1 while inside the same
	 * device page and re-translate when the running address crosses a device-page
	 * boundary. A buffer that fits within a single device page (the common case)
	 * never crosses, so this reduces to the stride of a contiguous buffer. */
	uint8_t *vbase = (uint8_t *)dbuf - page_off;
	const uint64_t dp_off =
		((uint64_t)vbase - heap->vaddr) & (heap->config->device_pagesize - 1);
	uint64_t strides_left =
		((heap->config->device_pagesize - dp_off) >> heap->config->pagesize_shift) - 1;
	uint64_t page_phys = cmd->prp1 - page_off;

	if (npages == 2) {
		cmd->prp2 = strides_left
				    ? page_phys + pagesize
				    : nvme_request_prp_retranslate_cuda(heap, vbase + pagesize);
		return;
	}

	uint64_t *prp_list = (uint64_t *)request->prp;

	cmd->prp2 = request->prp_addr;
	for (uint64_t i = 1; i < npages; ++i) {
		if (strides_left) {
			page_phys += pagesize;
			strides_left--;
		} else {
			page_phys = nvme_request_prp_retranslate_cuda(
				heap, vbase + (i << heap->config->pagesize_shift));
			strides_left =
				(heap->config->device_pagesize >> heap->config->pagesize_shift) - 1;
		}
		prp_list[i - 1] = page_phys;
	}
}

/**
 * Prepare the PRP list for a command with a CUDA iovec (scatter-gather) data buffer.
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
 * @param heap Pointer to the CUDA heap that iovec buffers are allocated within.
 * @param dvec Array of iovec structures describing the data segments.
 * @param dvec_cnt Number of elements in the dvec array.
 * @param cmd Pointer to the NVMe command to be prepared with PRP entries.
 */
static inline void
nvme_request_prep_command_prps_iov_cuda(struct nvme_request *request, struct cudamem_heap *heap,
				   	struct iovec *dvec, size_t dvec_cnt, struct nvme_command *cmd)
{
	const uint64_t pagesize = heap->config->pagesize;
	uint64_t *prp_list = (uint64_t *)request->prp;
	size_t prp_idx = 0;

	cmd->prp1 = cudamem_heap_block_vtp(heap, dvec[0].iov_base);

	for (size_t i = 0; i < dvec_cnt; ++i) {
		uint8_t *base = (uint8_t *)dvec[i].iov_base;
		size_t remaining = dvec[i].iov_len;
		size_t offset = 0;

		/* Skip the first page of the first iovec — it is PRP1 */
		if (i == 0) {
			offset = pagesize;
			remaining = (remaining > pagesize) ? remaining - pagesize : 0;
		}

		while (remaining > 0) {
			prp_list[prp_idx++] = cudamem_heap_block_vtp(heap, base + offset);

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
 * Prepare PRPs for a contiguous CUDA data buffer registered with a mapping
 * registry.
 *
 * The buffer is resolved page-by-page through the registry's chunk cache,
 * with no contiguity assumption. This variant is for buffers registered via
 * cudamem_mapping; heap-allocated buffers should use
 * nvme_request_prep_command_prps_contig_cuda() instead.
 *
 * Caveats
 * -------
 *
 * - Only PRP1 may carry a sub-page offset; the page count and every later
 *   entry are resolved from the page floor (entries past PRP1 must be
 *   page-aligned per spec).
 * - Does *not* support PRP list chaining; only a single list page is constructed.
 *
 * @param request Pointer to the NVMe request context used for tracking and metadata.
 * @param registry Mapping registry to resolve through.
 * @param config Device memory config (for host page size).
 * @param dbuf Pointer to the contiguous data buffer to be described by PRPs.
 * @param dbuf_nbytes Size in bytes of the data buffer.
 * @param cmd Pointer to the NVMe command to be prepared with PRP entries.
 * @return 0 on success, -EINVAL if any page of the buffer is unmapped.
 */
static inline int
nvme_request_prep_command_prps_contig_cuda_mapped(struct nvme_request *request,
						  struct cudamem_mapping_registry *registry,
						  struct cudamem_config *config, void *dbuf,
						  size_t dbuf_nbytes, struct nvme_command *cmd)
{
	const uint64_t pagesize = config->pagesize;
	const uint64_t pagesize_shift = config->pagesize_shift;
	const size_t prp_cap = pagesize / sizeof(uint64_t);
	const uint64_t page_off = (uintptr_t)dbuf & (pagesize - 1);
	uint8_t *page_base = (uint8_t *)dbuf - page_off;
	const uint64_t npages = (page_off + dbuf_nbytes + pagesize - 1) >> pagesize_shift;
	int err;

	if (npages > 1 + prp_cap) {
		return -EINVAL;
	}

	/* virt_to_phys preserves the sub-page offset, so PRP1 carries it. */
	err = cudamem_mapping_virt_to_phys(registry, dbuf, &cmd->prp1);
	if (err) {
		return err;
	}

	if (npages == 1) {
		return 0;
	}
	if (npages == 2) {
		return cudamem_mapping_virt_to_phys(registry, page_base + pagesize, &cmd->prp2);
	}

	uint64_t *prp_list = (uint64_t *)request->prp;
	cmd->prp2 = request->prp_addr;
	for (uint64_t i = 1; i < npages; ++i) {
		err = cudamem_mapping_virt_to_phys(registry, page_base + (i << pagesize_shift),
						   &prp_list[i - 1]);
		if (err) {
			return err;
		}
	}
	return 0;
}

/**
 * Prepare PRPs for a CUDA iovec (scatter-gather) data buffer registered with
 * a mapping registry.
 *
 * Each entry is resolved page-by-page through the registry's chunk cache.
 * This variant is for iovecs registered via cudamem_mapping; heap-allocated
 * iovecs should use nvme_request_prep_command_prps_iov_cuda() instead.
 *
 * Each `dvec[i].iov_base` must be host-page-aligned (every PRP-list entry
 * must be page-aligned per NVMe spec); asserted.
 *
 * @param request Pointer to the NVMe request context used for tracking and metadata.
 * @param registry Mapping registry to resolve through.
 * @param config Device memory config (for host page size).
 * @param dvec Array of iovec structures describing the data segments.
 * @param dvec_cnt Number of elements in the dvec array.
 * @param cmd Pointer to the NVMe command to be prepared with PRP entries.
 * @return 0 on success, -EINVAL if any iovec cannot be resolved.
 */
static inline int
nvme_request_prep_command_prps_iov_cuda_mapped(struct nvme_request *request,
					       struct cudamem_mapping_registry *registry,
					       struct cudamem_config *config, struct iovec *dvec,
					       size_t dvec_cnt, struct nvme_command *cmd)
{
	if (dvec_cnt == 0) {
		return -EINVAL;
	}

	const uint64_t pagesize = config->pagesize;
	const size_t prp_cap = pagesize / sizeof(uint64_t);
	uint64_t *prp_list = (uint64_t *)request->prp;
	size_t prp_idx = 0;
	int err;

	for (size_t i = 0; i < dvec_cnt; ++i) {
		uint8_t *base = (uint8_t *)dvec[i].iov_base;
		size_t iov_len = dvec[i].iov_len;
		size_t offset = 0;
		size_t remaining = iov_len;

		assert(((uintptr_t)base & (pagesize - 1)) == 0);

		if (i == 0) {
			err = cudamem_mapping_virt_to_phys(registry, base, &cmd->prp1);
			if (err) {
				return err;
			}
			offset = pagesize;
			remaining = (remaining > pagesize) ? remaining - pagesize : 0;
		}

		while (remaining > 0) {
			if (prp_idx >= prp_cap) {
				return -EINVAL;
			}
			err = cudamem_mapping_virt_to_phys(registry, base + offset,
							   &prp_list[prp_idx++]);
			if (err) {
				return err;
			}
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
