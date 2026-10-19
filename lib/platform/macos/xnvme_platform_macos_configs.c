// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifdef XNVME_PLATFORM_MACOS_ENABLED
#include <xnvme_be.h>
#include <IOKit/storage/nvme/NVMeSMARTLibExternal.h>

#include <xnvme_be_posix.h>
#include <xnvme_be_macos.h>
#include <xnvme_be_emu.h>
#include <xnvme_be_nil.h>
#include <xnvme_be_thrpool.h>
#include <xnvme_be_ramdisk.h>
#include <xnvme_be_driverkit.h>

const struct xnvme_be_config g_xnvme_platform_macos_emu = {
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

const struct xnvme_be_config g_xnvme_platform_macos_thrpool = {
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

const struct xnvme_be_config g_xnvme_platform_macos_posix = {
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

#ifdef XNVME_BE_DRIVERKIT_ENABLED
const struct xnvme_be_config g_xnvme_platform_macos_driverkit_emu = {
	.async = &g_xnvme_be_emu_async,
	.sync = &g_xnvme_be_driverkit_sync,
	.admin = &g_xnvme_be_driverkit_admin,
	.dev = &g_xnvme_be_driverkit_dev,
	.mem = &g_xnvme_be_driverkit_mem,
	.attr =
		{
			.name = "driverkit_emu",
			.descr = "DriverKit with emulated async",
			.caps = XNVME_BE_CAP_NVME_PCIE,
		},
};
#endif

#ifdef XNVME_BE_RAMDISK_ENABLED
const struct xnvme_be_config g_xnvme_platform_macos_ramdisk = {
	.async = &g_xnvme_be_nil_async,
	.sync = &g_xnvme_be_ramdisk_sync,
	.admin = &g_xnvme_be_ramdisk_admin,
	.dev = &g_xnvme_be_ramdisk_dev,
	.mem = &g_xnvme_be_posix_mem,
	.attr =
		{
			.name = "ramdisk",
			.descr = "Ramdisk with nil async",
			.caps = XNVME_BE_CAP_RAMDISK,
		},
};

const struct xnvme_be_config g_xnvme_platform_macos_ramdisk_thrpool = {
	.async = &g_xnvme_be_thrpool_async,
	.sync = &g_xnvme_be_ramdisk_sync,
	.admin = &g_xnvme_be_ramdisk_admin,
	.dev = &g_xnvme_be_ramdisk_dev,
	.mem = &g_xnvme_be_posix_mem,
	.attr =
		{
			.name = "ramdisk_thrpool",
			.descr = "Ramdisk with thread pool",
			.caps = XNVME_BE_CAP_RAMDISK,
		},
};

const struct xnvme_be_config g_xnvme_platform_macos_ramdisk_emu = {
	.async = &g_xnvme_be_emu_async,
	.sync = &g_xnvme_be_ramdisk_sync,
	.admin = &g_xnvme_be_ramdisk_admin,
	.dev = &g_xnvme_be_ramdisk_dev,
	.mem = &g_xnvme_be_posix_mem,
	.attr =
		{
			.name = "ramdisk_emu",
			.descr = "Ramdisk with emulated async",
			.caps = XNVME_BE_CAP_RAMDISK,
		},
};
#endif
#endif
