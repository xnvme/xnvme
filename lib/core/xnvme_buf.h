// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BUF_H
#define __INTERNAL_XNVME_BUF_H

/**
 * Register memory where the OS pins and translates the payload of each command itself
 *
 * Any memory then serves as command payload, so there is nothing to register; the arguments are
 * checked and nothing is recorded.
 */
int
xnvme_buf_noreg_register(const struct xnvme_dev *dev, void *vaddr, size_t nbytes);

/**
 * Unregister memory registered with xnvme_buf_noreg_register()
 */
int
xnvme_buf_noreg_unregister(const struct xnvme_dev *dev, void *vaddr);

/**
 * The alignment of memory registered with xnvme_buf_noreg_register(): that of the I/O path
 */
int
xnvme_buf_noreg_get_alignment(const struct xnvme_dev *dev, size_t *align);

#endif /* __INTERNAL_XNVME_BUF_H */
