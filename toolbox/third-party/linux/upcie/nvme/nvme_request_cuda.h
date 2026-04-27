// SPDX-License-Identifier: BSD-3-Clause


/**
 * CUDA NVMe Request Extension
 * ===========================
 *
 * This header extends the functionality defined in the uPCIe NVMe Request header
 * `upcie/nvme/nvme_request.h` with a CUDA dependent PRP preparation function.
 */

/**
 * Prepare the PRP list for a command with a contiguous CUDA data buffer.
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
 * @param heap Pointer to the CUDA memory heap that dbuf is allocated within.
 * @param dbuf Pointer to the contiguous data buffer to be described by PRPs.
 * @param dbuf_nbytes Size in bytes of the data buffer.
 * @param cmd Pointer to the NVMe command to be prepared with PRP entries.
 */
static inline void
nvme_request_prep_command_prps_contig_cuda_heap(struct nvme_request *request,
						struct cudamem_heap *heap, void *dbuf,
						size_t dbuf_nbytes, struct nvme_command *cmd)
{
	const uint64_t npages = (dbuf_nbytes + heap->config->pagesize - 1) >> heap->config->pagesize_shift;
	const uint64_t pagesize = heap->config->pagesize;

	/* Chaining is not supported, thus assert that the given dbuf fits. */
	assert(npages <= 1 + 512);

	cmd->prp1 = cudamem_heap_block_vtp(heap, dbuf);

	if (npages == 1) {
		return;
	} else if (npages == 2) {
		cmd->prp2 = cmd->prp1 + pagesize;
	} else {
		uint64_t *prp_list = (uint64_t *)request->prp;

		cmd->prp2 = request->prp_addr;
		for (uint64_t i = 1; i < npages; ++i) {
			prp_list[i - 1] = cmd->prp1 + (i << heap->config->pagesize_shift);
		}
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
nvme_request_prep_command_prps_iov_cuda_heap(struct nvme_request *request,
					     struct cudamem_heap *heap, struct iovec *dvec,
					     size_t dvec_cnt, struct nvme_command *cmd)
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
 * Prepare PRPs for a contiguous CUDA data buffer that lives in an externally
 * mapped region.
 *
 * The buffer is resolved page-by-page through the owning mapping's `phys_lut`,
 * with no contiguity assumption. This variant is for buffers registered via
 * cudamem_mapping; heap-allocated buffers should use
 * nvme_request_prep_command_prps_contig_cuda_heap() instead. Most call sites
 * should use nvme_request_prep_command_prps_contig_cuda(), which dispatches.
 *
 * Caveats
 * -------
 *
 * - Does *not* support PRP list chaining; only a single list page is constructed.
 * - The entire buffer must reside within a single mapping.
 *
 * @param request Pointer to the NVMe request context used for tracking and metadata.
 * @param mappings Head of the list of registered mappings to resolve through.
 * @param dbuf Pointer to the contiguous data buffer to be described by PRPs.
 * @param dbuf_nbytes Size in bytes of the data buffer.
 * @param cmd Pointer to the NVMe command to be prepared with PRP entries.
 * @return 0 on success, -EINVAL if the buffer is not contained in any mapping.
 */
static inline int
nvme_request_prep_command_prps_contig_cuda_mapped(struct nvme_request *request,
						  struct cudamem_mapping *mappings, void *dbuf,
						  size_t dbuf_nbytes, struct nvme_command *cmd)
{
	const uint64_t vaddr = (uint64_t)dbuf;

	for (struct cudamem_mapping *m = mappings; m; m = m->next) {
		const uint64_t pagesize       = m->config->pagesize;
		const uint64_t pagesize_shift = m->config->pagesize_shift;
		const int device_pagesize     = m->config->device_pagesize;
		const size_t prp_cap          = pagesize / sizeof(uint64_t);
		const uint64_t npages         = (dbuf_nbytes + pagesize - 1) >> pagesize_shift;
		size_t base_offset;

		if (vaddr < m->vaddr || vaddr >= m->vaddr + m->size) {
			continue;
		}

		base_offset = vaddr - m->vaddr;
		if (base_offset + dbuf_nbytes > m->size) {
			return -EINVAL;
		}
		if (npages > 1 + prp_cap) {
			return -EINVAL;
		}

		cmd->prp1 = m->phys_lut[base_offset / device_pagesize] +
			    (base_offset % device_pagesize);

		if (npages == 1) {
			return 0;
		}
		if (npages == 2) {
			size_t off = base_offset + pagesize;
			cmd->prp2  = m->phys_lut[off / device_pagesize] + (off % device_pagesize);
			return 0;
		}

		uint64_t *prp_list = (uint64_t *)request->prp;
		cmd->prp2          = request->prp_addr;
		for (uint64_t i = 1; i < npages; ++i) {
			size_t off = base_offset + (i << pagesize_shift);
			prp_list[i - 1] =
				m->phys_lut[off / device_pagesize] + (off % device_pagesize);
		}
		return 0;
	}

	return -EINVAL;
}

/**
 * Prepare PRPs for a CUDA iovec (scatter-gather) data buffer that lives in a
 * single externally mapped region.
 *
 * Every entry must lie within the same mapping (selected from the first
 * entry); each entry is bounds-checked and resolved page-by-page through the
 * mapping's `phys_lut`. This variant is for iovecs registered via
 * cudamem_mapping; heap-allocated iovecs should use
 * nvme_request_prep_command_prps_iov_cuda_heap() instead. Most call sites
 * should use nvme_request_prep_command_prps_iov_cuda(), which dispatches.
 *
 * @param request Pointer to the NVMe request context used for tracking and metadata.
 * @param mappings Head of the list of registered mappings to resolve through.
 * @param dvec Array of iovec structures describing the data segments.
 * @param dvec_cnt Number of elements in the dvec array.
 * @param cmd Pointer to the NVMe command to be prepared with PRP entries.
 * @return 0 on success, -EINVAL if any iovec cannot be resolved.
 */
static inline int
nvme_request_prep_command_prps_iov_cuda_mapped(struct nvme_request *request,
					       struct cudamem_mapping *mappings,
					       struct iovec *dvec, size_t dvec_cnt,
					       struct nvme_command *cmd)
{
	if (dvec_cnt == 0) {
		return -EINVAL;
	}

	const uint64_t first_vaddr = (uint64_t)dvec[0].iov_base;
	struct cudamem_mapping *m  = NULL;

	for (struct cudamem_mapping *cur = mappings; cur; cur = cur->next) {
		if (first_vaddr >= cur->vaddr && first_vaddr < cur->vaddr + cur->size) {
			m = cur;
			break;
		}
	}
	if (!m) {
		return -EINVAL;
	}

	const uint64_t pagesize   = m->config->pagesize;
	const int device_pagesize = m->config->device_pagesize;
	const size_t prp_cap      = pagesize / sizeof(uint64_t);
	uint64_t *prp_list        = (uint64_t *)request->prp;
	size_t prp_idx            = 0;

	for (size_t i = 0; i < dvec_cnt; ++i) {
		const uint64_t base  = (uint64_t)dvec[i].iov_base;
		const size_t iov_len = dvec[i].iov_len;
		size_t offset        = 0;
		size_t remaining     = iov_len;

		if (base < m->vaddr || base + iov_len > m->vaddr + m->size) {
			return -EINVAL;
		}

		if (i == 0) {
			size_t off = base - m->vaddr;
			cmd->prp1  = m->phys_lut[off / device_pagesize] + (off % device_pagesize);
			offset     = pagesize;
			remaining  = (remaining > pagesize) ? remaining - pagesize : 0;
		}

		while (remaining > 0) {
			if (prp_idx >= prp_cap) {
				return -EINVAL;
			}
			size_t off = (base + offset) - m->vaddr;
			prp_list[prp_idx++] =
				m->phys_lut[off / device_pagesize] + (off % device_pagesize);
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

/**
 * Prepare PRPs for a contiguous CUDA data buffer, dispatching by location.
 *
 * If `dbuf` lies inside `heap`, this defers to
 * nvme_request_prep_command_prps_contig_cuda_heap(); otherwise it walks
 * `mappings` via nvme_request_prep_command_prps_contig_cuda_mapped().
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
nvme_request_prep_command_prps_contig_cuda(struct nvme_request *request,
					   struct cudamem_heap *heap,
					   struct cudamem_mapping *mappings, void *dbuf,
					   size_t dbuf_nbytes, struct nvme_command *cmd)
{
	if (cudamem_heap_contains(heap, dbuf)) {
		nvme_request_prep_command_prps_contig_cuda_heap(request, heap, dbuf, dbuf_nbytes,
								cmd);
		return 0;
	}

	return nvme_request_prep_command_prps_contig_cuda_mapped(request, mappings, dbuf,
								 dbuf_nbytes, cmd);
}

/**
 * Prepare PRPs for a CUDA iovec data buffer, dispatching by location.
 *
 * If `dvec[0].iov_base` lies inside `heap`, this defers to
 * nvme_request_prep_command_prps_iov_cuda_heap(); otherwise it walks
 * `mappings` via nvme_request_prep_command_prps_iov_cuda_mapped(). The
 * dispatcher only inspects the first entry; the called variant enforces its
 * own placement rules across the rest.
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
nvme_request_prep_command_prps_iov_cuda(struct nvme_request *request, struct cudamem_heap *heap,
					struct cudamem_mapping *mappings, struct iovec *dvec,
					size_t dvec_cnt, struct nvme_command *cmd)
{
	if (dvec_cnt > 0 && cudamem_heap_contains(heap, dvec[0].iov_base)) {
		nvme_request_prep_command_prps_iov_cuda_heap(request, heap, dvec, dvec_cnt, cmd);
		return 0;
	}

	return nvme_request_prep_command_prps_iov_cuda_mapped(request, mappings, dvec, dvec_cnt,
							      cmd);
}
