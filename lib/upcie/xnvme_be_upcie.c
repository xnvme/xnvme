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
			.caps = XNVME_BE_CAP_NVME_PCIE,
			.family = XNVME_BE_FAMILY_UPCIE,
		},
};

#endif
