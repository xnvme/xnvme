// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>

#ifdef XNVME_BE_LIBURING_DMABUF_ENABLED
#include <errno.h>
#include <fcntl.h>
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

	// A range inside a registered dma-buf, udmabuf host memory too, takes a reference of it
	err = xnvme_be_liburing_dmabuf_ref(dev, vaddr, nbytes, true);
	if (err != -ENOENT) {
		return err;
	}
	if (xnvme_buf_is_host(vaddr)) {
		return xnvme_buf_noreg_register(dev, vaddr, nbytes);
	}

	err = xnvme_buf_export_dmabuf(vaddr, &base, &alloc_nbytes, &fd);
	if (err) {
		return err;
	}

	err = xnvme_be_liburing_dmabuf_add(dev, base, alloc_nbytes, fd);
	if (err) {
		close(fd);
		// On -EEXIST, another thread registered it first and this call took a reference
		return (err == -EEXIST) ? 0 : err;
	}

	return 0;
}

static int
buf_register_dmabuf(const struct xnvme_dev *dev, int fd, void *vaddr)
{
	off_t size;
	size_t nbytes;
	int own, err;

	size = lseek(fd, 0, SEEK_END);
	if (size <= 0) {
		XNVME_DEBUG("FAILED: lseek(dma-buf), errno: %d", errno);
		return size ? -errno : -EINVAL;
	}
	nbytes = (size_t)size;

	// A dma-buf inside a registered one returns -EBUSY, as a reference would count its memory
	// as the registered dma-buf's
	err = xnvme_be_liburing_dmabuf_ref(dev, vaddr, nbytes, false);
	if (err != -ENOENT) {
		return err;
	}

	own = fcntl(fd, F_DUPFD_CLOEXEC, 0);
	if (own < 0) {
		return -errno;
	}

	err = xnvme_be_liburing_dmabuf_add(dev, vaddr, nbytes, own);
	if (err) {
		close(own);
		// On -EEXIST, another thread registered it first and this call took a reference
		return (err == -EEXIST) ? 0 : err;
	}

	return 0;
}

static int
buf_unregister(const struct xnvme_dev *dev, void *vaddr)
{
	int err;

	err = xnvme_be_liburing_dmabuf_unref(dev, vaddr);
	if ((err == -ENOENT) && xnvme_buf_is_host(vaddr)) {
		return xnvme_buf_noreg_unregister(dev, vaddr);
	}

	return err;
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
	.buf_register_dmabuf = buf_register_dmabuf,
	.buf_unregister = buf_unregister,
	.buf_get_alignment = buf_get_alignment,
};
#endif
