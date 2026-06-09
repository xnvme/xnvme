// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifdef XNVME_PLATFORM_FREEBSD_ENABLED
#include <xnvme_be.h>
#include <xnvme_be_cbi.h>
#include <xnvme_be_fbsd.h>

const struct xnvme_be_config g_xnvme_be_fbsd_kqueue_nvme = {
	.async = &g_xnvme_be_fbsd_async,
	.sync = &g_xnvme_be_fbsd_sync_nvme,
	.admin = &g_xnvme_be_fbsd_admin_nvme,
	.dev = &g_xnvme_be_fbsd_dev,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.attr =
		{
			.name = "kqueue",
			.descr = "kqueue with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_fbsd_kqueue_psync = {
	.async = &g_xnvme_be_fbsd_async,
	.sync = &g_xnvme_be_cbi_sync_psync,
	.admin = &g_xnvme_be_cbi_admin_shim,
	.dev = &g_xnvme_be_fbsd_dev,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.attr =
		{
			.name = "kqueue_file",
			.descr = "kqueue with POSIX sync",
			.caps = XNVME_BE_CAP_FILE,
		},
};

const struct xnvme_be_config g_xnvme_be_fbsd_posix_nvme = {
	.async = &g_xnvme_be_cbi_async_posix,
	.sync = &g_xnvme_be_fbsd_sync_nvme,
	.admin = &g_xnvme_be_fbsd_admin_nvme,
	.dev = &g_xnvme_be_fbsd_dev,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.attr =
		{
			.name = "posix",
			.descr = "POSIX aio with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_fbsd_thrpool_nvme = {
	.async = &g_xnvme_be_cbi_async_thrpool,
	.sync = &g_xnvme_be_fbsd_sync_nvme,
	.admin = &g_xnvme_be_fbsd_admin_nvme,
	.dev = &g_xnvme_be_fbsd_dev,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.attr =
		{
			.name = "thrpool",
			.descr = "Thread pool with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_fbsd_emu_nvme = {
	.async = &g_xnvme_be_cbi_async_emu,
	.sync = &g_xnvme_be_fbsd_sync_nvme,
	.admin = &g_xnvme_be_fbsd_admin_nvme,
	.dev = &g_xnvme_be_fbsd_dev,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.attr =
		{
			.name = "emu",
			.descr = "Emulated async with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_fbsd_nil_nvme = {
	.async = &g_xnvme_be_cbi_async_nil,
	.sync = &g_xnvme_be_fbsd_sync_nvme,
	.admin = &g_xnvme_be_fbsd_admin_nvme,
	.dev = &g_xnvme_be_fbsd_dev,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.attr =
		{
			.name = "nil",
			.descr = "Nil async with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV,
		},
};

#endif
