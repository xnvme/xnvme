// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifdef XNVME_BE_SPDK_ENABLED
#include <xnvme_be.h>
#include <xnvme_be_spdk.h>

const struct xnvme_be_config g_xnvme_be_spdk = {
	.async = &g_xnvme_be_spdk_async,
	.sync = &g_xnvme_be_spdk_sync,
	.admin = &g_xnvme_be_spdk_admin,
	.dev = &g_xnvme_be_spdk_dev,
	.mem = &g_xnvme_be_spdk_mem,
	.attr =
		{
			.name = "spdk",
			.descr = "SPDK userspace NVMe driver",
			.caps = XNVME_BE_CAP_NVME_PCIE | XNVME_BE_CAP_NVME_TCP |
				XNVME_BE_CAP_NVME_RDMA | XNVME_BE_CAP_MPROC,
		},
};

#endif
