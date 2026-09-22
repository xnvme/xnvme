// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_LINUX_DMABUF_IOMMU_H
#define __INTERNAL_XNVME_BE_LINUX_DMABUF_IOMMU_H
#include <stdint.h>

/**
 * Determine whether an IOMMU translates for the given device
 *
 * Reads sysfs and needs no module, which is what lets a registration be refused
 * rather than left to transfer nothing.
 *
 * @return 1 when an IOMMU translates, 0 when it does not, negative errno on
 * error
 */
int
xnvme_be_linux_dmabuf_iommu_translates(const struct xnvme_dev *dev);

/**
 * Install the physical range behind 'dmabuf_fd' in the device's IOMMU domain
 *
 * Mapped at an IOVA equal to the physical address, since the addresses reaching
 * the device are the ones the exporting driver hands the kernel, not ones this
 * gets to choose.
 *
 * @param handle Where to store the handle the mapping is released by
 *
 * @return On success, 0 is returned. On error, negative errno
 */
int
xnvme_be_linux_dmabuf_iommu_map(const struct xnvme_dev *dev, int dmabuf_fd, uint64_t *handle);

/**
 * Release a mapping installed by xnvme_be_linux_dmabuf_iommu_map()
 */
void
xnvme_be_linux_dmabuf_iommu_unmap(uint64_t handle);

#endif /* __INTERNAL_XNVME_BE_LINUX_DMABUF_IOMMU_H */
