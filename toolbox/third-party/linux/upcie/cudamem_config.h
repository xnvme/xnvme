// SPDX-License-Identifier: BSD-3-Clause

/**
 * Configuration for CUDA device memory properties
 * =================================================
 *
 * Analogous to hostmem_config, which tracks both host page size and hugepage
 * size, cudamem_config tracks both the host page size (used for PRP
 * construction and NVMe alignment) and the GPU device page size (the dma-buf
 * page granularity for memory allocated via cuMemAlloc).
 */

/**
 * Memory properties for CUDA device memory
 */
struct cudamem_config {
	int pagesize;       ///< Host page size (e.g. 4 KB); used for NVMe PRP and allocation alignment
	int pagesize_shift; ///< pagesize expressed as a power of two: pagesize == 1 << pagesize_shift
	int device_pagesize;  ///< GPU device page size (dma-buf page granularity for cuMemAlloc memory)
};

static inline int
cudamem_config_pp(struct cudamem_config *config)
{
	int wrtn = 0;

	wrtn += printf("cudamem_config:");

	if (!config) {
		wrtn += printf(" ~\n");
		return wrtn;
	}

	wrtn += printf("\n");
	wrtn += printf("  pagesize:      %d\n", config->pagesize);
	wrtn += printf("  pagesize_shift:%d\n", config->pagesize_shift);
	wrtn += printf("  device_pagesize: %d\n", config->device_pagesize);

	return wrtn;
}

/**
 * Initialize cudamem_config by querying the host page size and setting the GPU
 * device page size.
 *
 * The device page size is fixed at 64 KB: this is the dma-buf page granularity
 * for memory allocated via cuMemAlloc on all NVIDIA GPUs. It is distinct from
 * the minimum granularity returned by cuMemGetAllocationGranularity, which
 * applies only to the virtual memory management API (cuMemCreate/cuMemMap).
 *
 * @param config  Pointer to the config struct to initialize
 * @param gpu_id  CUDA device ordinal (unused, reserved for future use)
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
cudamem_config_init(struct cudamem_config *config, int gpu_id)
{
	(void)gpu_id;

	if (!config) {
		return -EINVAL;
	}

	config->pagesize = getpagesize();
	config->pagesize_shift = upcie_util_shift_from_size(config->pagesize);
	config->device_pagesize = 65536;

	return 0;
}
