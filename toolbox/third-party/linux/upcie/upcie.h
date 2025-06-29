// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * uPCIe header bundle
 * ===================
 *
 * This is the default umbrella header for uPCIe. It provides a convenient way to include all uPCIe
 * header-only libraries at once, simplifying integration for most use cases.
 *
 * Usage of this bundle is optional -- individual headers can be included selectively as needed.
 * This is particularly useful when:
 *
 * - You want full control over which libc or toolchain headers are included (e.g., avoiding
 *   assumptions about glibc or musl).
 * - You need to redefine system integration (e.g., MMIO access, memory handling).
 * - You want to avoid namespace clashes with other libraries or in-house code.
 *
 * To customize usage, simply replace this bundle with a header that includes only the components
 * required by your driver. This pattern is intentional: the bundle is a convenience, not a
 * requirement.
 *
 * Extension: NVMe
 * ---------------
 *
 * The NVMe extension provides data structures and helpers for building an NVMe driver using uPCIe.
 * It builds on uPCIe's libraries for bitfield manipulation, memory-mapped I/O (MMIO), and direct
 * memory access (DMA). To enable it, define the following before including this bundle:
 *
 *   #define _UPCIE_WITH_NVME
 *   #include <upcie/upcie.h>
 *
 * @file upcie.h
 * @version 0.3.2
 */
#ifndef UPCIE_H
#define UPCIE_H

#define _GNU_SOURCE

#ifdef __cplusplus
extern "C" {
#endif

// Toolchain and system headers
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/memfd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

// Linux UAPI
#include <linux/memfd.h>
#include <linux/vfio.h>

// uPCIe libraries
#include <upcie/debug.h>
#include <upcie/barriers.h>
#include <upcie/bitfield.h>
#include <upcie/hostmem.h>
#include <upcie/hostmem_config.h>
#include <upcie/hostmem_hugepage.h>
#include <upcie/hostmem_heap.h>
#include <upcie/hostmem_dma.h>
#include <upcie/mmio.h>
#include <upcie/pci.h>
#include <upcie/vfioctl.h>

// uPCIe NVMe libraries
#ifdef _UPCIE_WITH_NVME
#include <upcie/nvme/nvme_command.h>
#include <upcie/nvme/nvme_request.h>
#include <upcie/nvme/nvme_mmio.h>
#include <upcie/nvme/nvme_qid.h>
#include <upcie/nvme/nvme_qpair.h>
#include <upcie/nvme/nvme_controller.h>
#endif

#ifdef __cplusplus
}
#endif

#endif // UPCIE_H