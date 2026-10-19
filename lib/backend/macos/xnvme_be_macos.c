// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifdef XNVME_PLATFORM_MACOS_ENABLED
#include <xnvme_be.h>
#include <IOKit/storage/nvme/NVMeSMARTLibExternal.h>

#include <xnvme_be_cbi.h>
#include <xnvme_be_macos.h>

const struct xnvme_be_config g_xnvme_be_macos_emu = {
	.async = &g_xnvme_be_cbi_async_emu,
	.sync = &g_xnvme_be_macos_sync_psync,
	.admin = &g_xnvme_be_macos_admin,
	.dev = &g_xnvme_be_macos_dev,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.attr =
		{
			.name = "emu",
			.descr = "Emulated async with macOS SMART",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_macos_thrpool = {
	.async = &g_xnvme_be_cbi_async_thrpool,
	.sync = &g_xnvme_be_macos_sync_psync,
	.admin = &g_xnvme_be_macos_admin,
	.dev = &g_xnvme_be_macos_dev,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.attr =
		{
			.name = "thrpool",
			.descr = "Thread pool with macOS SMART",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_macos_posix = {
	.async = &g_xnvme_be_cbi_async_posix,
	.sync = &g_xnvme_be_macos_sync_psync,
	.admin = &g_xnvme_be_macos_admin,
	.dev = &g_xnvme_be_macos_dev,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.attr =
		{
			.name = "posix",
			.descr = "POSIX aio with macOS SMART",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

#endif
