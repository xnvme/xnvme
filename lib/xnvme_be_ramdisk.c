// Copyright (C) Mads Ynddal <m.ynddal@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_RAMDISK_ENABLED
#include <xnvme_be_ramdisk.h>
#include <xnvme_be_posix.h>

static struct xnvme_be_mixin g_xnvme_be_mixin_ramdisk[] = {
	{
		.mtype = XNVME_BE_MEM,
		.name = "posix",
		.descr = "Use libc malloc()/free() with sysconf for alignment",
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
		.mtype = XNVME_BE_SYNC,
		.name = "ramdisk",
		.descr = "Use memory for synchronous I/O",
		.sync = &g_xnvme_be_ramdisk_sync,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ADMIN,
		.name = "ramdisk",
		.descr = "Use memory as to construct NVMe idfy responses",
		.admin = &g_xnvme_be_ramdisk_admin,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_DEV,
		.name = "ramdisk",
		.descr = "Use memory and no device enumeration",
		.dev = &g_xnvme_be_ramdisk_dev,
		.check_support = xnvme_be_supported,
	},
};
#endif

struct xnvme_be xnvme_be_ramdisk = {
	.mem = XNVME_BE_NOSYS_MEM,
	.admin = XNVME_BE_NOSYS_ADMIN,
	.sync = XNVME_BE_NOSYS_SYNC,
	.async = XNVME_BE_NOSYS_QUEUE,
	.dev = XNVME_BE_NOSYS_DEV,
	.attr =
		{
			.name = "ramdisk",
#ifdef XNVME_BE_RAMDISK_ENABLED
			.enabled = 1,
#endif
		},
#ifdef XNVME_BE_RAMDISK_ENABLED
	.nobjs = sizeof g_xnvme_be_mixin_ramdisk / sizeof *g_xnvme_be_mixin_ramdisk,
	.objs = g_xnvme_be_mixin_ramdisk,
#endif
};
