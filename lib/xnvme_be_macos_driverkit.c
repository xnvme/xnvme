// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifdef XNVME_PLATFORM_MACOS_ENABLED
#include <xnvme_be.h>
#include <xnvme_be_cbi.h>
#include <xnvme_be_macos_driverkit.h>

const struct xnvme_be_config g_xnvme_be_driverkit_native = {
	.async = &g_xnvme_be_macos_driverkit_async,
	.sync = &g_xnvme_be_macos_driverkit_sync,
	.admin = &g_xnvme_be_macos_driverkit_admin,
	.dev = &g_xnvme_be_macos_driverkit_dev,
	.mem = &g_xnvme_be_macos_driverkit_mem,
	.attr =
		{
			.name = "driverkit",
			.descr = "DriverKit native async",
			.caps = XNVME_BE_CAP_NVME_PCIE,
		},
};

const struct xnvme_be_config g_xnvme_be_driverkit_emu = {
	.async = &g_xnvme_be_cbi_async_emu,
	.sync = &g_xnvme_be_macos_driverkit_sync,
	.admin = &g_xnvme_be_macos_driverkit_admin,
	.dev = &g_xnvme_be_macos_driverkit_dev,
	.mem = &g_xnvme_be_macos_driverkit_mem,
	.attr =
		{
			.name = "driverkit",
			.descr = "DriverKit with emulated async",
			.caps = XNVME_BE_CAP_NVME_PCIE,
		},
};

#endif
