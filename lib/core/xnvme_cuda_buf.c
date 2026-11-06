// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifdef XNVME_CUDA_ENABLED
#include <cuda.h>
#include <cuda_runtime.h>
#include <libxnvme.h>
#include <xnvme_host_buf.h>
#include <errno.h>
#include <unistd.h>

bool
xnvme_buf_is_cuda(const void *buf)
{
	cudaError_t err;
	struct cudaPointerAttributes attr;

	err = cudaPointerGetAttributes(&attr, buf);
	if (err != cudaSuccess) {
		XNVME_DEBUG("FAILED: cudaPointerGetAttributes(buf), err: %s",
			    cudaGetErrorString(err));
		return false; // If it fails we assume it is not a CUDA buffer
	}

	return attr.type == cudaMemoryTypeDevice;
}

int
xnvme_cuda_buf_clear(void *buf, size_t nbytes)
{
	cudaError_t err;

	err = cudaMemset(buf, 0, nbytes);
	if (err == cudaSuccess) {
		err = cudaStreamSynchronize(NULL);
	}
	if (err != cudaSuccess) {
		XNVME_DEBUG("FAILED: cudaMemset/cudaStreamSynchronize, err: %s",
			    cudaGetErrorString(err));
		return -EIO;
	}

	return 0;
}

int
xnvme_cuda_buf_memcpy(void *dst, const void *src, size_t nbytes)
{
	cudaError_t err;

	err = cudaMemcpy(dst, src, nbytes, cudaMemcpyDefault);
	if (err != cudaSuccess) {
		XNVME_DEBUG("FAILED: cudaMemcpy, err: %s", cudaGetErrorString(err));
		return -EIO;
	}
	return 0;
}

int
xnvme_cuda_buf_diff(const void *expected, const void *actual, size_t nbytes, size_t *diff,
		    bool print)
{
	void *exp, *act;
	cudaError_t cu_err;
	int err;

	exp = malloc(nbytes);
	if (!exp) {
		err = -errno;
		XNVME_DEBUG("FAILED: malloc(exp), errno: %d", err);
		return err;
	}

	act = malloc(nbytes);
	if (!act) {
		err = -errno;
		XNVME_DEBUG("FAILED: malloc(act), errno: %d", err);
		goto exit;
	}

	cu_err = cudaMemcpy(exp, expected, nbytes, cudaMemcpyDefault);
	if (cu_err != cudaSuccess) {
		XNVME_DEBUG("FAILED: cudaMemcpy(expected), err: %s", cudaGetErrorString(cu_err));
		err = -EIO;
		goto exit;
	}

	cu_err = cudaMemcpy(act, actual, nbytes, cudaMemcpyDefault);
	if (cu_err != cudaSuccess) {
		XNVME_DEBUG("FAILED: cudaMemcpy(actual), err: %s", cudaGetErrorString(cu_err));
		err = -EIO;
		goto exit;
	}

	err = xnvme_host_buf_diff(exp, act, nbytes, diff, print);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_host_buf_diff(), err: %d", err);
		goto exit;
	}

exit:
	free(exp);
	free(act);

	return err;
}

int
xnvme_cuda_buf_fill(void *buf, size_t nbytes, const char *content)
{
	cudaError_t cu_err;
	int err;
	uint8_t *cbuf;

	cbuf = malloc(nbytes);
	if (!cbuf) {
		err = -errno;
		XNVME_DEBUG("FAILED: malloc(cbuf), errno: %d", err);
		return err;
	}

	err = xnvme_host_buf_fill(cbuf, nbytes, content);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_host_buf_fill(%s), err: %d", content, err);
		goto exit;
	}

	cu_err = cudaMemcpy(buf, cbuf, nbytes, cudaMemcpyDefault);
	if (cu_err != cudaSuccess) {
		XNVME_DEBUG("FAILED: cudaMemcpy, err: %s", cudaGetErrorString(cu_err));
		err = -EIO;
		goto exit;
	}
exit:
	free(cbuf);
	return err;
}

// A dma-buf is exported in whole pages
static size_t
_page_ceil(size_t nbytes)
{
	long pagesize = sysconf(_SC_PAGESIZE);

	if (pagesize <= 0) {
		XNVME_DEBUG("FAILED: sysconf(), errno: %d", errno);
		return 0;
	}

	return (nbytes + (size_t)pagesize - 1) & ~((size_t)pagesize - 1);
}

int
xnvme_cuda_buf_get_range(void *vaddr, void **base, size_t *nbytes)
{
	CUdeviceptr alloc_base;
	size_t alloc_nbytes;

	/*
	 * Prefer the mapping over the allocation, as cudaMalloc() may suballocate from a larger
	 * mapping
	 */
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

	*base = (void *)alloc_base;
	*nbytes = alloc_nbytes;

	return 0;
}

int
xnvme_cuda_buf_export_dmabuf(void *base, size_t nbytes, int *fd)
{
	size_t export_nbytes;

	export_nbytes = _page_ceil(nbytes);
	if (!export_nbytes) {
		return -EINVAL;
	}

	if (cuMemGetHandleForAddressRange(fd, (CUdeviceptr)base, export_nbytes,
					  CU_MEM_RANGE_HANDLE_TYPE_DMA_BUF_FD,
					  0) != CUDA_SUCCESS) {
		XNVME_DEBUG("FAILED: cuMemGetHandleForAddressRange(%p, %zu)", base, export_nbytes);
		return -EIO;
	}

	return 0;
}
#else

#include <libxnvme.h>
#include <errno.h>

bool
xnvme_buf_is_cuda(const void *XNVME_UNUSED(buf))
{
	return false;
}

int
xnvme_cuda_buf_clear(void *XNVME_UNUSED(buf), size_t XNVME_UNUSED(nbytes))
{
	XNVME_DEBUG("FAILED: xNVMe was compiled without CUDA");
	return -ENOTSUP;
}

int
xnvme_cuda_buf_memcpy(void *XNVME_UNUSED(dst), const void *XNVME_UNUSED(src),
		      size_t XNVME_UNUSED(nbytes))
{
	XNVME_DEBUG("FAILED: xNVMe was compiled without CUDA");
	return -ENOTSUP;
}

int
xnvme_cuda_buf_diff(const void *XNVME_UNUSED(expected), const void *XNVME_UNUSED(actual),
		    size_t XNVME_UNUSED(nbytes), size_t *XNVME_UNUSED(diff),
		    bool XNVME_UNUSED(print))
{
	XNVME_DEBUG("FAILED: xNVMe was compiled without CUDA");
	return -ENOTSUP;
}

int
xnvme_cuda_buf_fill(void *XNVME_UNUSED(buf), size_t XNVME_UNUSED(nbytes),
		    const char *XNVME_UNUSED(content))
{
	XNVME_DEBUG("FAILED: xNVMe was compiled without CUDA");
	return -ENOTSUP;
}

int
xnvme_cuda_buf_get_range(void *XNVME_UNUSED(vaddr), void **XNVME_UNUSED(base),
			 size_t *XNVME_UNUSED(nbytes))
{
	XNVME_DEBUG("FAILED: xNVMe was compiled without CUDA");
	return -ENOTSUP;
}

int
xnvme_cuda_buf_export_dmabuf(void *XNVME_UNUSED(base), size_t XNVME_UNUSED(nbytes),
			     int *XNVME_UNUSED(fd))
{
	XNVME_DEBUG("FAILED: xNVMe was compiled without CUDA");
	return -ENOTSUP;
}
#endif
