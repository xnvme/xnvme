// SPDX-License-Identifier: BSD-3-Clause

/**
 * Configuration for CUDA device memory properties
 * =================================================
 *
 * Analogous to hostmem_config, which tracks both host page size and hugepage
 * size, cudamem_config tracks both the host page size (used for PRP
 * construction and NVMe alignment) and the GPU device page size (the dma-buf
 * page granularity for memory allocated via cuMemAlloc).
 *
 * It also stashes the total device memory and BAR1 size, both used by
 * downstream consumers: device_memsize sizes the cudamem_mapping_registry
 * LUT, and bar1_size is verified to be at least as large as device_memsize at
 * init time (a precondition for PCIe P2P DMA against arbitrary device pages).
 *
 * Caveat: Single-GPU usage
 * ------------------------
 *
 * One config describes one device. Process-wide multi-GPU usage requires one
 * cudamem_config (and one cudamem_mapping_registry, one cudamem_heap) per GPU.
 */

/**
 * Memory properties for CUDA device memory
 */
struct cudamem_config {
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
cudamem_config_pp(struct cudamem_config *config)
{
	int wrtn = 0;

	wrtn += printf("cudamem_config:");

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
 * Initialize cudamem_config by querying host page size, GPU device page size,
 * total device memory, and BAR1 size.
 *
 * The device page size is fixed at 64 KB: this is the dma-buf page granularity
 * for memory allocated via cuMemAlloc on all NVIDIA GPUs. It is distinct from
 * the minimum granularity returned by cuMemGetAllocationGranularity, which
 * applies only to the virtual memory management API (cuMemCreate/cuMemMap).
 *
 * `alloc_granularity` is queried via cuMemGetAllocationGranularity with
 * CU_MEM_ALLOC_GRANULARITY_MINIMUM. The CUDA documentation defines this for
 * the VMM API, but cudamem_mapping reuses the same value as the BAR1
 * large-page size for cuMemAlloc-backed memory: on NVIDIA GPUs the kernel
 * driver maps both VMM and cuMemAlloc allocations through the same BAR1
 * page-table large-page mechanism, so the VMM minimum reservation unit
 * matches the contiguous IOVA window each cuMemAlloc chunk occupies. The
 * contiguity check in cudamem_mapping_chunk_populate (returning -EOPNOTSUPP
 * when violated) catches a hardware/driver mismatch at runtime.
 *
 * BAR1 size is read via pci_bar_size(bdf, 1, ...), where <bdf> is obtained
 * from cuDeviceGetPCIBusId(). The PCI BAR index 1 follows NVIDIA's discrete
 * GPU convention (BAR0 is 32-bit MMIO, BAR1 is the framebuffer aperture); a
 * 64-bit BAR0 layout (consuming registers 0 and 1) would shift the FB
 * aperture to resource2 and is not supported. The function fails if BAR1
 * size is smaller than total device memory: PCIe P2P DMA over the full
 * device memory range requires the BAR1 to span it, which in turn requires
 * resizable BAR (or a similarly sized fixed BAR) to be enabled in firmware.
 *
 * @param config  Pointer to the config struct to initialize
 * @param gpu_id  CUDA device ordinal
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int
cudamem_config_init(struct cudamem_config *config, int gpu_id)
{
	char bdf[PCI_BDF_LEN + 1] = {0};
	CUmemAllocationProp prop = {0};
	CUdevice dev;
	size_t total_mem = 0;
	size_t gran = 0;
	CUresult cr;
	int err;

	if (!config) {
		return -EINVAL;
	}

	config->pagesize = getpagesize();
	config->pagesize_shift = upcie_util_shift_from_size(config->pagesize);
	config->device_pagesize = 65536;

	cr = cuDeviceGet(&dev, gpu_id);
	if (cr != CUDA_SUCCESS) {
		UPCIE_DEBUG("FAILED: cuDeviceGet(gpu_id=%d), cr: %d", gpu_id, cr);
		return -ENODEV;
	}

	cr = cuDeviceTotalMem(&total_mem, dev);
	if (cr != CUDA_SUCCESS) {
		UPCIE_DEBUG("FAILED: cuDeviceTotalMem(), cr: %d", cr);
		return -EIO;
	}
	config->device_memsize = total_mem;

	cr = cuDeviceGetPCIBusId(bdf, sizeof(bdf), dev);
	if (cr != CUDA_SUCCESS) {
		UPCIE_DEBUG("FAILED: cuDeviceGetPCIBusId(), cr: %d", cr);
		return -EIO;
	}

	/* cuDeviceGetPCIBusId returns uppercase hex; sysfs uses lowercase. */
	for (int i = 0; bdf[i]; i++) {
		if (bdf[i] >= 'A' && bdf[i] <= 'F') {
			bdf[i] = bdf[i] - 'A' + 'a';
		}
	}

	err = pci_bar_size(bdf, 1, &config->bar1_size);
	if (err) {
		UPCIE_DEBUG("FAILED: pci_bar_size(%s, 1), err: %d", bdf, err);
		return err;
	}

	if (config->bar1_size < config->device_memsize) {
		UPCIE_DEBUG("FAILED: BAR1(%zu) < device_memsize(%zu); enable resizable BAR in "
			    "firmware",
			    config->bar1_size, config->device_memsize);
		return -EOPNOTSUPP;
	}

	prop.type = CU_MEM_ALLOCATION_TYPE_PINNED;
	prop.location.type = CU_MEM_LOCATION_TYPE_DEVICE;
	prop.location.id = (int)dev;

	cr = cuMemGetAllocationGranularity(&gran, &prop, CU_MEM_ALLOC_GRANULARITY_MINIMUM);
	if (cr != CUDA_SUCCESS) {
		UPCIE_DEBUG("FAILED: cuMemGetAllocationGranularity(), cr: %d", cr);
		return -EIO;
	}
	if (!gran || (gran & (gran - 1))) {
		UPCIE_DEBUG("FAILED: cuMemGetAllocationGranularity returned non-power-of-two: %zu",
			    gran);
		return -EINVAL;
	}
	config->alloc_granularity = gran;
	config->alloc_granularity_shift = upcie_util_shift_from_size(gran);

	return 0;
}
