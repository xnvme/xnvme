// SPDX-License-Identifier: BSD-3-Clause

/**
 * HIP/ROCm uPCIe header bundle
 * ============================
 *
 * This includes the standard uPCIe bundle, as well as the HIP/ROCm specific
 * headers for AMD GPU device memory. NVMe arrives with the standard bundle, so
 * a payload can be DMA'd straight to or from GPU VRAM on a host-driven queue.
 *
 * @file upcie_hip.h
 * @version 0.9.0
 */

#ifndef HIPUPCIE_H
#define HIPUPCIE_H

#define _GNU_SOURCE

#ifdef __cplusplus
extern "C" {
#endif

// Toolchain and system headers
#ifndef __HIP_PLATFORM_AMD__
#define __HIP_PLATFORM_AMD__
#endif
#include <hip/hip_runtime_api.h>

// uPCIe libraries
#include <upcie/upcie.h>

// HIP/ROCm uPCIe libraries
#include <upcie/hipmem_config.h>
#include <upcie/hipmem_heap.h>
#include <upcie/hipmem_dma.h>
#include <upcie/hipmem_mapping.h>
#include <upcie/dmamem_hip.h>

#ifdef __cplusplus
}
#endif

#endif // HIPUPCIE_H
