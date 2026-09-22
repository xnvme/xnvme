// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#if (defined(XNVME_CUDA_ENABLED) || defined(XNVME_HIP_ENABLED)) && \
	defined(XNVME_BE_CBI_MEM_POSIX_ENABLED)
#define XNVME_BE_LINUX_DMABUF_MEM_ENABLED
#endif

#ifdef XNVME_BE_LINUX_DMABUF_MEM_ENABLED
#ifdef XNVME_CUDA_ENABLED
#include <cuda.h>
#include <cuda_runtime.h>
#include <xnvme_cuda_buf.h>
#endif
#ifdef XNVME_HIP_ENABLED
#include <hip/hip_runtime.h>
#include <xnvme_hip_buf.h>
#endif
#include <errno.h>
#include <unistd.h>
#include <libxnvme.h>
#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_dmabuf.h>
#include <xnvme_be_linux_dmabuf_iommu.h>

/**
 * Largest dma-buf io_uring accepts for buffer-registration
 *
 * @see io_register_dmabuf() in 'io_uring/rsrc.c'
 */
#define XNVME_BE_LINUX_DMABUF_NBYTES_MAX (1024ULL * 1024 * 1024)

/**
 * Round an allocation length up to what a dma-buf export will take
 *
 * A runtime-reported size need not be page-aligned; rounding up stays inside
 * the page-backed allocation.
 *
 * @return On success, the length to export. On error, 0
 */
static size_t
_export_nbytes(size_t alloc_nbytes)
{
	long pagesize;
	size_t nbytes;

	pagesize = sysconf(_SC_PAGESIZE);
	if (pagesize == -1) {
		XNVME_DEBUG("FAILED: sysconf(), errno: %d", errno);
		return 0;
	}

	nbytes = (alloc_nbytes + (size_t)pagesize - 1) & ~((size_t)pagesize - 1);
	if (nbytes > XNVME_BE_LINUX_DMABUF_NBYTES_MAX) {
		XNVME_DEBUG("FAILED: allocation of %zu exceeds the 1GiB dma-buf limit", nbytes);
		return 0;
	}

	return nbytes;
}

/**
 * Export the device allocation holding 'vaddr' as a dma-buf
 *
 * An export describes an allocation rather than a range, so the enclosing one
 * is recovered first and is what gets exported. Offsets are unaffected, being
 * taken from the base returned here.
 *
 * @return On success, 0 is returned. On error, negative errno
 */
static int
_dmabuf_export(void *vaddr, void **base, size_t *nbytes, int *fd)
{
	size_t export_nbytes;

#ifdef XNVME_CUDA_ENABLED
	if (xnvme_buf_is_cuda(vaddr)) {
		CUdeviceptr alloc_base;
		size_t alloc_nbytes;

		// The mapping before the allocation, as dmamem_cuda_registry_range()
		// in <upcie/dmamem_cuda.h> does. An allocation the runtime reports is
		// the caller's suballocation, whose base is arbitrary, while an export
		// describes the block the driver placed it in
		if ((cuPointerGetAttribute(&alloc_base, CU_POINTER_ATTRIBUTE_MAPPING_BASE_ADDR,
					   (CUdeviceptr)vaddr) != CUDA_SUCCESS) ||
		    (cuPointerGetAttribute(&alloc_nbytes, CU_POINTER_ATTRIBUTE_MAPPING_SIZE,
					   (CUdeviceptr)vaddr) != CUDA_SUCCESS)) {
			if (cuMemGetAddressRange(&alloc_base, &alloc_nbytes, (CUdeviceptr)vaddr) !=
			    CUDA_SUCCESS) {
				XNVME_DEBUG("FAILED: cuMemGetAddressRange(%p)", vaddr);
				return -EINVAL;
			}
		}

		export_nbytes = _export_nbytes(alloc_nbytes);
		if (!export_nbytes) {
			return -EINVAL;
		}

		if (cuMemGetHandleForAddressRange(fd, alloc_base, export_nbytes,
						  CU_MEM_RANGE_HANDLE_TYPE_DMA_BUF_FD,
						  0) != CUDA_SUCCESS) {
			XNVME_DEBUG("FAILED: cuMemGetHandleForAddressRange(0x%llx, %zu)",
				    (unsigned long long)alloc_base, export_nbytes);
			return -EIO;
		}

		*base = (void *)alloc_base;
		*nbytes = alloc_nbytes;

		return 0;
	}
#endif
#ifdef XNVME_HIP_ENABLED
	if (xnvme_buf_is_hip(vaddr)) {
		if (hipMemGetAddressRange((hipDeviceptr_t *)base, nbytes, (hipDeviceptr_t)vaddr) !=
		    hipSuccess) {
			XNVME_DEBUG("FAILED: hipMemGetAddressRange(%p)", vaddr);
			return -EINVAL;
		}

		export_nbytes = _export_nbytes(*nbytes);
		if (!export_nbytes) {
			return -EINVAL;
		}

		if (hipMemGetHandleForAddressRange(fd, (hipDeviceptr_t)*base, export_nbytes,
						   hipMemRangeHandleTypeDmaBufFd,
						   0) != hipSuccess) {
			XNVME_DEBUG("FAILED: hipMemGetHandleForAddressRange(%p, %zu)", *base,
				    export_nbytes);
			return -EIO;
		}

		return 0;
	}
#endif

	XNVME_DEBUG("FAILED: buf(%p) is neither CUDA nor HIP device memory", vaddr);

	return -EINVAL;
}

