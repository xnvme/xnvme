// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_MEM_H
#define __INTERNAL_XNVME_MEM_H

#include <libxnvme.h>

/**
 * Map memory for a memory interface that takes any memory for IO, as where the kernel pins and
 * translates the payload of each command itself: checks the arguments and records nothing
 */
int
xnvme_mem_noreq_map(const struct xnvme_dev *dev, void *vaddr, size_t nbytes, uint64_t *phys);

/**
 * Unmap memory mapped with xnvme_mem_noreq_map()
 */
int
xnvme_mem_noreq_unmap(const struct xnvme_dev *dev, void *buf);

/**
 * Fill in the requirements of a memory interface that takes any memory: the least alignment the
 * platform allocates with, that of a pointer
 */
int
xnvme_mem_noreq_map_req(const struct xnvme_dev *dev, struct xnvme_mem_map_req *req);

#endif /* __INTERNAL_XNVME_MEM_H */
