// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_LINUX_H
#define __INTERNAL_XNVME_BE_LINUX_H
#include <xnvme_be_posix.h>
#include <xnvme_dev.h>

#ifndef LINUX_BLOCK_SSW
#define LINUX_BLOCK_SSW 9
#endif

#define XNVME_LINUX_CTRLR_FMT _PATH_DEV "nvme%1u"
#define XNVME_LINUX_NS_FMT    _PATH_DEV "nvme%1un%1u"

#include <xnvme_be_linux_state.h>

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

/**
 * @return -ENOSYS without BLKGETNRZONES
 */
int
xnvme_be_linux_block_nrzones(struct xnvme_dev *dev, uint64_t *nr_zones);

int
xnvme_be_linux_uapi_ver_fpr(FILE *stream, enum xnvme_pr opts);

/**
 * Implementations of the memory management interface using hugepages
 */
extern struct xnvme_be_mem g_xnvme_be_linux_mem_hugepage;

/**
 * Implementations of the admin command interface
 */
extern struct xnvme_be_admin g_xnvme_be_linux_admin_nvme;
extern struct xnvme_be_admin g_xnvme_be_linux_admin_block;

/**
 * Implementations of the synchronous command interface
 */
extern struct xnvme_be_sync g_xnvme_be_linux_sync_nvme;
extern struct xnvme_be_sync g_xnvme_be_linux_sync_block;

/**
 * Implementations of the device enumeration and handles
 */
extern struct xnvme_be_dev g_xnvme_be_linux_dev;

int
xnvme_be_linux_dev_open(struct xnvme_dev *dev);

void
xnvme_be_linux_dev_close(struct xnvme_dev *dev);

int
xnvme_be_linux_nvme_dev_nsid(struct xnvme_dev *dev);

/**
 * @return -EIO on an NVMe error status
 */
int
xnvme_be_linux_nvme_ioctl(struct xnvme_dev *dev, unsigned long ioctl_req,
			  struct xnvme_cmd_ctx *ctx);

/**
 * Fill the completion of the given command context from an NVMe status
 *
 * @param ctx Pointer to the ::xnvme_cmd_ctx that the kernel completed
 * @param ioctl_req The ioctl request that the completion came back through
 * @param status The NVMe status that the kernel returned, 0 on success
 *
 * @return On success, 0 is returned. On error, negative errno is returned.
 */
int
xnvme_be_linux_nvme_map_cpl(struct xnvme_cmd_ctx *ctx, unsigned long ioctl_req, int status);

/**
 * Fill the completion of the given command context from a kernel errno
 *
 * The errno is recorded as a vendor specific status: Status Code Type 7 with
 * the errno in the Status Code, as the other backends record errno.
 *
 * @param ctx Pointer to the ::xnvme_cmd_ctx that the kernel failed
 * @param err The positive errno that the kernel returned
 */
void
xnvme_be_linux_nvme_map_errno(struct xnvme_cmd_ctx *ctx, int err);

#endif /* __INTERNAL_XNVME_BE_LINUX */
