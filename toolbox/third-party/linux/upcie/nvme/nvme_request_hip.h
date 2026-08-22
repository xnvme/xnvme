// SPDX-License-Identifier: BSD-3-Clause


/**
 * HIP NVMe Request Extension
 * ===========================
 *
 * This header extends the functionality defined in the uPCIe NVMe Request header
 * `upcie/nvme/nvme_request.h` with a HIP dependent PRP preparation function.
 *
 * @file nvme_request_hip.h
 * @version 0.6.0
 */

/**
 * Re-translate a PRP entry that crosses a device-page boundary.
 *
 * Kept out-of-line so the rare boundary path does not inline a vtp into the hot
 * contig builder; the common in-device-page case strides physically from PRP1,
 * and the inlined stride loop keeps large-I/O builds free of any per-entry call.
 */
static inline uint64_t __attribute__((noinline))
nvme_request_prp_retranslate_hip(struct hipmem_heap *heap, void *virt)
{
	return hipmem_heap_block_vtp(heap, virt);
}

/**
 * Prepare PRPs for a contiguous HIP data buffer registered with a mapping
 * registry.
 *
 * The buffer is resolved page-by-page through the registry's chunk cache,
 * with no contiguity assumption. This variant is for buffers registered via
 * hipmem_mapping; heap-allocated buffers should use
 * nvme_request_prep_command_prps_contig_dmamem() instead.
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
 * iovecs should use nvme_request_prep_command_prps_iov_dmamem() instead.
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
