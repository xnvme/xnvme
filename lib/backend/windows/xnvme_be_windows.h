// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifndef __INTERNAL_XNVME_BE_WINDOWS_H
#define __INTERNAL_XNVME_BE_WINDOWS_H
#include <xnvme_dev.h>

/**
 * Internal representation of options for Windows backends
 */
struct xnvme_be_windows_opts {
	int flags;
};

#include <xnvme_be_windows_state.h>

/**
 * Prints the information about the  Windows Version
 *
 * @param stream output stream used for printing
 * @param opts
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_windows_uapi_ver_fpr(FILE *stream, enum xnvme_pr opts);

/**
 * Implementations of the admin command interface
 */
extern struct xnvme_be_admin g_xnvme_be_windows_admin_fs;
extern struct xnvme_be_admin g_xnvme_be_windows_admin_nvme;
extern struct xnvme_be_admin g_xnvme_be_windows_admin_block;

/**
 * Implementations of the synchronous command interface
 */
extern struct xnvme_be_sync g_xnvme_be_windows_sync_fs;
extern struct xnvme_be_sync g_xnvme_be_windows_sync_nvme;

/**
 * Implementations of the asynchronous command interface
 */

/**
 * Implementations of the memory management interface
 */
extern struct xnvme_be_mem g_xnvme_be_windows_mem;

/**
 * Implementations of the device enumeration and handles
 */
extern struct xnvme_be_dev g_xnvme_be_windows_dev;

#endif /* __INTERNAL_XNVME_BE_WINDOWS */
