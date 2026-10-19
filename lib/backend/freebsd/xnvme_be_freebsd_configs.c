// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifdef XNVME_PLATFORM_FREEBSD_ENABLED
#include <xnvme_be.h>
#include <xnvme_be_posix.h>
#include <xnvme_be_freebsd.h>
#include <xnvme_be_emu.h>
#include <xnvme_be_nil.h>
#include <xnvme_be_shim.h>
#include <xnvme_be_thrpool.h>

const struct xnvme_be_config g_xnvme_be_freebsd_kqueue = {
	.async = &g_xnvme_be_freebsd_async,
	.sync = &g_xnvme_be_freebsd_sync_nvme,
	.admin = &g_xnvme_be_freebsd_admin_nvme,
	.dev = &g_xnvme_be_freebsd_dev,
	.mem = &g_xnvme_be_posix_mem,
	.attr =
		{
			.name = "kqueue",
			.descr = "kqueue with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_freebsd_emu_file = {
	.async = &g_xnvme_be_emu_async,
	.sync = &g_xnvme_be_posix_sync,
	.admin = &g_xnvme_be_shim_admin,
	.dev = &g_xnvme_be_freebsd_dev,
	.mem = &g_xnvme_be_posix_mem,
	.attr =
		{
			.name = "emu_file",
			.descr = "Emulated async with file I/O",
			.caps = XNVME_BE_CAP_FILE,
		},
};

const struct xnvme_be_config g_xnvme_be_freebsd_kqueue_file = {
	.async = &g_xnvme_be_freebsd_async,
	.sync = &g_xnvme_be_posix_sync,
	.admin = &g_xnvme_be_shim_admin,
	.dev = &g_xnvme_be_freebsd_dev,
	.mem = &g_xnvme_be_posix_mem,
	.attr =
		{
			.name = "kqueue_file",
			.descr = "kqueue with POSIX sync",
			.caps = XNVME_BE_CAP_FILE,
		},
};

const struct xnvme_be_config g_xnvme_be_freebsd_posix = {
	.async = &g_xnvme_be_posix_async,
	.sync = &g_xnvme_be_freebsd_sync_nvme,
	.admin = &g_xnvme_be_freebsd_admin_nvme,
	.dev = &g_xnvme_be_freebsd_dev,
	.mem = &g_xnvme_be_posix_mem,
	.attr =
		{
			.name = "posix",
			.descr = "POSIX aio with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_freebsd_thrpool = {
	.async = &g_xnvme_be_thrpool_async,
	.sync = &g_xnvme_be_freebsd_sync_nvme,
	.admin = &g_xnvme_be_freebsd_admin_nvme,
	.dev = &g_xnvme_be_freebsd_dev,
	.mem = &g_xnvme_be_posix_mem,
	.attr =
		{
			.name = "thrpool",
			.descr = "Thread pool with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_freebsd_emu = {
	.async = &g_xnvme_be_emu_async,
	.sync = &g_xnvme_be_freebsd_sync_nvme,
	.admin = &g_xnvme_be_freebsd_admin_nvme,
	.dev = &g_xnvme_be_freebsd_dev,
	.mem = &g_xnvme_be_posix_mem,
	.attr =
		{
			.name = "emu",
			.descr = "Emulated async with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_freebsd_nil = {
	.async = &g_xnvme_be_nil_async,
	.sync = &g_xnvme_be_freebsd_sync_nvme,
	.admin = &g_xnvme_be_freebsd_admin_nvme,
	.dev = &g_xnvme_be_freebsd_dev,
	.mem = &g_xnvme_be_posix_mem,
	.attr =
		{
			.name = "nil",
			.descr = "Nil async with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV,
		},
};

#endif
