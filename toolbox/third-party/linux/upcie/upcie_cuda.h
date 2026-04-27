// SPDX-License-Identifier: BSD-3-Clause

/**
 * CUDA uPCIe header bundle
 * ========================
 *
 * This includes the standard uPCIe bundle, as well as the cuda specific headers.
 */

#ifndef CUPCIE_H
#define CUPCIE_H

#define _GNU_SOURCE

#ifdef __cplusplus
extern "C" {
#endif

// Toolchain and system headers
#include <cuda.h>

// uPCIe libraries
#include <upcie/upcie.h>

// CUDA uPCIe libraries
#include <upcie/cudamem_config.h>
#include <upcie/cudamem_heap.h>
#include <upcie/cudamem_dma.h>
#include <upcie/cudamem_mapping.h>

// CUDA uPCIe NVMe libraries
#ifdef _UPCIE_WITH_NVME
#include <upcie/nvme/nvme_request_cuda.h>
#include <upcie/nvme/nvme_qpair_cuda.h>
#include <upcie/nvme/nvme_controller_cuda.h>
#endif

#ifdef __cplusplus
}
#endif

#endif // CUPCIE_H
