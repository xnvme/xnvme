// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_SPDK_ENABLED
#include <errno.h>
#include <spdk/env.h>
#include <xnvme_dev.h>

void *
xnvme_be_spdk_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys)
{
	const size_t alignment = dev->geo.nbytes ? dev->geo.nbytes : 4096;
	void *buf;

	buf = spdk_dma_malloc(nbytes, alignment, phys);
	if (!buf) {
		errno = ENOMEM;
		return NULL;
	}

	return buf;
}

void *
xnvme_be_spdk_buf_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes, uint64_t *phys)
{
	const size_t alignment = dev->geo.nbytes ? dev->geo.nbytes : 4096;
	void *rebuf;

	rebuf = spdk_dma_realloc(buf, nbytes, alignment, phys);
	if (!rebuf) {
		errno = ENOMEM;
		return NULL;
	}

	return rebuf;
}

void
xnvme_be_spdk_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	spdk_dma_free(buf);
}

int
xnvme_be_spdk_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf, uint64_t *phys)
{
	*phys = spdk_vtophys(buf, NULL);
	if (SPDK_VTOPHYS_ERROR == *phys) {
		return -EIO;
	}
	return 0;
}
#endif

struct xnvme_be_mem g_xnvme_be_spdk_mem = {
	.id = "spdk",
#ifdef XNVME_BE_SPDK_ENABLED
	.buf_alloc = xnvme_be_spdk_buf_alloc,
	.buf_realloc = xnvme_be_spdk_buf_realloc,
	.buf_free = xnvme_be_spdk_buf_free,
	.buf_vtophys = xnvme_be_spdk_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#endif
};
