// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_FBSD_ENABLED
#include <xnvme_be_posix.h>
#include <xnvme_be_fbsd.h>

static struct xnvme_be_mixin g_xnvme_be_mixin_fbsd[] = {
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
		.name = "psync",
		.descr = "Use pread()/write() for synchronous I/O and FreeBSD preadv()/pwritev()",
		.sync = &g_xnvme_be_fbsd_sync_psync,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_SYNC,
		.name = "nvme",
		.descr = "Use FreeBSD NVMe Driver ioctl() for sync commands",
		.sync = &g_xnvme_be_fbsd_sync_nvme,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ADMIN,
		.name = "nvme",
		.descr = "Use FreeBSD NVMe Driver ioctl() for admin commands",
		.admin = &g_xnvme_be_fbsd_admin_nvme,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_DEV,
		.name = "fbsd",
		.descr = "Use FreeBSD file/dev handles and enumerate NVMe devices",
		.dev = &g_xnvme_be_fbsd_dev,
		.check_support = xnvme_be_supported,
	},
};
#endif

struct xnvme_be xnvme_be_fbsd = {
	.admin = XNVME_BE_NOSYS_ADMIN,
	.async = XNVME_BE_NOSYS_QUEUE,
	.dev = XNVME_BE_NOSYS_DEV,
	.mem = XNVME_BE_NOSYS_MEM,
	.sync = XNVME_BE_NOSYS_SYNC,
	.attr =
		{
#ifdef XNVME_BE_FBSD_ENABLED
			.enabled = 1,
#endif
			.name = "fbsd",
		},
#ifdef XNVME_BE_FBSD_ENABLED
	.nobjs = sizeof g_xnvme_be_mixin_fbsd / sizeof *g_xnvme_be_mixin_fbsd,
	.objs = g_xnvme_be_mixin_fbsd,
#endif
};
