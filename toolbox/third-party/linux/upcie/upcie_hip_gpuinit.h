// SPDX-License-Identifier: BSD-3-Clause

/**
 * HIP/ROCm GPU-initiated NVMe header bundle
 * =========================================
 *
 * Opt-in bundle for GPU-initiated NVMe I/O on AMD: a HIP kernel builds SQEs,
 * rings the doorbell, and reaps completions from device code, with payload
 * DMA'd straight into GPU VRAM.
 *
 * This is a superset of `upcie/upcie_hip.h`. It adds the GPU-initiated queue
 * pair and controller, which bring machinery the CPU-initiated paths do not
 * need: the doorbell bridge and its host poller (the GPU cannot master MMIO to
 * the NVMe BAR on this class of device, so a CPU thread relays the doorbell),
 * fine-grained coherent host placement of the SQ/CQ, and a device-side reap.
 * Keeping it separate keeps the regular and CPU-initiated-P2P builds free of
 * that overhead.
 *
 * Requires `_UPCIE_WITH_NVME`. The queue pair and controller device functions
 * are meant to be compiled with hipcc.
 *
 * @file upcie_hip_gpuinit.h
 * @version 0.5.0
 */

#ifndef HIPUPCIE_GPUINIT_H
#define HIPUPCIE_GPUINIT_H

#include <upcie/upcie_hip.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _UPCIE_WITH_NVME
#include <upcie/nvme/nvme_qpair_hip.h>
#include <upcie/nvme/nvme_controller_hip.h>
#endif

#ifdef __cplusplus
}
#endif

#endif // HIPUPCIE_GPUINIT_H
