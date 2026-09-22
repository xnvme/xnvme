// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_LINUX_DMABUF_H
#define __INTERNAL_XNVME_BE_LINUX_DMABUF_H
#include <stddef.h>
#include <stdint.h>

/**
 * Number of dma-buf mappings a device can hold
 *
 * A registry index is the 'sqe->buf_index' of the io_uring buffer-table, which
 * is registered with a fixed number of slots.
 */
#define XNVME_BE_LINUX_DMABUF_MAX 1024

/**
 * Add a dma-buf mapping to the registry of the given device
 *
 * The registry takes ownership of 'fd' and closes it on removal, and of
 * 'map_handle', the IOMMU mapping released along with it; zero when the device
 * needs none. The entry starts with one reference.
 *
 * @return On success, the registry index is returned. On error, negative errno
 */
int
xnvme_be_linux_dmabuf_add(const struct xnvme_dev *dev, void *vaddr, size_t nbytes, int fd,
			  uint64_t map_handle);

/**
 * Take another reference on the mapping at the given registry index
 *
 * Allocations are what a mapping covers, and a runtime packs several buffers
 * into one, so the same mapping is arrived at more than once.
 *
 * @return On success, 0 is returned. On error, negative errno
 */
int
xnvme_be_linux_dmabuf_ref(const struct xnvme_dev *dev, uint16_t index);

/**
 * Drop a reference to the mapping at the given registry index
 *
 * Released once the last reference is gone.
 *
 * @return On success, 0 is returned. On error, negative errno
 */
int
xnvme_be_linux_dmabuf_del(const struct xnvme_dev *dev, uint16_t index);

/**
 * Resolve a buffer to the mapping containing it
 *
 * @param nbytes Length of the buffer; the mapping has to contain all of it
 *
 * @return On success, 0 is returned. On miss, -ENOENT
 */
int
xnvme_be_linux_dmabuf_lookup(const struct xnvme_dev *dev, const void *buf, size_t nbytes,
			     uint16_t *index, uint64_t *offset);

/**
 * Retrieve the generation of the registry of the given device
 *
 * It changes whenever a mapping is added or removed, so a consumer that kept
 * the one it last acted on can tell that it is behind.
 *
 * @param nslots Where to store the number of slots in use, an upper bound on
 * the indexes worth asking about
 *
 * @return The generation, or 0 when the device has no registry
 */
uint32_t
xnvme_be_linux_dmabuf_generation(const struct xnvme_dev *dev, uint32_t *nslots);

/**
 * Retrieve the dma-buf descriptor of the mapping at the given registry index
 *
 * @return On success, the descriptor is returned. On error, negative errno
 */
int
xnvme_be_linux_dmabuf_fd(const struct xnvme_dev *dev, uint16_t index);

/**
 * Release the registry of the given device, closing the descriptors it holds
 */
void
xnvme_be_linux_dmabuf_term(const struct xnvme_dev *dev);

#endif /* __INTERNAL_XNVME_BE_LINUX_DMABUF_H */
