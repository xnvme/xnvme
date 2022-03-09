// Copyright (C) Rishabh Shukla <rishabh.sh@samsung.com>
// Copyright (C) Pranjal Dave <pranjal.58@partner.samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_WINDOWS_ENABLED
#include <errno.h>
#include <windows.h>
#include <xnvme_dev.h>

/**
 * Allocate a buffer for IO with the given device for the Windows Backend
 *
 * @param nbytes The size of the allocated buffer in bytes
 *
 * @return On success, a pointer to the allocated memory is returned. On error, NULL is returned
 * and `errno` set to indicate the error.
 */
void *
xnvme_be_windows_buf_alloc(const struct xnvme_dev *XNVME_UNUSED(dev), size_t nbytes,
			   uint64_t *XNVME_UNUSED(phys))
{
	// TODO: register buffer when async=iou

	// NOTE: Assign virt to phys?
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	return xnvme_buf_virt_alloc(si.dwPageSize, nbytes);
}

/**
 * buffer reallocate implemented for Windows Backend
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param buf The buffer to reallocate
 * @param nbytes The size of the allocated buffer in bytes
 *
 * @return On success, a pointer to the allocated memory is returned. On error, NULL is returned
 * and `errno` set to indicate the error.
 */
void *
xnvme_be_windows_buf_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes,
			     uint64_t *XNVME_UNUSED(phys))
{
	const size_t alignment = dev->geo.nbytes ? dev->geo.nbytes : 4096;
	void *rebuf;

	rebuf = _aligned_realloc(buf, nbytes, alignment);
	if (!rebuf) {
		errno = ENOMEM;
		return NULL;
	}

	return rebuf;
}

/**
 * Free the given IO buffer allocated with xnvme_buf_alloc()
 *
 * @param buf Pointer to a buffer allocated with xnvme_buf_alloc()
 */
void
xnvme_be_windows_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	xnvme_buf_virt_free(buf);
}

/**
 * Allocate a buffer for IO with the given device for the Windows Backend
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param buf The buffer to allocate respective physical memory
 * @param phys physical address
 *
 * @return On success, 0 is returned. On error, GetLastError is returned.
 */
int
xnvme_be_windows_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *XNVME_UNUSED(buf),
			     uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: xnvme_be_windows: does not support phys/DMA alloc");
	return -ENOSYS;
}
#endif

struct xnvme_be_mem g_xnvme_be_windows_mem = {
#ifdef XNVME_BE_WINDOWS_ENABLED
	.buf_alloc = xnvme_be_windows_buf_alloc,
	.buf_realloc = xnvme_be_windows_buf_realloc,
	.buf_free = xnvme_be_windows_buf_free,
	.buf_vtophys = xnvme_be_windows_buf_vtophys,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
#endif
};
