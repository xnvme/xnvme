// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_be.h>
#include <xnvme_be_upcie.h>

struct xnvme_be_upcie_rte g_upcie_rte = {0};

const struct xnvme_be_config g_xnvme_be_upcie = {
	.async = &g_xnvme_be_upcie_async,
	.sync = &g_xnvme_be_upcie_sync,
	.admin = &g_xnvme_be_upcie_admin,
	.dev = &g_xnvme_be_upcie_dev,
	.mem = &g_xnvme_be_upcie_mem,
	.attr =
		{
			.name = "upcie",
			.descr = "uPCIe userspace NVMe driver",
			.caps = XNVME_BE_CAP_NVME_PCIE | XNVME_BE_CAP_CTRLR_SHARING,
		},
};

#endif

#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_be.h>
#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_cuda.h>

struct xnvme_be_upcie_cuda_rte g_upcie_cuda_rte = {0};

const struct xnvme_be_config g_xnvme_be_upcie_cuda = {
	.async = &g_xnvme_be_upcie_async_cuda,
	.sync = &g_xnvme_be_upcie_sync,
	.admin = &g_xnvme_be_upcie_admin,
	.dev = &g_xnvme_be_upcie_dev_cuda,
	.mem = &g_xnvme_be_upcie_mem_cuda,
	.attr =
		{
			.name = "upcie-cuda",
			.descr = "CUDA-based uPCIe userspace NVMe driver",
			.caps = XNVME_BE_CAP_NVME_PCIE | XNVME_BE_CAP_CTRLR_SHARING,
		},
};

#endif

#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <xnvme_be.h>
#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_hip.h>

struct xnvme_be_upcie_hip_rte g_upcie_hip_rte = {0};

const struct xnvme_be_config g_xnvme_be_upcie_hip = {
	.async = &g_xnvme_be_upcie_async_hip,
	.sync = &g_xnvme_be_upcie_sync,
	.admin = &g_xnvme_be_upcie_admin,
	.dev = &g_xnvme_be_upcie_dev_hip,
	.mem = &g_xnvme_be_upcie_mem_hip,
	.attr =
		{
			.name = "upcie-hip",
			.descr = "HIP-based uPCIe userspace NVMe driver",
			.caps = XNVME_BE_CAP_NVME_PCIE | XNVME_BE_CAP_CTRLR_SHARING,
		},
};

#endif
