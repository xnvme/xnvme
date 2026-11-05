// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_HOST_BUF_H
#define __INTERNAL_XNVME_HOST_BUF_H

int
xnvme_host_buf_clear(void *buf, size_t nbytes);

int
xnvme_host_buf_memcpy(void *dst, const void *src, size_t nbytes);

int
xnvme_host_buf_diff(const void *expected, const void *actual, size_t nbytes, size_t *diff,
		    bool print);

int
xnvme_host_buf_fill(void *buf, size_t nbytes, const char *content);

/**
 * Whether [vaddr, vaddr + nbytes) lies in hugetlb pages, which stay where they are
 *
 * Ordinary pages can move once their physical address is handed out, so a device that is given
 * physical addresses, without an IOMMU, can only be given memory in hugetlb pages.
 */
bool
xnvme_buf_is_hugetlb(const void *vaddr, size_t nbytes);

#endif
