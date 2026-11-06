// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>

#ifdef XNVME_BE_LIBURING_DMABUF_ENABLED
#include <errno.h>
#include <unistd.h>
#include <xnvme_buf.h>
#include <xnvme_be_posix.h>
#include <xnvme_be_liburing_dmabuf.h>

static int
buf_register(const struct xnvme_dev *dev, void *vaddr, size_t nbytes)
{
	size_t alloc_nbytes;
	void *base;
	int fd, err;

	if ((!vaddr) || (!nbytes)) {
		return -EINVAL;
	}

	if (xnvme_buf_is_host(vaddr)) {
		return xnvme_buf_noreg_register(dev, vaddr, nbytes);
	}

	err = xnvme_be_liburing_dmabuf_ref(dev, vaddr, nbytes);
	if (err != -ENOENT) {
		return err;
	}

	err = xnvme_buf_export_dmabuf(vaddr, &base, &alloc_nbytes, &fd);
	if (err) {
		return err;
	}

	err = xnvme_be_liburing_dmabuf_add(dev, base, alloc_nbytes, fd);
	if (err) {
		close(fd);
		// Added by another thread meanwhile, which also took the reference
		return (err == -EEXIST) ? 0 : err;
	}

	return 0;
}

static int
buf_unregister(const struct xnvme_dev *dev, void *vaddr)
{
	if (xnvme_buf_is_host(vaddr)) {
		return xnvme_buf_noreg_unregister(dev, vaddr);
	}

	return xnvme_be_liburing_dmabuf_unref(dev, vaddr);
}

static int
buf_get_alignment(const struct xnvme_dev *XNVME_UNUSED(dev), size_t *align)
{
	long pagesize;

	pagesize = sysconf(_SC_PAGESIZE);
	if (pagesize == -1) {
		XNVME_DEBUG("FAILED: sysconf(), errno: %d", errno);
		return -errno;
	}

	// A dma-buf is exported and registered in whole pages
	*align = (size_t)pagesize;

	return 0;
}

struct xnvme_be_mem g_xnvme_be_liburing_mem_dmabuf = {
	.id = "io_uring_dmabuf",
	.buf_alloc = xnvme_be_posix_mem_buf_alloc,
	.buf_realloc = xnvme_be_posix_mem_buf_realloc,
	.buf_free = xnvme_be_posix_mem_buf_free,
	.buf_vtophys = xnvme_be_posix_mem_buf_vtophys,
	.buf_register = buf_register,
	.buf_unregister = buf_unregister,
	.buf_get_alignment = buf_get_alignment,
};
#endif
