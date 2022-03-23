// Copyright (C) Mads Ynddal <m.ynddal@samsung.com>
// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_MACOS_ENABLED
#include <xnvme_be_posix.h>
#include <xnvme_be_macos.h>

static struct xnvme_be_mixin g_xnvme_be_mixin_macos[] = {
	{
		.mtype = XNVME_BE_MEM,
		.name = "posix",
		.descr = "Use C11 lib malloc/free with sysconf for alignment",
		.mem = &g_xnvme_be_posix_mem,
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
		.name = "posix",
		.descr = "Use POSIX aio for Asynchronous I/O",
		.async = &g_xnvme_be_posix_async_aio,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_SYNC,
		.name = "psync",
		.descr = "Use pread()/write() for synchronous I/O and preadv()/pwritev()",
		.sync = &g_xnvme_be_macos_sync_psync,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ADMIN,
		.name = "file_as_ns",
		.descr = "Use file-stat as to construct NVMe idfy responses",
		.admin = &g_xnvme_be_posix_admin_shim,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_DEV,
		.name = "posix",
		.descr = "Use POSIX file/dev handles and no device enumeration",
		.dev = &g_xnvme_be_posix_dev,
		.check_support = xnvme_be_supported,
	},
};
#endif

struct xnvme_be xnvme_be_macos = {
	.admin = XNVME_BE_NOSYS_ADMIN,
	.async = XNVME_BE_NOSYS_QUEUE,
	.dev = XNVME_BE_NOSYS_DEV,
	.mem = XNVME_BE_NOSYS_MEM,
	.sync = XNVME_BE_NOSYS_SYNC,
	.attr =
		{
#ifdef XNVME_BE_MACOS_ENABLED
			.enabled = 1,
#endif
			.name = "macos",
		},
#ifdef XNVME_BE_MACOS_ENABLED
	.nobjs = sizeof g_xnvme_be_mixin_macos / sizeof *g_xnvme_be_mixin_macos,
	.objs = g_xnvme_be_mixin_macos,
#endif
};
