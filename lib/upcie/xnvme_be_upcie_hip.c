// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Backend wiring for `upcie-hip`
 *
 * The admin, sync and async paths are the ones implemented for `upcie`; they
 * pick up this backend's device memory through xnvme_be_upcie_state.dmem,
 * assigned by dev_open. Only dev and mem are hip-specific.
 */
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <xnvme_be.h>
#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_hip.h>

struct xnvme_be_upcie_hip_rte g_upcie_hip_rte = {0};

const struct xnvme_be_config g_xnvme_be_upcie_hip = {
	.async = &g_xnvme_be_upcie_async,
	.sync = &g_xnvme_be_upcie_sync,
	.admin = &g_xnvme_be_upcie_admin,
	.dev = &g_xnvme_be_upcie_hip_dev,
	.mem = &g_xnvme_be_upcie_hip_mem,
	.attr =
		{
			.name = "upcie-hip",
			.descr = "HIP-based uPCIe userspace NVMe driver",
			.caps = XNVME_BE_CAP_NVME_PCIE | XNVME_BE_CAP_CTRLR_SHARING,
		},
};

#endif
