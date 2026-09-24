// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <xnvme_be.h>
#include <xnvme_be_nvmf.h>
#include <xnvme_be_cbi.h>

const struct xnvme_be_config g_xnvme_be_nvmf = {
	.sync = &g_xnvme_be_nvmf_sync,
	.admin = &g_xnvme_be_nvmf_admin,
	.dev = &g_xnvme_be_nvmf_dev,
	.async = &g_xnvme_be_cbi_async_nil,
	.mem = &g_xnvme_be_cbi_mem_posix,
	.attr =
		{
			.name = "nvmf",
			.descr = "xNVMe integrated NVMe-oF userspace driver",
			.caps = XNVME_BE_CAP_NVME_TCP | XNVME_BE_CAP_NVME_RDMA,
		},
};
