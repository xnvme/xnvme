// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifdef XNVME_BE_RAMDISK_ENABLED
#include <xnvme_be.h>
#include <xnvme_be_ramdisk.h>
#include <xnvme_be_cbi.h>

#if defined(XNVME_BE_CBI_MEM_POSIX_ENABLED)
#define XNVME_BE_RAMDISK_MEM &g_xnvme_be_cbi_mem_posix
#elif defined(XNVME_PLATFORM_WINDOWS_ENABLED)
#include <xnvme_be_windows.h>
#define XNVME_BE_RAMDISK_MEM &g_xnvme_be_windows_mem
#else
#error "ramdisk needs a mem implementation"
#endif

#if defined(XNVME_PLATFORM_LINUX_ENABLED)
#include <xnvme_be_linux.h>
#define XNVME_BE_RAMDISK_MEM_OVERRIDES                         \
	.mem_overrides = (const struct xnvme_be_mem *const[]){ \
		&g_xnvme_be_linux_mem_hugepage,                \
		NULL,                                          \
	},
#else
#define XNVME_BE_RAMDISK_MEM_OVERRIDES
#endif

const struct xnvme_be_config g_xnvme_be_ramdisk_nil = {
	.async = &g_xnvme_be_cbi_async_nil,
	.sync = &g_xnvme_be_ramdisk_sync,
	.admin = &g_xnvme_be_ramdisk_admin,
	.dev = &g_xnvme_be_ramdisk_dev,
	.mem = XNVME_BE_RAMDISK_MEM,
	XNVME_BE_RAMDISK_MEM_OVERRIDES.attr =
		{
			.name = "ramdisk",
			.descr = "Ramdisk with nil async",
			.caps = XNVME_BE_CAP_RAMDISK,
		},
};

const struct xnvme_be_config g_xnvme_be_ramdisk_thrpool = {
	.async = &g_xnvme_be_cbi_async_thrpool,
	.sync = &g_xnvme_be_ramdisk_sync,
	.admin = &g_xnvme_be_ramdisk_admin,
	.dev = &g_xnvme_be_ramdisk_dev,
	.mem = XNVME_BE_RAMDISK_MEM,
	XNVME_BE_RAMDISK_MEM_OVERRIDES.attr =
		{
			.name = "ramdisk_thrpool",
			.descr = "Ramdisk with thread pool",
			.caps = XNVME_BE_CAP_RAMDISK,
		},
};

const struct xnvme_be_config g_xnvme_be_ramdisk_emu = {
	.async = &g_xnvme_be_cbi_async_emu,
	.sync = &g_xnvme_be_ramdisk_sync,
	.admin = &g_xnvme_be_ramdisk_admin,
	.dev = &g_xnvme_be_ramdisk_dev,
	.mem = XNVME_BE_RAMDISK_MEM,
	XNVME_BE_RAMDISK_MEM_OVERRIDES.attr =
		{
			.name = "ramdisk_emu",
			.descr = "Ramdisk with emulated async",
			.caps = XNVME_BE_CAP_RAMDISK,
		},
};

#endif
