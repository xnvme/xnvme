// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_RAMDISK_ENABLED
#include <xnvme_be_ramdisk.h>
#include <xnvme_be_cbi.h>
#ifdef XNVME_BE_LINUX_ENABLED
#include <xnvme_be_linux.h>
#endif
#ifdef XNVME_BE_WINDOWS_ENABLED
#include <xnvme_be_windows.h>
#endif

static struct xnvme_be_mixin g_xnvme_be_mixin_ramdisk[] = {
#ifdef XNVME_BE_CBI_MEM_POSIX_ENABLED
	{
		.mtype = XNVME_BE_MEM,
		.name = "posix",
		.descr = "Use libc malloc()/free() with sysconf for alignment",
		.mem = &g_xnvme_be_cbi_mem_posix,
		.check_support = xnvme_be_supported,
	},
#endif
#ifdef XNVME_BE_LINUX_ENABLED
	{
		.mtype = XNVME_BE_MEM,
		.name = "hugepage",
		.descr = "Allocate buffers using hugepages via mmap on hugetlbfs",
		.mem = &g_xnvme_be_linux_mem_hugepage,
		.check_support = xnvme_be_supported,
	},
#endif
#ifdef XNVME_BE_WINDOWS_ENABLED
	{
		.mtype = XNVME_BE_MEM,
		.name = "windows",
		.descr = "Use Windows memory allocator",
		.mem = &g_xnvme_be_windows_mem,
		.check_support = xnvme_be_supported,
	},
#endif

	{
		.mtype = XNVME_BE_ASYNC,
		.name = "nil",
		.descr = "Use nil for Asynchronous I/O",
		.async = &g_xnvme_be_cbi_async_nil,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ASYNC,
		.name = "thrpool",
		.descr = "Use thread pool for Asynchronous I/O",
		.async = &g_xnvme_be_cbi_async_thrpool,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ASYNC,
		.name = "emu",
		.descr = "Use emu pool for Asynchronous I/O",
		.async = &g_xnvme_be_cbi_async_emu,
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
