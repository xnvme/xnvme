// SPDX-License-Identifier: BSD-3-Clause

/**
 * HIP/ROCm uPCIe header bundle
 * ============================
 *
 * This includes the standard uPCIe bundle, as well as the HIP/ROCm specific
 * headers for AMD GPU device memory.
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

// CUDA uPCIe NVMe libraries
#ifdef _UPCIE_WITH_NVME
#include <upcie/nvme/nvme_request_hip.h>
#include <upcie/nvme/nvme_qpair_hip.h>
#include <upcie/nvme/nvme_controller_hip.h>
#endif

#ifdef __cplusplus
}
#endif

#endif // CUPCIE_H
