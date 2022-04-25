/**
 * Cross-platform I/O library for NVMe based devices
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file libxnvme_be.h
 */
#ifndef __LIBXNVME_BE_H
#define __LIBXNVME_BE_H
#include <libxnvme.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Representation of xNVMe library backend attributes
 *
 * @struct xnvme_be_attr_list
 */
struct xnvme_be_attr {
	const char *name; ///< Backend name

	uint8_t enabled; ///< Whether the backend is 'enabled'

	uint8_t _rsvd[15];
};

/**
 * List of xNVMe library backend attributes
 *
 * @struct xnvme_be_attr_list
 */
struct xnvme_be_attr_list {
	uint32_t capacity;           ///< Remaining unused entries
	int count;                   ///< Number of used entries
	struct xnvme_be_attr item[]; ///< Array of items
};

/**
 * List backends bundled with the library
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_be_attr_list_bundled(struct xnvme_be_attr_list **list);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_BE_H */
