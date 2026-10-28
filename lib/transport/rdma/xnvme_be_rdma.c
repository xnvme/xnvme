// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <xnvme_be.h>
#include <xnvme_be_cbi.h>

#include <xnvme_be_rdma.h>

const struct xnvme_be_config g_xnvme_be_rdma = {
	.sync = &g_xnvme_be_rdma_sync,
	.admin = &g_xnvme_be_rdma_admin,
	.dev = &g_xnvme_be_rdma_dev,
	.async = &g_xnvme_be_rdma_async,
	.mem = &g_xnvme_be_rdma_mem,
	.attr =
		{
			.name = "rdma",
			.descr = "xNVMe integrated NVMe over RDMA userspace driver",
			.caps = XNVME_BE_CAP_NVME_RDMA,
		},
};
