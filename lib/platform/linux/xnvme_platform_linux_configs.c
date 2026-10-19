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
#include <xnvme_be_posix.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_emu.h>
#include <xnvme_be_nil.h>
#include <xnvme_be_shim.h>
#include <xnvme_be_thrpool.h>
#include <xnvme_be_libaio.h>
#include <xnvme_be_liburing.h>
#include <xnvme_be_ramdisk.h>

const struct xnvme_be_config g_xnvme_platform_linux_emu = {
	.async = &g_xnvme_be_emu_async,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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
const struct xnvme_be_config g_xnvme_platform_linux_emu_bdev = {
	.async = &g_xnvme_be_emu_async,
	.sync = &g_xnvme_be_linux_sync_block,
	.admin = &g_xnvme_be_linux_admin_block,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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

#ifdef XNVME_BE_LIBURING_ENABLED
const struct xnvme_be_config g_xnvme_platform_linux_io_uring_cmd = {
	.async = &g_xnvme_be_liburing_async_ucmd,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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

const struct xnvme_be_config g_xnvme_platform_linux_io_uring = {
	.async = &g_xnvme_be_liburing_async,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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
#endif /* XNVME_BE_LIBURING_ENABLED */

#if defined(XNVME_BE_LIBURING_ENABLED) && defined(XNVME_BE_LINUX_BLOCK_ENABLED)
const struct xnvme_be_config g_xnvme_platform_linux_io_uring_bdev = {
	.async = &g_xnvme_be_liburing_async,
	.sync = &g_xnvme_be_linux_sync_block,
	.admin = &g_xnvme_be_linux_admin_block,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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

#ifdef XNVME_BE_LIBAIO_ENABLED
const struct xnvme_be_config g_xnvme_platform_linux_libaio = {
	.async = &g_xnvme_be_libaio_async,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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

#if defined(XNVME_BE_LIBAIO_ENABLED) && defined(XNVME_BE_LINUX_BLOCK_ENABLED)
const struct xnvme_be_config g_xnvme_platform_linux_libaio_bdev = {
	.async = &g_xnvme_be_libaio_async,
	.sync = &g_xnvme_be_linux_sync_block,
	.admin = &g_xnvme_be_linux_admin_block,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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

#ifdef XNVME_BE_POSIX_ENABLED
const struct xnvme_be_config g_xnvme_platform_linux_posix = {
	.async = &g_xnvme_be_posix_async,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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

const struct xnvme_be_config g_xnvme_platform_linux_thrpool = {
	.async = &g_xnvme_be_thrpool_async,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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
const struct xnvme_be_config g_xnvme_platform_linux_thrpool_bdev = {
	.async = &g_xnvme_be_thrpool_async,
	.sync = &g_xnvme_be_linux_sync_block,
	.admin = &g_xnvme_be_linux_admin_block,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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

const struct xnvme_be_config g_xnvme_platform_linux_nil = {
	.async = &g_xnvme_be_nil_async,
	.sync = &g_xnvme_be_linux_sync_nvme,
	.admin = &g_xnvme_be_linux_admin_nvme,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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

const struct xnvme_be_config g_xnvme_platform_linux_emu_file = {
	.async = &g_xnvme_be_emu_async,
	.sync = &g_xnvme_be_posix_sync,
	.admin = &g_xnvme_be_shim_admin,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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

const struct xnvme_be_config g_xnvme_platform_linux_thrpool_file = {
	.async = &g_xnvme_be_thrpool_async,
	.sync = &g_xnvme_be_posix_sync,
	.admin = &g_xnvme_be_shim_admin,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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

#ifdef XNVME_BE_LIBURING_ENABLED
const struct xnvme_be_config g_xnvme_platform_linux_io_uring_file = {
	.async = &g_xnvme_be_liburing_async,
	.sync = &g_xnvme_be_posix_sync,
	.admin = &g_xnvme_be_shim_admin,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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

#ifdef XNVME_BE_LIBAIO_ENABLED
const struct xnvme_be_config g_xnvme_platform_linux_libaio_file = {
	.async = &g_xnvme_be_libaio_async,
	.sync = &g_xnvme_be_posix_sync,
	.admin = &g_xnvme_be_shim_admin,
	.dev = &g_xnvme_be_linux_dev,
	.mem = &g_xnvme_be_posix_mem,
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

#ifdef XNVME_BE_RAMDISK_ENABLED
const struct xnvme_be_config g_xnvme_platform_linux_ramdisk = {
	.async = &g_xnvme_be_nil_async,
	.sync = &g_xnvme_be_ramdisk_sync,
	.admin = &g_xnvme_be_ramdisk_admin,
	.dev = &g_xnvme_be_ramdisk_dev,
	.mem = &g_xnvme_be_posix_mem,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "ramdisk",
			.descr = "Ramdisk with nil async",
			.caps = XNVME_BE_CAP_RAMDISK,
		},
};

const struct xnvme_be_config g_xnvme_platform_linux_ramdisk_thrpool = {
	.async = &g_xnvme_be_thrpool_async,
	.sync = &g_xnvme_be_ramdisk_sync,
	.admin = &g_xnvme_be_ramdisk_admin,
	.dev = &g_xnvme_be_ramdisk_dev,
	.mem = &g_xnvme_be_posix_mem,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "ramdisk_thrpool",
			.descr = "Ramdisk with thread pool",
			.caps = XNVME_BE_CAP_RAMDISK,
		},
};

const struct xnvme_be_config g_xnvme_platform_linux_ramdisk_emu = {
	.async = &g_xnvme_be_emu_async,
	.sync = &g_xnvme_be_ramdisk_sync,
	.admin = &g_xnvme_be_ramdisk_admin,
	.dev = &g_xnvme_be_ramdisk_dev,
	.mem = &g_xnvme_be_posix_mem,
	.mem_overrides =
		(const struct xnvme_be_mem *const[]){
			&g_xnvme_be_linux_mem_hugepage,
			NULL,
		},
	.attr =
		{
			.name = "ramdisk_emu",
			.descr = "Ramdisk with emulated async",
			.caps = XNVME_BE_CAP_RAMDISK,
		},
};
#endif
#endif
