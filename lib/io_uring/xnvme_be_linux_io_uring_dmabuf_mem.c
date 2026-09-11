// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>

#ifdef XNVME_BE_LINUX_IO_URING_DMABUF_ENABLED
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
#include <xnvme_be_cbi.h>
#include <xnvme_be_linux_io_uring_dmabuf.h>

static size_t
_export_nbytes(size_t alloc_nbytes)
{
	long pagesize;

	pagesize = sysconf(_SC_PAGESIZE);
	if (pagesize == -1) {
		XNVME_DEBUG("FAILED: sysconf(), errno: %d", errno);
		return 0;
	}

	// A dma-buf export is a whole number of pages
	return (alloc_nbytes + (size_t)pagesize - 1) & ~((size_t)pagesize - 1);
}

static int
_dmabuf_export(void *vaddr, void **base, size_t *nbytes, int *fd)
{
	size_t export_nbytes;

#ifdef XNVME_CUDA_ENABLED
	if (xnvme_buf_is_cuda(vaddr)) {
		CUdeviceptr alloc_base;
		size_t alloc_nbytes;

		// Prefer the mapping over the allocation, as cudaMalloc() may
		// suballocate from a larger mapping
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

static int
mem_map(const struct xnvme_dev *dev, void *vaddr, size_t nbytes, uint64_t *XNVME_UNUSED(phys))
{
	size_t alloc_nbytes;
	void *base;
	int fd, err;

	if ((!vaddr) || (!nbytes)) {
		return -EINVAL;
	}

	err = xnvme_io_uring_dmabuf_ref(dev, vaddr, nbytes);
	if (err != -ENOENT) {
		return err;
	}

	err = _dmabuf_export(vaddr, &base, &alloc_nbytes, &fd);
	if (err) {
		return err;
	}

	err = xnvme_io_uring_dmabuf_add(dev, base, alloc_nbytes, fd);
	if (err) {
		close(fd);
		// Added by another thread meanwhile, which also took the reference
		return (err == -EEXIST) ? 0 : err;
	}

	return 0;
}

static int
mem_unmap(const struct xnvme_dev *dev, void *vaddr)
{
	return xnvme_io_uring_dmabuf_unref(dev, vaddr);
}

struct xnvme_be_mem g_xnvme_be_linux_mem_io_uring_dmabuf = {
	.id = "io_uring_dmabuf",
	.buf_alloc = xnvme_be_cbi_mem_posix_buf_alloc,
	.buf_realloc = xnvme_be_cbi_mem_posix_buf_realloc,
	.buf_free = xnvme_be_cbi_mem_posix_buf_free,
	.buf_vtophys = xnvme_be_cbi_mem_posix_buf_vtophys,
	.mem_map = mem_map,
	.mem_unmap = mem_unmap,
};
#endif
