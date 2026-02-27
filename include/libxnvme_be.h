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
 * @struct xnvme_be_attr
 */
struct xnvme_be_attr {
	const char *name;  ///< Backend name
	const char *descr; ///< Human-readable description
	uint32_t caps;     ///< Bitmask of xnvme_be_cap (0 = legacy/mixin-based)

	uint8_t _rsvd[4];
};

/**
 * Prints the given backend attribute to the given output stream
 *
 * @param stream output stream used for printing
 * @param attr Pointer to the ::xnvme_be_attr to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_attr_fpr(FILE *stream, const struct xnvme_be_attr *attr, int opts);

/**
 * Prints the given backend attribute to stdout
 *
 * @param attr Pointer to the ::xnvme_be_attr to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_attr_pr(const struct xnvme_be_attr *attr, int opts);

/**
 * Prints platform name and available drivers to the given output stream
 *
 * @param stream output stream used for printing
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_registry_fpr(FILE *stream, enum xnvme_pr opts);

/**
 * Prints platform name and available drivers to stdout
 *
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_registry_pr(enum xnvme_pr opts);
