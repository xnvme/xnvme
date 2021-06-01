// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_POSIX_ENABLED
#include <unistd.h>
#include <errno.h>

void *
xnvme_be_posix_buf_alloc(const struct xnvme_dev *XNVME_UNUSED(dev), size_t nbytes,
			 uint64_t *XNVME_UNUSED(phys))
{
	long sz = sysconf(_SC_PAGESIZE);

	if (sz == -1) {
		XNVME_DEBUG("FAILED: sysconf(), errno: %d", errno);
		return NULL;
	}

	return xnvme_buf_virt_alloc(sz, nbytes);
}

void *
xnvme_be_posix_buf_realloc(const struct xnvme_dev *XNVME_UNUSED(dev), void *XNVME_UNUSED(buf),
			   size_t XNVME_UNUSED(nbytes), uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: _posix: does not support realloc");
	errno = ENOSYS;
	return NULL;
}

void
xnvme_be_posix_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	xnvme_buf_virt_free(buf);
}

int
xnvme_be_posix_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *XNVME_UNUSED(buf),
			   uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: _posix: does not support phys/DMA alloc");
	return -ENOSYS;
}
#endif

struct xnvme_be_mem g_xnvme_be_posix_mem = {
#ifdef XNVME_BE_POSIX_ENABLED
	.buf_alloc = xnvme_be_posix_buf_alloc,
	.buf_realloc = xnvme_be_posix_buf_realloc,
	.buf_free = xnvme_be_posix_buf_free,
	.buf_vtophys = xnvme_be_posix_buf_vtophys,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
#endif
};
