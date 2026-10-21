// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
#include <windows.h>
#include <xnvme_be_windows.h>
#include <xnvme_be_emu.h>
#include <xnvme_be_nil.h>
#include <xnvme_be_thrpool.h>
#include <xnvme_be_iocp.h>
#include <xnvme_be_ioring.h>
#include <xnvme_be_ramdisk.h>

const struct xnvme_be_config g_xnvme_platform_windows_emu = {
	.async = &g_xnvme_be_emu_async,
	.sync = &g_xnvme_be_windows_sync_nvme,
	.admin = &g_xnvme_be_windows_admin_nvme,
	.dev = &g_xnvme_be_windows_dev,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "emu",
			.descr = "Emulated async with NVMe ioctl",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_platform_windows_thrpool = {
	.async = &g_xnvme_be_thrpool_async,
	.sync = &g_xnvme_be_windows_sync_nvme,
	.admin = &g_xnvme_be_windows_admin_nvme,
	.dev = &g_xnvme_be_windows_dev,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "thrpool",
			.descr = "Thread pool with NVMe ioctl",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_platform_windows_iocp = {
	.async = &g_xnvme_be_iocp_async,
	.sync = &g_xnvme_be_windows_sync_nvme,
	.admin = &g_xnvme_be_windows_admin_nvme,
	.dev = &g_xnvme_be_windows_dev,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "iocp",
			.descr = "IOCP with NVMe ioctl",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_platform_windows_iocp_th = {
	.async = &g_xnvme_be_iocp_async_th,
	.sync = &g_xnvme_be_windows_sync_nvme,
	.admin = &g_xnvme_be_windows_admin_nvme,
	.dev = &g_xnvme_be_windows_dev,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "iocp_th",
			.descr = "IOCP threaded with NVMe ioctl",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_platform_windows_ioring = {
	.async = &g_xnvme_be_ioring_async,
	.sync = &g_xnvme_be_windows_sync_nvme,
	.admin = &g_xnvme_be_windows_admin_nvme,
	.dev = &g_xnvme_be_windows_dev,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "ioring",
			.descr = "IoRing with NVMe ioctl",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_platform_windows_nil = {
	.async = &g_xnvme_be_nil_async,
	.sync = &g_xnvme_be_windows_sync_nvme,
	.admin = &g_xnvme_be_windows_admin_nvme,
	.dev = &g_xnvme_be_windows_dev,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "nil",
			.descr = "Nil async with NVMe ioctl",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_platform_windows_iocp_file = {
	.async = &g_xnvme_be_iocp_async,
	.sync = &g_xnvme_be_windows_sync_fs,
	.admin = &g_xnvme_be_windows_admin_fs,
	.dev = &g_xnvme_be_windows_dev,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "iocp_file",
			.descr = "IOCP with file system APIs",
			.caps = XNVME_BE_CAP_FILE,
		},
};

const struct xnvme_be_config g_xnvme_platform_windows_thrpool_file = {
	.async = &g_xnvme_be_thrpool_async,
	.sync = &g_xnvme_be_windows_sync_fs,
	.admin = &g_xnvme_be_windows_admin_fs,
	.dev = &g_xnvme_be_windows_dev,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "thrpool_file",
			.descr = "Thread pool with file system APIs",
			.caps = XNVME_BE_CAP_FILE,
		},
};

#ifdef XNVME_BE_RAMDISK_ENABLED
const struct xnvme_be_config g_xnvme_platform_windows_ramdisk = {
	.async = &g_xnvme_be_nil_async,
	.sync = &g_xnvme_be_ramdisk_sync,
	.admin = &g_xnvme_be_ramdisk_admin,
	.dev = &g_xnvme_be_ramdisk_dev,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "ramdisk",
			.descr = "Ramdisk with nil async",
			.caps = XNVME_BE_CAP_RAMDISK,
		},
};

const struct xnvme_be_config g_xnvme_platform_windows_ramdisk_thrpool = {
	.async = &g_xnvme_be_thrpool_async,
	.sync = &g_xnvme_be_ramdisk_sync,
	.admin = &g_xnvme_be_ramdisk_admin,
	.dev = &g_xnvme_be_ramdisk_dev,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "ramdisk_thrpool",
			.descr = "Ramdisk with thread pool",
			.caps = XNVME_BE_CAP_RAMDISK,
		},
};

const struct xnvme_be_config g_xnvme_platform_windows_ramdisk_emu = {
	.async = &g_xnvme_be_emu_async,
	.sync = &g_xnvme_be_ramdisk_sync,
	.admin = &g_xnvme_be_ramdisk_admin,
	.dev = &g_xnvme_be_ramdisk_dev,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "ramdisk_emu",
			.descr = "Ramdisk with emulated async",
			.caps = XNVME_BE_CAP_RAMDISK,
		},
};
#endif
#endif
