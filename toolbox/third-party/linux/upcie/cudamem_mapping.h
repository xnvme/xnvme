// SPDX-License-Identifier: BSD-3-Clause

/**
 * Mapping of an externally-allocated CUDA buffer for NVMe DMA
 * ===========================================================
 *
 * Where cudamem_heap pre-allocates a single dma-buf-backed range and serves
 * sub-allocations from it, cudamem_mapping registers a buffer that the caller
 * already allocated (via cuMemAlloc, cuMemCreate/cuMemMap, or cudaMalloc) and
 * builds a physical address LUT for it via the same dma-buf interface.
 *
 * Mappings are independent and can be linked into a list; the resolver
 * cudamem_mapping_virt_to_phys() walks the list to dispatch a virtual address
 * to the right entry.
 *
 * Caveat: Hardware requirements
 * -----------------------------
 * Same as cudamem_heap: requires a GPU with PCIe P2P DMA support and a large
 * BAR1 region.
 */

/**
 * An externally-allocated CUDA buffer registered for NVMe DMA
 *
 * The caller owns the GPU virtual range; this struct holds the dma-buf
 * attachment and the per-device-page physical address LUT.
 *
 * `phys_lut` is a flexible array member; the struct must be allocated with
 * trailing space for `nphys` entries. cudamem_mapping_add() handles this.
 */
struct cudamem_mapping {
	uint64_t vaddr;				///< Virtual address of the registered range
	size_t size;				///< Size of the registered range in bytes
	struct dmabuf dmabuf;			///< dma-buf attachment for the range
	size_t nphys;				///< Number of device pages in the range (size / config->device_pagesize)
	struct cudamem_config *config;		///< Device memory configuration (page sizes)
	struct cudamem_mapping *next;		///< Optional list linkage; populated by the caller
	uint64_t phys_lut[];			///< One physical address per device page
};

/**
 * Populate a caller-allocated mapping for an externally-allocated CUDA range
 *
 * Obtains a dma-buf fd for [vaddr, vaddr + nbytes) via
 * cuMemGetHandleForAddressRange(), attaches it, and fills the LUT at
 * device_pagesize granularity.
 *
 * `mapping` must have trailing space for `nbytes / config->device_pagesize`
 * entries in `phys_lut`. Most callers should use cudamem_mapping_add(), which
 * sizes the allocation correctly.
 *
 * `vaddr` and `nbytes` must be aligned to `config->device_pagesize`.
 *
 * NOTE: Set up CUDA Driver (cuInit()) and CUDA Context (cuCtxCreate())
 * before calling this function.
 *
 * NOTE: `config` must remain valid for the lifetime of `mapping`.
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
cudamem_mapping_init(struct cudamem_mapping *mapping, void *vaddr, size_t nbytes,
		     struct cudamem_config *config)
{
	int dmabuf_fd = -1, err;
	CUresult cr;

	if (!mapping || !vaddr || !nbytes || !config) {
		return -EINVAL;
	}
	if ((uint64_t)vaddr % config->device_pagesize ||
	    nbytes % config->device_pagesize) {
		UPCIE_DEBUG("FAILED: vaddr/nbytes not aligned to device_pagesize(%d)",
			    config->device_pagesize);
		return -EINVAL;
	}

	mapping->vaddr = (uint64_t)vaddr;
	mapping->size = nbytes;
	mapping->nphys = nbytes / config->device_pagesize;
	mapping->config = config;
	mapping->next = NULL;

	cr = cuMemGetHandleForAddressRange(&dmabuf_fd, (CUdeviceptr)vaddr, nbytes,
					   CU_MEM_RANGE_HANDLE_TYPE_DMA_BUF_FD, 0);
	if (cr != CUDA_SUCCESS) {
		UPCIE_DEBUG("FAILED: cuMemGetHandleForAddressRange(), cr: %d", cr);
		return -EIO;
	}

	err = dmabuf_attach(dmabuf_fd, &mapping->dmabuf);
	if (err) {
		UPCIE_DEBUG("FAILED: dmabuf_attach(), err: %d", err);
		close(dmabuf_fd);
		return err;
	}

	err = dmabuf_get_lut(&mapping->dmabuf, mapping->nphys, mapping->phys_lut,
			    config->device_pagesize);
	if (err) {
		UPCIE_DEBUG("FAILED: dmabuf_get_lut(), err: %d", err);
		dmabuf_detach(&mapping->dmabuf);
		return err;
	}

	return 0;
}

/**
 * Tear down a mapping previously initialized with cudamem_mapping_init()
 *
 * Detaches the dma-buf. The struct itself is not freed (the LUT is part of
 * the same allocation); use cudamem_mapping_remove() for the heap-managed
 * counterpart. The underlying CUDA allocation is not freed either; that
 * remains the caller's responsibility.
 */
