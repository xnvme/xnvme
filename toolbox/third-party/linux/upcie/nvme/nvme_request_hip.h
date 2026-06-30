// SPDX-License-Identifier: BSD-3-Clause


/**
 * HIP NVMe Request Extension
 * ===========================
 *
 * This header extends the functionality defined in the uPCIe NVMe Request header
 * `upcie/nvme/nvme_request.h` with a HIP dependent PRP preparation function.
 */

/**
 * Prepare the PRP list for a command with a contiguous HIP data buffer.
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
 * @param heap Pointer to the HIP memory heap that dbuf is allocated within.
 * @param dbuf Pointer to the contiguous data buffer to be described by PRPs.
 * @param dbuf_nbytes Size in bytes of the data buffer.
 * @param cmd Pointer to the NVMe command to be prepared with PRP entries.
 */
static inline void
nvme_request_prep_command_prps_contig_hip(struct nvme_request *request, struct hipmem_heap *heap,
                                           void *dbuf, size_t dbuf_nbytes, struct nvme_command *cmd)
{
	const uint64_t pagesize = heap->config->pagesize;

	cmd->prp1 = hipmem_heap_block_vtp(heap, dbuf);

	/* Only PRP1 may carry a sub-page offset; the page count and every later
	 * entry are measured from the page floor. ceil((off+nbytes)/pagesize). */
	const uint64_t page_off = cmd->prp1 & (pagesize - 1);
	const uint64_t page_base = cmd->prp1 - page_off;
	const uint64_t npages =
		(page_off + dbuf_nbytes + pagesize - 1) >> heap->config->pagesize_shift;

	/* Chaining is not supported, thus assert that the given dbuf fits. */
	assert(npages <= 1 + 512);

	if (npages == 1) {
		return;
	} else if (npages == 2) {
		cmd->prp2 = page_base + pagesize;
	} else {
		uint64_t *prp_list = (uint64_t *)request->prp;

		cmd->prp2 = request->prp_addr;
		for (uint64_t i = 1; i < npages; ++i) {
			prp_list[i - 1] = page_base + (i << heap->config->pagesize_shift);
		}
	}
}

/**
 * Prepare the PRP list for a command with a HIP iovec (scatter-gather) data buffer.
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
 * @param heap Pointer to the HIP heap that iovec buffers are allocated within.
 * @param dvec Array of iovec structures describing the data segments.
 * @param dvec_cnt Number of elements in the dvec array.
 * @param cmd Pointer to the NVMe command to be prepared with PRP entries.
 */
static inline void
nvme_request_prep_command_prps_iov_hip(struct nvme_request *request, struct hipmem_heap *heap,
				   	struct iovec *dvec, size_t dvec_cnt, struct nvme_command *cmd)
{
	const uint64_t pagesize = heap->config->pagesize;
	uint64_t *prp_list = (uint64_t *)request->prp;
	size_t prp_idx = 0;

	cmd->prp1 = hipmem_heap_block_vtp(heap, dvec[0].iov_base);

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
			prp_list[prp_idx++] = hipmem_heap_block_vtp(heap, base + offset);

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
 * Prepare PRPs for a contiguous HIP data buffer registered with a mapping
 * registry.
 *
 * The buffer is resolved page-by-page through the registry's chunk cache,
 * with no contiguity assumption. This variant is for buffers registered via
 * hipmem_mapping; heap-allocated buffers should use
 * nvme_request_prep_command_prps_contig_hip() instead.
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
nvme_request_prep_command_prps_contig_hip_mapped(struct nvme_request *request,
						  struct hipmem_mapping_registry *registry,
						  struct hipmem_config *config, void *dbuf,
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
	err = hipmem_mapping_virt_to_phys(registry, dbuf, &cmd->prp1);
	if (err) {
		return err;
	}

	if (npages == 1) {
		return 0;
	}
	if (npages == 2) {
		return hipmem_mapping_virt_to_phys(registry, page_base + pagesize, &cmd->prp2);
	}

	uint64_t *prp_list = (uint64_t *)request->prp;
	cmd->prp2 = request->prp_addr;
	for (uint64_t i = 1; i < npages; ++i) {
		err = hipmem_mapping_virt_to_phys(registry, page_base + (i << pagesize_shift),
						   &prp_list[i - 1]);
		if (err) {
			return err;
		}
	}
	return 0;
}

/**
 * Prepare PRPs for a HIP iovec (scatter-gather) data buffer registered with
 * a mapping registry.
 *
 * Each entry is resolved page-by-page through the registry's chunk cache.
 * This variant is for iovecs registered via hipmem_mapping; heap-allocated
 * iovecs should use nvme_request_prep_command_prps_iov_hip() instead.
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
nvme_request_prep_command_prps_iov_hip_mapped(struct nvme_request *request,
					       struct hipmem_mapping_registry *registry,
					       struct hipmem_config *config, struct iovec *dvec,
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
			err = hipmem_mapping_virt_to_phys(registry, base, &cmd->prp1);
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
			err = hipmem_mapping_virt_to_phys(registry, base + offset,
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
