/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_be.h
 */

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
