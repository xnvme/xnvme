/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_libconf.h
 */

#ifndef __LIBXNVME_LIBCONF_H
#define __LIBXNVME_LIBCONF_H
#include <libxnvme.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A collection of strings describing the build-options and linkage of the xNVMe library
 *
 * @struct xnvme_libconf
 */
struct xnvme_libconf {
	const char **entries;
};

/**
 * Returns a ::xnvme_libconf
 *
 * @return Pointer to a ::xnvme_libconf
 */
const struct xnvme_libconf *
xnvme_libconf_get(void);

/**
 * Prints the given array of version-strings to the given output stream
 *
 * @param stream Output stream used for printing
 * @param libconf Pointer to the ::xnvme_libconf to print
 * @param opts Printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_libconf_fpr(FILE *stream, const struct xnvme_libconf *libconf, enum xnvme_pr opts);

/**
 * Prints the given array of version-strings to stdout
 *
 * @param libconf Pointer to the ::xnvme_libconf to print
 * @param opts Printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_libconf_pr(const struct xnvme_libconf *libconf, enum xnvme_pr opts);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_LIBCONF_H */
