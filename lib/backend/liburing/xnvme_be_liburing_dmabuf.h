// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_LIBURING_DMABUF_H
#define __INTERNAL_XNVME_BE_LIBURING_DMABUF_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Size of the buffer-table of every ring, and so the most dma-bufs a device maps
#define XNVME_BE_LIBURING_DMABUF_NENTRIES 256

struct xnvme_dev;
struct xnvme_queue_liburing;
struct xnvme_be_liburing_dmabuf;

#include <xnvme_buf.h>

// Returns 0 when the kernel can register dma-bufs with io_uring, -ENOTSUP when not
int
xnvme_be_liburing_dmabuf_supported(void);

// With 'within' false, -EBUSY when vaddr lies inside a registration rather than at its start
int
xnvme_be_liburing_dmabuf_ref(const struct xnvme_dev *dev, const void *vaddr, size_t nbytes,
			     bool within);

// Records xfer for the buffer helpers while the dma-buf is registered
int
xnvme_be_liburing_dmabuf_add(const struct xnvme_dev *dev, void *vaddr, size_t nbytes, int fd,
			     enum xnvme_buf_xfer xfer);

int
xnvme_be_liburing_dmabuf_unref(const struct xnvme_dev *dev, const void *vaddr);

int
xnvme_be_liburing_dmabuf_lookup(struct xnvme_be_liburing_dmabuf *registry, const void *buf,
				size_t nbytes, uint16_t *index, uint64_t *offset);

int
xnvme_be_liburing_dmabuf_attach(struct xnvme_queue_liburing *queue);

void
xnvme_be_liburing_dmabuf_detach(struct xnvme_queue_liburing *queue);

void
xnvme_be_liburing_dmabuf_term(const struct xnvme_dev *dev);

#endif /* __INTERNAL_XNVME_BE_LIBURING_DMABUF_H */
