/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_be.h
 */

/**
 * Representation of xNVMe library backend attributes
 *
 * @struct xnvme_be_attr
 */
struct xnvme_be_attr {
	const char *name;  ///< Backend name
	const char *descr; ///< Human-readable description
	uint32_t caps;     ///< Bitmask of xnvme_be_cap

	uint8_t _rsvd[4];
};

/**
 * Returns the backend attribute at the given index
 *
 * @param idx Index into the list of backend configurations
 *
 * @return Pointer to the attribute, NULL when idx is past the end
 */
const struct xnvme_be_attr *
xnvme_be_attr_get(int idx);
