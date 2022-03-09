// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_LINUX_H
#define __INTERNAL_XNVME_BE_LINUX_H
#include <xnvme_dev.h>

#ifndef LINUX_BLOCK_SSW
#define LINUX_BLOCK_SSW 9
#endif

#define XNVME_LINUX_CTRLR_SCAN _PATH_DEV "nvme%1u%[^\n]"
#define XNVME_LINUX_NS_SCAN    _PATH_DEV "nvme%1un%1u%[^\n]"

#define XNVME_LINUX_CTRLR_FMT _PATH_DEV "nvme%1u"
#define XNVME_LINUX_NS_FMT    _PATH_DEV "nvme%1un%1u"

/**
 * Internal representation of XNVME_BE_LINUX state
 */
struct xnvme_be_linux_state {
	int fd;

	uint8_t pseudo;
	uint8_t poll_io;
	uint8_t poll_sq;

	uint8_t _rsvd[121];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_linux_state) == XNVME_BE_STATE_NBYTES, "Incorrect size")

int
xnvme_file_opts_to_linux(struct xnvme_opts *opts);

static inline uint64_t
xnvme_lba2off(struct xnvme_dev *dev, uint64_t lba)
{
	return lba << dev->geo.ssw;
}

static inline uint64_t
xnvme_off2lba(struct xnvme_dev *dev, uint64_t off)
{
	return off >> dev->geo.ssw;
}

int
xnvme_be_linux_sysfs_dev_attr_to_buf(struct xnvme_dev *dev, const char *attr, char *buf,
				     int buf_len);

int
xnvme_be_linux_sysfs_dev_attr_to_num(struct xnvme_dev *dev, const char *attr, uint64_t *val);

int
xnvme_be_linux_uapi_ver_fpr(FILE *stream, enum xnvme_pr opts);

/**
 * Implementations of the memory management interface
 */
extern struct xnvme_be_mem g_xnvme_be_posix_mem;

/**
 * Implementations of the admin command interface
 */
extern struct xnvme_be_admin g_xnvme_be_linux_admin_nvme;
extern struct xnvme_be_admin g_xnvme_be_linux_admin_block;

/**
 * Implementations of the synchronous command interface
 */
extern struct xnvme_be_sync g_xnvme_be_posix_sync_psync;
extern struct xnvme_be_sync g_xnvme_be_linux_sync_nvme;
extern struct xnvme_be_sync g_xnvme_be_linux_sync_block;

/**
 * Implementations of the asynchronous command interface
 */
extern struct xnvme_be_async g_xnvme_be_linux_async_libaio;
extern struct xnvme_be_async g_xnvme_be_linux_async_liburing;
extern struct xnvme_be_async g_xnvme_be_linux_async_ucmd;

/**
 * Implementations of the device enumeration and handles
 */
extern struct xnvme_be_dev g_xnvme_be_dev_linux;

#endif /* __INTERNAL_XNVME_BE_LINUX */
