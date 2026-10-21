// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifdef XNVME_BE_DRIVERKIT_ENABLED
#include <xnvme_be.h>
#include <xnvme_be_driverkit.h>

const struct xnvme_be_config g_xnvme_be_driverkit = {
	.async = &g_xnvme_be_driverkit_async,
	.sync = &g_xnvme_be_driverkit_sync,
	.admin = &g_xnvme_be_driverkit_admin,
	.dev = &g_xnvme_be_driverkit_dev,
	.mem = &g_xnvme_be_driverkit_mem,
	.attr =
		{
			.name = "driverkit",
			.descr = "DriverKit native async",
			.caps = XNVME_BE_CAP_NVME_PCIE,
		},
};

#endif
