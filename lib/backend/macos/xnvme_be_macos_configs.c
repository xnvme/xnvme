// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifdef XNVME_PLATFORM_MACOS_ENABLED
#include <xnvme_be.h>
#include <IOKit/storage/nvme/NVMeSMARTLibExternal.h>

#include <xnvme_be_posix.h>
#include <xnvme_be_macos.h>
#include <xnvme_be_emu.h>
#include <xnvme_be_thrpool.h>

const struct xnvme_be_config g_xnvme_be_macos_emu = {
	.async = &g_xnvme_be_emu_async,
	.sync = &g_xnvme_be_macos_sync,
	.admin = &g_xnvme_be_macos_admin,
	.dev = &g_xnvme_be_macos_dev,
	.mem = &g_xnvme_be_posix_mem,
	.attr =
		{
			.name = "emu",
			.descr = "Emulated async with macOS SMART",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_macos_thrpool = {
	.async = &g_xnvme_be_thrpool_async,
	.sync = &g_xnvme_be_macos_sync,
	.admin = &g_xnvme_be_macos_admin,
	.dev = &g_xnvme_be_macos_dev,
	.mem = &g_xnvme_be_posix_mem,
	.attr =
		{
			.name = "thrpool",
			.descr = "Thread pool with macOS SMART",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_macos_posix = {
	.async = &g_xnvme_be_posix_async,
	.sync = &g_xnvme_be_macos_sync,
	.admin = &g_xnvme_be_macos_admin,
	.dev = &g_xnvme_be_macos_dev,
	.mem = &g_xnvme_be_posix_mem,
	.attr =
		{
			.name = "posix",
			.descr = "POSIX aio with macOS SMART",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

#endif
