// SPDX-License-Identifier: BSD-3-Clause

/**
 * Configuration for CUDA device memory properties
 * =================================================
 *
 * Analogous to hostmem_config, which tracks both host page size and hugepage
 * size, hipmem_config tracks both the host page size (used for PRP
 * construction and NVMe alignment) and the GPU device page size (the dma-buf
 * page granularity for memory allocated via cuMemAlloc).
 *
 * It also stashes the total device memory and BAR1 size, both used by
 * downstream consumers: device_memsize sizes the hipmem_mapping_registry
 * LUT, and bar1_size is verified to be at least as large as device_memsize at
 * init time (a precondition for PCIe P2P DMA against arbitrary device pages).
 *
 * Caveat: Single-GPU usage
 * ------------------------
 *
 * One config describes one device. Process-wide multi-GPU usage requires one
 * hipmem_config (and one hipmem_mapping_registry, one hipmem_heap) per GPU.
 */

/**
 * Memory properties for CUDA device memory
 */
struct hipmem_config {
	int pagesize; ///< Host page size (e.g. 4 KB); used for NVMe PRP and allocation alignment
	int pagesize_shift;    ///< pagesize expressed as a power of two: pagesize == 1 <<
			       ///< pagesize_shift
	int device_pagesize;   ///< GPU device page size (dma-buf page granularity for cuMemAlloc
			       ///< memory)
	size_t device_memsize; ///< Total GPU device memory in bytes (cuDeviceTotalMem)
	size_t bar1_size;      ///< Size of the GPU's BAR1 region in bytes
	size_t alloc_granularity;    ///< cuMemAlloc VA reservation unit and BAR1 large-page
				     ///< size; queried via cuMemGetAllocationGranularity
	int alloc_granularity_shift; ///< alloc_granularity expressed as a power of two:
				     ///< alloc_granularity == 1 << alloc_granularity_shift
};

static inline int
hipmem_config_pp(struct hipmem_config *config)
{
	int wrtn = 0;

	wrtn += printf("hipmem_config:");

	if (!config) {
		wrtn += printf(" ~\n");
		return wrtn;
	}

	wrtn += printf("\n");
	wrtn += printf("  pagesize:                %d\n", config->pagesize);
	wrtn += printf("  pagesize_shift:          %d\n", config->pagesize_shift);
	wrtn += printf("  device_pagesize:         %d\n", config->device_pagesize);
	wrtn += printf("  device_memsize:          %zu\n", config->device_memsize);
	wrtn += printf("  bar1_size:               %zu\n", config->bar1_size);
	wrtn += printf("  alloc_granularity:       %zu\n", config->alloc_granularity);
	wrtn += printf("  alloc_granularity_shift: %d\n", config->alloc_granularity_shift);

	return wrtn;
}

/**
 * Initialize hipmem_config by querying host page size, GPU device page size,
 * total device memory, and BAR1 size.
 *
 * The device page size is fixed at 64 KB: this is the dma-buf page granularity
 * for memory allocated via cuMemAlloc on all NVIDIA GPUs. It is distinct from
 * the minimum granularity returned by cuMemGetAllocationGranularity, which
 * applies only to the virtual memory management API (cuMemCreate/cuMemMap).
 *
 * `alloc_granularity` is queried via cuMemGetAllocationGranularity with
 * CU_MEM_ALLOC_GRANULARITY_MINIMUM. The CUDA documentation defines this for
 * the VMM API, but hipmem_mapping reuses the same value as the BAR1
 * large-page size for cuMemAlloc-backed memory: on NVIDIA GPUs the kernel
 * driver maps both VMM and cuMemAlloc allocations through the same BAR1
 * page-table large-page mechanism, so the VMM minimum reservation unit
 * matches the contiguous IOVA window each cuMemAlloc chunk occupies. The
 * contiguity check in hipmem_mapping_chunk_populate (returning -EOPNOTSUPP
 * when violated) catches a hardware/driver mismatch at runtime.
 *
 * The framebuffer aperture size is read via pci_bar_largest_size(bdf, ...),
 * where <bdf> is obtained from cuDeviceGetPCIBusId(); the largest BAR is the
 * framebuffer aperture regardless of whether BAR0 is 32-bit or 64-bit. PCIe
 * P2P DMA over the full device memory range requires the aperture to span it,
 * which in turn requires resizable BAR (or a similarly sized fixed BAR) to be
 * enabled in firmware.
 *
 * @param config  Pointer to the config struct to initialize
 * @param gpu_id  CUDA device ordinal
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
hipmem_config_init(struct hipmem_config *config, int gpu_id)
{
	char bdf[PCI_BDF_LEN + 1] = {0};
	hipDevice_t dev;
	size_t total_mem = 0;
	hipError_t cr;
	int err;

	if (!config) {
		return -EINVAL;
	}

	config->pagesize = getpagesize();
	config->pagesize_shift = upcie_util_shift_from_size(config->pagesize);
	config->device_pagesize = 4096; // AMD GPUs expose 4 KiB device pages

	cr = hipDeviceGet(&dev, gpu_id);
	if (cr != hipSuccess) {
		UPCIE_DEBUG("FAILED: hipDeviceGet(gpu_id=%d), cr: %d", gpu_id, cr);
		return -ENODEV;
	}

	cr = hipDeviceTotalMem(&total_mem, dev);
	if (cr != hipSuccess) {
		UPCIE_DEBUG("FAILED: hipDeviceTotalMem(), cr: %d", cr);
		return -EIO;
	}
	config->device_memsize = total_mem;

	cr = hipDeviceGetPCIBusId(bdf, sizeof(bdf), dev);
	if (cr != hipSuccess) {
		UPCIE_DEBUG("FAILED: hipDeviceGetPCIBusId(), cr: %d", cr);
		return -EIO;
	}

	/* hipDeviceGetPCIBusId returns uppercase hex; sysfs uses lowercase. */
	for (int i = 0; bdf[i]; i++) {
		if (bdf[i] >= 'A' && bdf[i] <= 'F') {
			bdf[i] = bdf[i] - 'A' + 'a';
		}
	}

	err = pci_bar_largest_size(bdf, &config->bar1_size);
	if (err) {
		UPCIE_DEBUG("FAILED: pci_bar_largest_size(%s), err: %d", bdf, err);
		return err;
	}

	if (config->bar1_size < config->device_memsize) {
		UPCIE_DEBUG("FAILED: BAR1(%zu) < device_memsize(%zu); enable resizable BAR in "
			    "firmware",
			    config->bar1_size, config->device_memsize);
		return -EOPNOTSUPP;
	}

	// AMD exposes VRAM as 4 KiB P2P pages behind the resizable BAR, with no
	// 64 KiB BAR1 large-page coalescing as on NVIDIA, so the allocation and
	// mapping granularity is simply the device page size.
	config->alloc_granularity = config->device_pagesize;
	config->alloc_granularity_shift = upcie_util_shift_from_size(config->device_pagesize);

	return 0;
}
