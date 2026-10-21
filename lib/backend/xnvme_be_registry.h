// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_REGISTRY_H
#define __INTERNAL_XNVME_BE_REGISTRY_H

#include <xnvme_be.h>

/* SPDK */
#ifdef XNVME_BE_SPDK_ENABLED
extern const struct xnvme_be_config g_xnvme_be_spdk;
#endif

/* libvfn/VFIO */
#ifdef XNVME_BE_LIBVFN_ENABLED
extern const struct xnvme_be_config g_xnvme_be_libvfn;
#endif

/* uPCIe */
#ifdef XNVME_BE_UPCIE_ENABLED
extern const struct xnvme_be_config g_xnvme_be_upcie;
#endif
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
extern const struct xnvme_be_config g_xnvme_be_upcie_cuda;
#endif
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
extern const struct xnvme_be_config g_xnvme_be_upcie_hip;
#endif

/* Linux */
#ifdef XNVME_PLATFORM_LINUX_ENABLED
extern const struct xnvme_be_config g_xnvme_platform_linux_emu;
#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
extern const struct xnvme_be_config g_xnvme_platform_linux_emu_bdev;
#endif
#ifdef XNVME_BE_LIBURING_ENABLED
extern const struct xnvme_be_config g_xnvme_platform_linux_io_uring_cmd;
extern const struct xnvme_be_config g_xnvme_platform_linux_io_uring;
#endif
#if defined(XNVME_BE_LIBURING_ENABLED) && defined(XNVME_BE_LINUX_BLOCK_ENABLED)
extern const struct xnvme_be_config g_xnvme_platform_linux_io_uring_bdev;
#endif
#ifdef XNVME_BE_LIBAIO_ENABLED
extern const struct xnvme_be_config g_xnvme_platform_linux_libaio;
#endif
#if defined(XNVME_BE_LIBAIO_ENABLED) && defined(XNVME_BE_LINUX_BLOCK_ENABLED)
extern const struct xnvme_be_config g_xnvme_platform_linux_libaio_bdev;
#endif
#ifdef XNVME_BE_POSIX_ENABLED
extern const struct xnvme_be_config g_xnvme_platform_linux_posix;
#endif
extern const struct xnvme_be_config g_xnvme_platform_linux_thrpool;
#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
extern const struct xnvme_be_config g_xnvme_platform_linux_thrpool_bdev;
#endif
extern const struct xnvme_be_config g_xnvme_platform_linux_nil;
extern const struct xnvme_be_config g_xnvme_platform_linux_emu_file;
extern const struct xnvme_be_config g_xnvme_platform_linux_thrpool_file;
#ifdef XNVME_BE_LIBURING_ENABLED
extern const struct xnvme_be_config g_xnvme_platform_linux_io_uring_file;
#endif
#ifdef XNVME_BE_LIBAIO_ENABLED
extern const struct xnvme_be_config g_xnvme_platform_linux_libaio_file;
#endif
#endif /* XNVME_PLATFORM_LINUX_ENABLED */

/* FreeBSD */
#ifdef XNVME_PLATFORM_FREEBSD_ENABLED
extern const struct xnvme_be_config g_xnvme_platform_freebsd_kqueue;
extern const struct xnvme_be_config g_xnvme_platform_freebsd_emu_file;
extern const struct xnvme_be_config g_xnvme_platform_freebsd_kqueue_file;
extern const struct xnvme_be_config g_xnvme_platform_freebsd_posix;
extern const struct xnvme_be_config g_xnvme_platform_freebsd_thrpool;
extern const struct xnvme_be_config g_xnvme_platform_freebsd_emu;
extern const struct xnvme_be_config g_xnvme_platform_freebsd_nil;
#endif

/* macOS */
#ifdef XNVME_PLATFORM_MACOS_ENABLED
extern const struct xnvme_be_config g_xnvme_platform_macos_emu;
extern const struct xnvme_be_config g_xnvme_platform_macos_thrpool;
extern const struct xnvme_be_config g_xnvme_platform_macos_posix;
extern const struct xnvme_be_config g_xnvme_be_driverkit;
#ifdef XNVME_BE_DRIVERKIT_ENABLED
extern const struct xnvme_be_config g_xnvme_platform_macos_driverkit_emu;
#endif
#endif

/* Ramdisk */
#ifdef XNVME_BE_RAMDISK_ENABLED
extern const struct xnvme_be_config g_xnvme_platform_linux_ramdisk;
extern const struct xnvme_be_config g_xnvme_platform_freebsd_ramdisk;
extern const struct xnvme_be_config g_xnvme_platform_macos_ramdisk;
extern const struct xnvme_be_config g_xnvme_platform_windows_ramdisk;
extern const struct xnvme_be_config g_xnvme_platform_linux_ramdisk_thrpool;
extern const struct xnvme_be_config g_xnvme_platform_freebsd_ramdisk_thrpool;
extern const struct xnvme_be_config g_xnvme_platform_macos_ramdisk_thrpool;
extern const struct xnvme_be_config g_xnvme_platform_windows_ramdisk_thrpool;
extern const struct xnvme_be_config g_xnvme_platform_linux_ramdisk_emu;
extern const struct xnvme_be_config g_xnvme_platform_freebsd_ramdisk_emu;
extern const struct xnvme_be_config g_xnvme_platform_macos_ramdisk_emu;
extern const struct xnvme_be_config g_xnvme_platform_windows_ramdisk_emu;
#endif

/* Windows */
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
extern const struct xnvme_be_config g_xnvme_platform_windows_emu;
extern const struct xnvme_be_config g_xnvme_platform_windows_thrpool;
extern const struct xnvme_be_config g_xnvme_platform_windows_iocp;
extern const struct xnvme_be_config g_xnvme_platform_windows_iocp_th;
extern const struct xnvme_be_config g_xnvme_platform_windows_ioring;
extern const struct xnvme_be_config g_xnvme_platform_windows_nil;
extern const struct xnvme_be_config g_xnvme_platform_windows_iocp_file;
extern const struct xnvme_be_config g_xnvme_platform_windows_thrpool_file;
#endif

#endif /* __INTERNAL_XNVME_BE_REGISTRY_H */
