// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_ENABLED
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <libxnvme.h>
#include <libxnvme_be.h>
#include <libxnvme_file.h>
#include <libxnvme_spec_fs.h>
#include <libxnvme_adm.h>
#include <libxnvme_znd.h>
#include <xnvme_dev.h>
#include <xnvme_be_posix.h>
#include <xnvme_be_linux.h>

int
xnvme_be_linux_uapi_ver_fpr(FILE *stream, enum xnvme_pr opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "linux;LINUX_VERSION_CODE-UAPI/%d-%d.%d.%d", LINUX_VERSION_CODE,
			(LINUX_VERSION_CODE & (0xff << 16)) >> 16,
			(LINUX_VERSION_CODE & (0xff << 8)) >> 8, LINUX_VERSION_CODE & 0xff);

	return wrtn;
}

int
xnvme_file_opts_to_linux(struct xnvme_opts *opts)
{
	int flags = 0;

	flags |= opts->create ? O_CREAT : 0x0;
	flags |= opts->direct ? O_DIRECT : 0x0;
	flags |= opts->rdonly ? O_RDONLY : 0x0;
	flags |= opts->wronly ? O_WRONLY : 0x0;
	flags |= opts->rdwr ? O_RDWR : 0x0;
	flags |= opts->truncate ? O_TRUNC : 0x0;

	return flags;
}

static struct xnvme_be_mixin g_xnvme_be_mixin_linux[] = {
	{
		.mtype = XNVME_BE_MEM,
		.name = "posix",
		.descr = "Use C11 lib malloc/free with sysconf for alignment",
		.mem = &g_xnvme_be_posix_mem,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ASYNC,
		.name = "emu",
		.descr = "Use emulated asynchronous I/O",
		.async = &g_xnvme_be_posix_async_emu,
		.check_support = xnvme_be_supported,
	},
#ifdef XNVME_BE_LINUX_LIBURING_ENABLED
	{
		.mtype = XNVME_BE_ASYNC,
		.name = "io_uring",
		.descr = "Use Linux io_uring/liburing for Asynchronous I/O",
		.async = &g_xnvme_be_linux_async_liburing,
		.check_support = xnvme_be_supported,
	},
	{
		.mtype = XNVME_BE_ASYNC,
		.name = "io_uring_cmd",
		.descr = "Use Linux io_uring passthru command for Asynchronous I/O",
		.async = &g_xnvme_be_linux_async_ucmd,
		.check_support = xnvme_be_supported,
	},
#endif
#ifdef XNVME_BE_LINUX_LIBAIO_ENABLED
	{
		.mtype = XNVME_BE_ASYNC,
		.name = "libaio",
		.descr = "Use Linux aio for Asynchronous I/O",
		.async = &g_xnvme_be_linux_async_libaio,
		.check_support = xnvme_be_supported,
	},
#endif
	{
		.mtype = XNVME_BE_ASYNC,
		.name = "posix",
		.descr = "Use POSIX aio for Asynchronous I/O",
		.async = &g_xnvme_be_posix_async_aio,
		.check_support = xnvme_be_supported,
	},
	{
		.mtype = XNVME_BE_ASYNC,
		.name = "thrpool",
		.descr = "Use thread pool for Asynchronous I/O",
		.async = &g_xnvme_be_posix_async_thrpool,
		.check_support = xnvme_be_supported,
	},
	{
		.mtype = XNVME_BE_ASYNC,
		.name = "nil",
		.descr = "Use nil-io; For introspective perf. evaluation",
		.async = &g_xnvme_be_posix_async_nil,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_SYNC,
		.name = "nvme",
		.descr = "Use Linux NVMe Driver ioctl() for synchronous I/O",
		.sync = &g_xnvme_be_linux_sync_nvme,
		.check_support = xnvme_be_supported,
	},
	{
		.mtype = XNVME_BE_SYNC,
		.name = "psync",
		.descr = "Use pread()/write() for synchronous I/O",
		.sync = &g_xnvme_be_posix_sync_psync,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ADMIN,
		.name = "nvme",
		.descr = "Use Linux NVMe Driver ioctl() for admin commands",
		.admin = &g_xnvme_be_linux_admin_nvme,
		.check_support = xnvme_be_supported,
	},
#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
	{
		.mtype = XNVME_BE_SYNC,
		.name = "block",
		.descr = "Use Linux Block Layer ioctl() and pread()/pwrite() for I/O",
		.sync = &g_xnvme_be_linux_sync_block,
		.check_support = xnvme_be_supported,
	},
	{
		.mtype = XNVME_BE_ADMIN,
		.name = "block",
		.descr = "Use Linux Block Layer ioctl() and sysfs for admin commands",
		.admin = &g_xnvme_be_linux_admin_block,
		.check_support = xnvme_be_supported,
	},
#endif

	{
		.mtype = XNVME_BE_DEV,
		.name = "linux",
		.descr = "Use Linux file/dev handles and enumerate NVMe devices",
		.dev = &g_xnvme_be_dev_linux,
		.check_support = xnvme_be_supported,
	},
};
// static const int
// g_xnvme_be_mixin_linux_nobjs = sizeof g_xnvme_be_mixin_linux / sizeof * g_xnvme_be_mixin_linux;
#else
int
xnvme_be_linux_uapi_ver_fpr(FILE *stream, enum xnvme_pr XNVME_UNUSED(opts))
{
	return fprintf(stream, "linux;LINUX_VERSION_CODE-UAPI/NOSYS\n");
}
#endif

struct xnvme_be xnvme_be_linux = {
	.mem = XNVME_BE_NOSYS_MEM,
	.admin = XNVME_BE_NOSYS_ADMIN,
	.sync = XNVME_BE_NOSYS_SYNC,
	.async = XNVME_BE_NOSYS_QUEUE,
	.dev = XNVME_BE_NOSYS_DEV,
	.attr =
		{
#ifdef XNVME_BE_LINUX_ENABLED
			.enabled = 1,
#endif
			.name = "linux",
		},
	.state = {0},
#ifdef XNVME_BE_LINUX_ENABLED
	.nobjs = sizeof g_xnvme_be_mixin_linux / sizeof *g_xnvme_be_mixin_linux,
	.objs = g_xnvme_be_mixin_linux,
#endif
};