/**
 * Export the allocation holding 'vaddr' and add it to the device registry
 */
static int
_dmabuf_register(const struct xnvme_dev *dev, void *vaddr, size_t nbytes)
{
	uint64_t map_handle = 0;
	uint64_t offset;
	uint16_t index;
	void *base;
	size_t alloc_nbytes;
	int fd = -1;
	int err;

	err = xnvme_be_linux_dmabuf_lookup(dev, vaddr, nbytes, &index, &offset);
	if (!err) {
		return xnvme_be_linux_dmabuf_ref(dev, index);
	}
	if (err != -ENOENT) {
		return err;
	}

	err = _dmabuf_export(vaddr, &base, &alloc_nbytes, &fd);
	if (err) {
		return err;
	}

	err = xnvme_be_linux_dmabuf_iommu_translates(dev);
	if (err < 0) {
		XNVME_DEBUG("FAILED: xnvme_be_linux_dmabuf_iommu_translates(), err: %d", err);
		close(fd);
		return err;
	}
	if (err) {
		err = xnvme_be_linux_dmabuf_iommu_map(dev, fd, &map_handle);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_be_linux_dmabuf_iommu_map(), err: %d", err);
			close(fd);
			return err;
		}
	}

	err = xnvme_be_linux_dmabuf_add(dev, base, alloc_nbytes, fd, map_handle);
	if (err < 0) {
		XNVME_DEBUG("FAILED: xnvme_be_linux_dmabuf_add(), err: %d", err);
		if (map_handle) {
			xnvme_be_linux_dmabuf_iommu_unmap(map_handle);
		}
		close(fd);
		return err;
	}

	XNVME_DEBUG("INFO: registered dma-buf(%p, %zu) at index: %d", base, alloc_nbytes, err);

	return 0;
}

/**
 * Register a caller-allocated device buffer for DMA
 */
static int
mem_map(const struct xnvme_dev *dev, void *vaddr, size_t nbytes, uint64_t *XNVME_UNUSED(phys))
{
	return _dmabuf_register(dev, vaddr, nbytes);
}

static int
mem_unmap(const struct xnvme_dev *dev, void *vaddr)
{
	uint64_t offset;
	uint16_t index;
	int err;

	err = xnvme_be_linux_dmabuf_lookup(dev, vaddr, 1, &index, &offset);
	if (err) {
		return err;
	}

	return xnvme_be_linux_dmabuf_del(dev, index);
}
#endif

struct xnvme_be_mem g_xnvme_be_linux_mem_dmabuf = {
	.id = "linux-dmabuf",
#ifdef XNVME_BE_LINUX_DMABUF_MEM_ENABLED
	.buf_alloc = xnvme_be_cbi_mem_posix_buf_alloc,
	.buf_realloc = xnvme_be_cbi_mem_posix_buf_realloc,
	.buf_free = xnvme_be_cbi_mem_posix_buf_free,
	.buf_vtophys = xnvme_be_cbi_mem_posix_buf_vtophys,
	.mem_map = mem_map,
	.mem_unmap = mem_unmap,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#endif
};
