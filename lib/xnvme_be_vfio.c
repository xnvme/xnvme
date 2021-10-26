// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
#include <xnvme_be_posix.h>
#include <xnvme_be_vfio.h>

struct xnvme_be_mixin g_xnvme_be_mixin_vfio[] = {
	{
		.mtype = XNVME_BE_MEM,
		.name = "vfio",
		.descr = "Use libc malloc()/free() with sysconf for alignment",
		.mem = &g_xnvme_be_vfio_mem,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ASYNC,
		.name = "vfio",
		.descr = "Use the VFIO NVMe driver",
		.async = &g_xnvme_be_vfio_async,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_SYNC,
		.name = "vfio",
		.descr = "Use the VFIO NVMe driver",
		.sync = &g_xnvme_be_vfio_sync,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ADMIN,
		.name = "vfio",
		.descr = "Use the VFIO NVMe driver",
		.admin = &g_xnvme_be_vfio_admin,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_DEV,
		.name = "vfio",
		.descr = "Use the VFIO NVMe driver",
		.dev = &g_xnvme_be_vfio_dev,
		.check_support = xnvme_be_supported,
	},
};
#endif

struct xnvme_be xnvme_be_vfio = {
	.mem = XNVME_BE_NOSYS_MEM,
	.admin = XNVME_BE_NOSYS_ADMIN,
	.sync = XNVME_BE_NOSYS_SYNC,
	.async = XNVME_BE_NOSYS_QUEUE,
	.dev = XNVME_BE_NOSYS_DEV,
	.attr =
		{
			.name = "vfio",
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
			.enabled = 1,
#endif
		},
	.state = {0},
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
	.nobjs = sizeof g_xnvme_be_mixin_vfio / sizeof *g_xnvme_be_mixin_vfio,
	.objs = g_xnvme_be_mixin_vfio,
#endif
};
