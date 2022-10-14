// Copyright (C) Rishabh Shukla <rishabh.sh@samsung.com>
// Copyright (C) Pranjal Dave <pranjal.58@partner.samsung.com>
// SPDX-License-Identifier: Apache-2.0
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

/**
 * Internal representation of XNVME_BE_WINDOWS_STATE
 */
struct xnvme_be_windows_state {
	void *sync_handle;
	void *async_handle;
	int fd;
	uint8_t _rsvd[108]; ///< reserved space
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_windows_state) == XNVME_BE_STATE_NBYTES,
		    "Incorrect size")

/**
 * Prints the information about the  Windows Version
 *
 * @param stream output stream used for printing
 * @param opts
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_be_windows_uapi_ver_fpr(FILE *stream, enum xnvme_pr opts);
int
xnvme_file_creation_opts_to_windows(struct xnvme_opts *opts);
int
xnvme_file_access_opts_to_windows(struct xnvme_opts *opts);
int
xnvme_file_attributes_opts_to_windows(struct xnvme_opts *opts);

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
extern struct xnvme_be_async g_xnvme_be_posix_async_emu;
extern struct xnvme_be_async g_xnvme_be_posix_async_thrpool;
extern struct xnvme_be_async g_xnvme_be_windows_async_iocp;
extern struct xnvme_be_async g_xnvme_be_windows_async_iocp_th;
extern struct xnvme_be_async g_xnvme_be_windows_async_ioring;

/**
 * Implementations of the memory management interface
 */
extern struct xnvme_be_mem g_xnvme_be_windows_mem;

/**
 * Implementations of the device enumeration and handles
 */
extern struct xnvme_be_dev g_xnvme_be_dev_windows;

#endif /* __INTERNAL_XNVME_BE_WINDOWS */
