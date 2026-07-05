// SPDX-License-Identifier: BSD-3-Clause

/**
 * HIP/ROCm uPCIe header bundle
 * ============================
 *
 * This includes the standard uPCIe bundle plus the HIP/ROCm headers for AMD GPU
 * device memory. It covers the regular CPU-initiated paths, including
 * CPU-initiated with GPU-direct P2P on AMD (host-driven NVMe queues, payload
 * DMA'd straight to/from GPU VRAM via hipmem).
 *
 * The GPU-initiated path (a HIP kernel drives the NVMe queues itself: the
 * doorbell bridge, its host poller, coherent queue placement, and the device
 * reap) is deliberately NOT pulled in here. It carries machinery (pthreads,
 * per-queue host relays) that the CPU-initiated paths do not need. Include
 * `upcie/upcie_hip_gpuinit.h` to opt into it.
 *
 * @file upcie_hip.h
 * @version 0.5.1
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

// HIP/ROCm uPCIe NVMe libraries
//
// Only the request helper is bundled here: it builds PRP lists against hipmem
// buffers for the CPU-initiated (host-driven queue) P2P path and pulls in no
// GPU-initiated machinery. The GPU-initiated controller/qpair live in
// upcie/upcie_hip_gpuinit.h.
#ifdef _UPCIE_WITH_NVME
#include <upcie/nvme/nvme_request_hip.h>
#endif

#ifdef __cplusplus
}
#endif

#endif // HIPUPCIE_H
