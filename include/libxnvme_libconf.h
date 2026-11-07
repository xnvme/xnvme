/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_libconf.h
 */

#ifndef __LIBXNVME_LIBCONF_H
#define __LIBXNVME_LIBCONF_H

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

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_LIBCONF_H */