static inline void
cudamem_mapping_term(struct cudamem_mapping *mapping)
{
	if (!mapping) {
		return;
	}

	dmabuf_detach(&mapping->dmabuf);
}

/**
 * Allocate a mapping, initialize it, and prepend it to the given list
 *
 * Allocates a single block sized for the struct plus its LUT. Ownership is
 * transferred to the list. If `out` is non-NULL, it receives the mapping
 * pointer (e.g. to read the resulting `phys_lut[0]`).
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
cudamem_mapping_add(struct cudamem_mapping **head, void *vaddr, size_t nbytes,
		    struct cudamem_config *config, struct cudamem_mapping **out)
{
	struct cudamem_mapping *m;
	size_t nphys;
	int err;

	if (!head || !vaddr || !nbytes || !config) {
		return -EINVAL;
	}
	if ((uint64_t)vaddr % config->device_pagesize ||
	    nbytes % config->device_pagesize) {
		UPCIE_DEBUG("FAILED: vaddr/nbytes not aligned to device_pagesize(%d)",
			    config->device_pagesize);
		return -EINVAL;
	}

	nphys = nbytes / config->device_pagesize;
	m = malloc(sizeof(*m) + nphys * sizeof(uint64_t));
	if (!m) {
		return -ENOMEM;
	}

	err = cudamem_mapping_init(m, vaddr, nbytes, config);
	if (err) {
		free(m);
		return err;
	}

	m->next = *head;
	*head = m;

	if (out) {
		*out = m;
	}

	return 0;
}

/**
 * Remove the mapping with the given vaddr from the list, term it, and free it
 *
 * Matches by exact start address, mirroring cudamem_mapping_init()'s
 * registration contract.
 *
 * @return 0 on success, -EINVAL if no mapping with that vaddr is in the list.
 */
static inline int
cudamem_mapping_remove(struct cudamem_mapping **head, void *vaddr)
{
	uint64_t key = (uint64_t)vaddr;

	if (!head) {
		return -EINVAL;
	}

	for (struct cudamem_mapping **prev = head, *m = *head; m; prev = &m->next, m = m->next) {
		if (m->vaddr == key) {
			*prev = m->next;
			cudamem_mapping_term(m);
			free(m);
			return 0;
		}
	}

	return -EINVAL;
}

/**
 * Remove and free every mapping in the list, leaving `*head` NULL
 */
static inline void
cudamem_mapping_clear(struct cudamem_mapping **head)
{
	struct cudamem_mapping *next;

	if (!head) {
		return;
	}

	for (struct cudamem_mapping *m = *head; m; m = next) {
		next = m->next;
		cudamem_mapping_term(m);
		free(m);
	}
	*head = NULL;
}

/**
 * Resolve a CUDA virtual address that lies in one of the registered mappings
 *
 * Walks `mappings` and returns the physical address corresponding to `virt`.
 *
 * @return 0 on success, -EINVAL if `virt` is not contained in any mapping.
 */
static inline int
cudamem_mapping_virt_to_phys(struct cudamem_mapping *mappings, void *virt, uint64_t *phys)
{
	uint64_t vaddr = (uint64_t)virt;

	if (!virt || !phys) {
		return -EINVAL;
	}

	for (struct cudamem_mapping *m = mappings; m; m = m->next) {
		if (vaddr >= m->vaddr && vaddr < m->vaddr + m->size) {
			size_t offset = vaddr - m->vaddr;
			*phys = m->phys_lut[offset / m->config->device_pagesize] +
				(offset % m->config->device_pagesize);
			return 0;
		}
	}

	return -EINVAL;
}
