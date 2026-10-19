// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#ifdef XNVME_PLATFORM_LINUX_ENABLED
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>
#include <xnvme_be_linux.h>

int
xnvme_be_linux_uapi_ver_fpr(FILE *stream, enum xnvme_pr opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "linux;LINUX_VERSION_CODE-UAPI/%d-%d.%d.%d", LINUX_VERSION_CODE,
			(LINUX_VERSION_CODE & (0xff << 16)) >> 16,
			(LINUX_VERSION_CODE & (0xff << 8)) >> 8, LINUX_VERSION_CODE & 0xff);

	return wrtn;
}

int
xnvme_file_opts_to_linux(struct xnvme_opts *opts)
{
	int flags = 0;

	flags |= opts->create ? O_CREAT : 0x0;
	flags |= opts->direct ? O_DIRECT : 0x0;
	flags |= opts->rdonly ? O_RDONLY : 0x0;
	flags |= opts->wronly ? O_WRONLY : 0x0;
	flags |= opts->rdwr ? O_RDWR : 0x0;
	flags |= opts->truncate ? O_TRUNC : 0x0;

	return flags;
}

const struct xnvme_be_config g_xnvme_be_linux_emu_nvme = {
	.async = &g_xnvme_be_cbi_async_emu,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "emu",
			.descr = "Emulated async with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV | XNVME_BE_CAP_NVME_BDEV,
		},
};

#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
const struct xnvme_be_config g_xnvme_be_linux_emu_block = {
	.async = &g_xnvme_be_cbi_async_emu,
	.sync = &g_xnvme_be_linux_sync_block,
	.admin = &g_xnvme_be_linux_admin_block,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "emu_bdev",
			.descr = "Emulated async with block layer",
			.caps = XNVME_BE_CAP_NVME_BDEV | XNVME_BE_CAP_BDEV,
		},
};
#endif

#ifdef XNVME_BE_LINUX_LIBURING_ENABLED
const struct xnvme_be_config g_xnvme_be_linux_ucmd_nvme = {
	.async = &g_xnvme_be_linux_async_ucmd,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "io_uring_cmd",
			.descr = "io_uring passthru with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV | XNVME_BE_CAP_NVME_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_linux_iou_nvme = {
	.async = &g_xnvme_be_linux_async_liburing,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "io_uring",
			.descr = "io_uring with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV | XNVME_BE_CAP_NVME_BDEV,
		},
};
#endif /* XNVME_BE_LINUX_LIBURING_ENABLED */

#if defined(XNVME_BE_LINUX_LIBURING_ENABLED) && defined(XNVME_BE_LINUX_BLOCK_ENABLED)
const struct xnvme_be_config g_xnvme_be_linux_iou_block = {
	.async = &g_xnvme_be_linux_async_liburing,
	.sync = &g_xnvme_be_linux_sync_block,
	.admin = &g_xnvme_be_linux_admin_block,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "io_uring_bdev",
			.descr = "io_uring with block layer",
			.caps = XNVME_BE_CAP_NVME_BDEV | XNVME_BE_CAP_BDEV,
		},
};
#endif

#ifdef XNVME_BE_LINUX_LIBAIO_ENABLED
const struct xnvme_be_config g_xnvme_be_linux_aio_nvme = {
	.async = &g_xnvme_be_linux_async_libaio,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "libaio",
			.descr = "libaio with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV | XNVME_BE_CAP_NVME_BDEV,
		},
};
#endif

#if defined(XNVME_BE_LINUX_LIBAIO_ENABLED) && defined(XNVME_BE_LINUX_BLOCK_ENABLED)
const struct xnvme_be_config g_xnvme_be_linux_aio_block = {
	.async = &g_xnvme_be_linux_async_libaio,
	.sync = &g_xnvme_be_linux_sync_block,
	.admin = &g_xnvme_be_linux_admin_block,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "libaio_bdev",
			.descr = "libaio with block layer",
			.caps = XNVME_BE_CAP_NVME_BDEV | XNVME_BE_CAP_BDEV,
		},
};
#endif

#ifdef XNVME_BE_CBI_ASYNC_POSIX_ENABLED
const struct xnvme_be_config g_xnvme_be_linux_posix_nvme = {
	.async = &g_xnvme_be_cbi_async_posix,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "posix",
			.descr = "POSIX aio with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV | XNVME_BE_CAP_NVME_BDEV,
		},
};
#endif

const struct xnvme_be_config g_xnvme_be_linux_thrpool_nvme = {
	.async = &g_xnvme_be_cbi_async_thrpool,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "thrpool",
			.descr = "Thread pool with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV | XNVME_BE_CAP_NVME_BDEV,
		},
};

#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
const struct xnvme_be_config g_xnvme_be_linux_thrpool_block = {
	.async = &g_xnvme_be_cbi_async_thrpool,
	.sync = &g_xnvme_be_linux_sync_block,
	.admin = &g_xnvme_be_linux_admin_block,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "thrpool_bdev",
			.descr = "Thread pool with block layer",
			.caps = XNVME_BE_CAP_NVME_BDEV | XNVME_BE_CAP_BDEV,
		},
};
#endif

const struct xnvme_be_config g_xnvme_be_linux_nil_nvme = {
	.async = &g_xnvme_be_cbi_async_nil,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "nil",
			.descr = "Nil async with NVMe ioctl",
			.caps = XNVME_BE_CAP_NVME_CDEV | XNVME_BE_CAP_NVME_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_linux_emu_file = {
	.async = &g_xnvme_be_cbi_async_emu,
	.sync = &g_xnvme_be_cbi_sync_psync,
	.admin = &g_xnvme_be_cbi_admin_shim,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "emu_file",
			.descr = "Emulated async with file I/O",
			.caps = XNVME_BE_CAP_FILE,
		},
};

const struct xnvme_be_config g_xnvme_be_linux_thrpool_file = {
	.async = &g_xnvme_be_cbi_async_thrpool,
	.sync = &g_xnvme_be_cbi_sync_psync,
	.admin = &g_xnvme_be_cbi_admin_shim,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "thrpool_file",
			.descr = "Thread pool with file I/O",
			.caps = XNVME_BE_CAP_FILE,
		},
};

#ifdef XNVME_BE_LINUX_LIBURING_ENABLED
const struct xnvme_be_config g_xnvme_be_linux_iou_file = {
	.async = &g_xnvme_be_linux_async_liburing,
	.sync = &g_xnvme_be_cbi_sync_psync,
	.admin = &g_xnvme_be_cbi_admin_shim,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "io_uring_file",
			.descr = "io_uring with file I/O",
			.caps = XNVME_BE_CAP_FILE,
		},
};
#endif

#ifdef XNVME_BE_LINUX_LIBAIO_ENABLED
const struct xnvme_be_config g_xnvme_be_linux_aio_file = {
	.async = &g_xnvme_be_linux_async_libaio,
	.sync = &g_xnvme_be_cbi_sync_psync,
	.admin = &g_xnvme_be_cbi_admin_shim,
	.dev = &g_xnvme_be_dev_linux,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "libaio_file",
			.descr = "libaio with file I/O",
			.caps = XNVME_BE_CAP_FILE,
		},
};
#endif

#else
int
xnvme_be_linux_uapi_ver_fpr(FILE *stream, enum xnvme_pr XNVME_UNUSED(opts))
{
	return fprintf(stream, "linux;LINUX_VERSION_CODE-UAPI/NOSYS\n");
}
#endif
