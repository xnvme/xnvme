// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_LINUX_IO_URING_DMABUF_H
#define __INTERNAL_XNVME_BE_LINUX_IO_URING_DMABUF_H
#include <stddef.h>
#include <stdint.h>

// Size of the buffer-table of every ring, and so the most dma-bufs a device maps
#define XNVME_IO_URING_DMABUF_NENTRIES 256

struct xnvme_dev;
struct xnvme_queue_liburing;
struct xnvme_io_uring_dmabuf;

int
xnvme_io_uring_dmabuf_ref(const struct xnvme_dev *dev, const void *vaddr, size_t nbytes);

int
xnvme_io_uring_dmabuf_add(const struct xnvme_dev *dev, void *vaddr, size_t nbytes, int fd);

int
xnvme_io_uring_dmabuf_unref(const struct xnvme_dev *dev, const void *vaddr);

int
xnvme_io_uring_dmabuf_lookup(struct xnvme_io_uring_dmabuf *registry, const void *buf,
			     size_t nbytes, uint16_t *index, uint64_t *offset);

int
xnvme_io_uring_dmabuf_attach(struct xnvme_queue_liburing *queue);

void
xnvme_io_uring_dmabuf_detach(struct xnvme_queue_liburing *queue);

void
xnvme_io_uring_dmabuf_term(const struct xnvme_dev *dev);

#endif /* __INTERNAL_XNVME_BE_LINUX_IO_URING_DMABUF_H */
