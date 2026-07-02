// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <xnvme_be.h>
#include <xnvme_be_upcie_hip.h>
#include <xnvme_be_upcie_hip_gpuinit.h>

/* Device discovery, buffer memory, admin commands, and the synchronous path are
 * identical to the CPU-initiated backend (same hipmem heap and NVMe controller);
 * only the async submission differs, so those components are reused as-is and the
 * GPU-initiated engine lives in g_xnvme_be_upcie_hip_gpuinit_async.
 */
const struct xnvme_be_config g_xnvme_be_upcie_hip_gpuinit = {
	.async = &g_xnvme_be_upcie_hip_gpuinit_async,
	.sync = &g_xnvme_be_upcie_hip_sync,
	.admin = &g_xnvme_be_upcie_hip_admin,
	.dev = &g_xnvme_be_upcie_hip_dev,
	.mem = &g_xnvme_be_upcie_hip_mem,
	.attr =
		{
			.name = "upcie-hip-gpuinit",
			.descr = "GPU-initiated HIP-based uPCIe userspace NVMe driver",
			.caps = XNVME_BE_CAP_NVME_PCIE,
		},
};

#endif
