// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * dmamem constructor: from an existing dma-buf fd
 * ===============================================
 *
 * Takes a dma-buf fd from any exporter (udmabuf host memory, CUDA VMM,
 * HIP, libdrm/PRIME, VFIO_DEVICE_FEATURE_DMA_BUF) and imports it into an
 * iommufd IOAS via IOMMU_IOAS_MAP_FILE. This is the entry point that
 * answers the "does iommufd's MAP_FILE accept dma-buf-backed fds today?"
 * question against a running kernel; a failure at the ioctl surfaces the
 * exact errno so the block (kernel-side, exporter-side, or IOAS
 * configuration) resolves to a concrete signal.
 *
 * The dmamem takes ownership of the dma-buf fd on success and closes it
 * as part of dmamem_destroy. On failure the caller retains ownership and
 * must close the fd itself.
 *
 * Whether the resulting dmamem exposes a CPU VA is up to the exporter.
 * udmabuf-backed dma-bufs are CPU-mappable; VRAM-backed dma-bufs from the
 * GPU runtimes usually are not. If mmap on the dma-buf fd fails we leave
 * cpu_va NULL and rely on offset-based access.
 *
 * @file dmamem_dmabuf.h
 * @version 0.7.0
 */

/**
 * Import an existing dma-buf fd into the handle's IOAS as a dmamem.
 *
 * @param dmem       Pre-allocated dmamem descriptor to fill.
 * @param iommufd    Open iommufd handle with an allocated IOAS (caller-owned).
 * @param dmabuf_fd  Open dma-buf fd from any exporter.
 * @param size       Size of the range to map.
 *
 * @return 0 on success (dmamem now owns dmabuf_fd), negative errno on
 * error (caller retains dmabuf_fd).
 */
static inline int
dmamem_from_dmabuf(struct dmamem *dmem, struct iommufd *iommufd, int dmabuf_fd, size_t size)
{
	void *cpu_va;
	int err;

	if (!dmem || !iommufd || iommufd->fd < 0 || dmabuf_fd < 0 || !size) {
		return -EINVAL;
	}

	memset(dmem, 0, sizeof(*dmem));
	dmem->fd = dmabuf_fd;
	dmem->iommufd = iommufd;
	dmem->size = size;
	dmem->backing = DMAMEM_BACKING_DMABUF;
	dmem->translator = DMAMEM_XLATE_ARITHMETIC;
	dmem->owned = 1;

	/*
	 * CPU-mappability is exporter-dependent. Try mmap; leave cpu_va
	 * NULL on failure. This is diagnostic, not fatal.
	 */
	cpu_va = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dmabuf_fd, 0);
	if (cpu_va != MAP_FAILED) {
		dmem->cpu_va = cpu_va;
		dmem->base_va = cpu_va;
	}

	err = iommufd_ioas_map_file(iommufd, dmabuf_fd, 0, size,
				    IOMMU_IOAS_MAP_READABLE | IOMMU_IOAS_MAP_WRITEABLE,
				    &dmem->base_iova);
	if (err) {
		UPCIE_DEBUG("FAILED: iommufd_ioas_map_file(dma-buf); err(%d)", err);
		if (dmem->cpu_va) {
			munmap(dmem->cpu_va, size);
			dmem->cpu_va = NULL;
		}
		memset(dmem, 0, sizeof(*dmem));
		dmem->fd = -1;
		return err;
	}

	return 0;
}
