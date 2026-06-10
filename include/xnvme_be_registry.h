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
#ifdef XNVME_BE_VFIO_ENABLED
extern const struct xnvme_be_config g_xnvme_be_vfio;
#endif

/* uPCIe */
#ifdef XNVME_BE_UPCIE_ENABLED
extern const struct xnvme_be_config g_xnvme_be_upcie;
#endif
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
extern const struct xnvme_be_config g_xnvme_be_upcie_cuda;
#endif

/* Linux */
#ifdef XNVME_PLATFORM_LINUX_ENABLED
extern const struct xnvme_be_config g_xnvme_be_linux_emu_nvme;
#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
extern const struct xnvme_be_config g_xnvme_be_linux_emu_block;
#endif
#ifdef XNVME_BE_LINUX_LIBURING_ENABLED
extern const struct xnvme_be_config g_xnvme_be_linux_ucmd_nvme;
extern const struct xnvme_be_config g_xnvme_be_linux_iou_nvme;
#endif
#if defined(XNVME_BE_LINUX_LIBURING_ENABLED) && defined(XNVME_BE_LINUX_BLOCK_ENABLED)
extern const struct xnvme_be_config g_xnvme_be_linux_iou_block;
#endif
#ifdef XNVME_BE_LINUX_LIBAIO_ENABLED
extern const struct xnvme_be_config g_xnvme_be_linux_aio_nvme;
#endif
#if defined(XNVME_BE_LINUX_LIBAIO_ENABLED) && defined(XNVME_BE_LINUX_BLOCK_ENABLED)
extern const struct xnvme_be_config g_xnvme_be_linux_aio_block;
#endif
#ifdef XNVME_BE_CBI_ASYNC_POSIX_ENABLED
extern const struct xnvme_be_config g_xnvme_be_linux_posix_nvme;
#endif
extern const struct xnvme_be_config g_xnvme_be_linux_thrpool_nvme;
#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
extern const struct xnvme_be_config g_xnvme_be_linux_thrpool_block;
#endif
extern const struct xnvme_be_config g_xnvme_be_linux_nil_nvme;
extern const struct xnvme_be_config g_xnvme_be_linux_emu_file;
extern const struct xnvme_be_config g_xnvme_be_linux_thrpool_file;
#ifdef XNVME_BE_LINUX_LIBURING_ENABLED
extern const struct xnvme_be_config g_xnvme_be_linux_iou_file;
#endif
#ifdef XNVME_BE_LINUX_LIBAIO_ENABLED
extern const struct xnvme_be_config g_xnvme_be_linux_aio_file;
#endif
#endif /* XNVME_PLATFORM_LINUX_ENABLED */

/* FreeBSD */
#ifdef XNVME_PLATFORM_FREEBSD_ENABLED
extern const struct xnvme_be_config g_xnvme_be_fbsd_kqueue_nvme;
extern const struct xnvme_be_config g_xnvme_be_fbsd_emu_file;
extern const struct xnvme_be_config g_xnvme_be_fbsd_kqueue_psync;
extern const struct xnvme_be_config g_xnvme_be_fbsd_posix_nvme;
extern const struct xnvme_be_config g_xnvme_be_fbsd_thrpool_nvme;
extern const struct xnvme_be_config g_xnvme_be_fbsd_emu_nvme;
extern const struct xnvme_be_config g_xnvme_be_fbsd_nil_nvme;
#endif

/* macOS */
#ifdef XNVME_PLATFORM_MACOS_ENABLED
extern const struct xnvme_be_config g_xnvme_be_macos_emu;
extern const struct xnvme_be_config g_xnvme_be_macos_thrpool;
extern const struct xnvme_be_config g_xnvme_be_macos_posix;
extern const struct xnvme_be_config g_xnvme_be_driverkit_native;
extern const struct xnvme_be_config g_xnvme_be_driverkit_emu;
#endif

/* Ramdisk */
#ifdef XNVME_BE_RAMDISK_ENABLED
extern const struct xnvme_be_config g_xnvme_be_ramdisk_nil;
extern const struct xnvme_be_config g_xnvme_be_ramdisk_thrpool;
extern const struct xnvme_be_config g_xnvme_be_ramdisk_emu;
#endif

/* Windows */
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
extern const struct xnvme_be_config g_xnvme_be_windows_emu_nvme;
extern const struct xnvme_be_config g_xnvme_be_windows_thrpool_nvme;
extern const struct xnvme_be_config g_xnvme_be_windows_iocp_nvme;
extern const struct xnvme_be_config g_xnvme_be_windows_iocp_th_nvme;
extern const struct xnvme_be_config g_xnvme_be_windows_ioring_nvme;
extern const struct xnvme_be_config g_xnvme_be_windows_nil_nvme;
#ifdef XNVME_BE_WINDOWS_FS_ENABLED
extern const struct xnvme_be_config g_xnvme_be_windows_iocp_fs;
extern const struct xnvme_be_config g_xnvme_be_windows_thrpool_fs;
#endif
#endif

#endif /* __INTERNAL_XNVME_BE_REGISTRY_H */
