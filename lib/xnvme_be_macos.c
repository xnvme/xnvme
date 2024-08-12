// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_MACOS_ENABLED
#include <IOKit/storage/nvme/NVMeSMARTLibExternal.h>

#include <xnvme_be_cbi.h>
#include <xnvme_be_macos.h>

static struct xnvme_be_mixin g_xnvme_be_mixin_macos[] = {
	{
		.mtype = XNVME_BE_MEM,
		.name = "posix",
		.descr = "Use C11 lib malloc/free with sysconf for alignment",
		.mem = &g_xnvme_be_cbi_mem_posix,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ASYNC,
		.name = "emu",
		.descr = "Use emulated asynchronous I/O",
		.async = &g_xnvme_be_cbi_async_emu,
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
		.name = "posix",
		.descr = "Use POSIX aio for Asynchronous I/O",
		.async = &g_xnvme_be_cbi_async_posix,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_SYNC,
		.name = "macos",
		.descr = "Use pread()/write() for synchronous I/O",
		.sync = &g_xnvme_be_macos_sync_psync,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ADMIN,
		.name = "macos",
		.descr = "Use macOS NVMe SMART Interface admin commands",
		.admin = &g_xnvme_be_macos_admin,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_DEV,
		.name = "macos",
		.descr = "Use macOS file/dev handles and enumerate NVMe devices",
		.dev = &g_xnvme_be_macos_dev,
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
