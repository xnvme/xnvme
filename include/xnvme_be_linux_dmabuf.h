// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_LINUX_DMABUF_H
#define __INTERNAL_XNVME_BE_LINUX_DMABUF_H
#include <stddef.h>
#include <stdint.h>

/**
 * Add a dma-buf mapping to the registry of the given device
 *
 * Maps the dma-buf into the IOMMU of the device when needed. On success, the
 * registry takes ownership of 'fd' and the entry starts with one reference.
 *
 * @return On success, the registry index is returned. On error, negative errno
 */
int
xnvme_be_linux_dmabuf_add(const struct xnvme_dev *dev, void *vaddr, size_t nbytes, int fd);

/**
 * Take another reference on the mapping at the given registry index
 *
 * @return On success, 0 is returned. On error, negative errno
 */
int
xnvme_be_linux_dmabuf_ref(const struct xnvme_dev *dev, uint16_t index);

/**
 * Drop a reference on the mapping at the given registry index, releasing it on
 * the last one
 *
 * @return On success, 0 is returned. On error, negative errno
 */
int
xnvme_be_linux_dmabuf_del(const struct xnvme_dev *dev, uint16_t index);

/**
 * Find the mapping containing all of the given buffer
 *
 * @return On success, 0 is returned. On miss, -ENOENT
 */
int
xnvme_be_linux_dmabuf_lookup(const struct xnvme_dev *dev, const void *buf, size_t nbytes,
			     uint16_t *index, uint64_t *offset);

/**
 * Retrieve the generation of the registry, which changes on every add and del
 *
 * @param nentries Where to store the number of entries, used and free
 *
 * @return The generation, or 0 when the device has no registry
 */
uint32_t
xnvme_be_linux_dmabuf_generation(const struct xnvme_dev *dev, uint32_t *nentries);

/**
 * Retrieve the dma-buf descriptor of the mapping at the given registry index
 *
 * @return On success, the descriptor is returned. On error, negative errno
 */
int
xnvme_be_linux_dmabuf_fd(const struct xnvme_dev *dev, uint16_t index);

/**
 * Release the registry of the given device and all of its mappings
 */
void
xnvme_be_linux_dmabuf_term(const struct xnvme_dev *dev);

#endif /* __INTERNAL_XNVME_BE_LINUX_DMABUF_H */
