// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_be.h>
#include <xnvme_be_upcie_cuda.h>

struct xnvme_be_upcie_cuda_rte g_upcie_cuda_rte = {0};

const struct xnvme_be_config g_xnvme_be_upcie_cuda = {
	.async = &g_xnvme_be_upcie_cuda_async,
	.sync = &g_xnvme_be_upcie_cuda_sync,
	.admin = &g_xnvme_be_upcie_cuda_admin,
	.dev = &g_xnvme_be_upcie_cuda_dev,
	.mem = &g_xnvme_be_upcie_cuda_mem,
	.attr =
		{
			.name = "upcie-cuda",
			.descr = "CUDA-based uPCIe userspace NVMe driver",
			.caps = XNVME_BE_CAP_NVME_PCIE,
			.family = XNVME_BE_FAMILY_UPCIE,
		},
};

#endif
