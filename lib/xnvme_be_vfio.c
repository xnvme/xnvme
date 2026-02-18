// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_VFIO_ENABLED
#include <xnvme_be_vfio.h>

struct xnvme_be_mixin g_xnvme_be_mixin_vfio[] = {
	{
		.mtype = XNVME_BE_MEM,
		.name = "libvfn",
		.descr = "Use libc malloc()/free() with sysconf for alignment",
		.mem = &g_xnvme_be_vfio_mem,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ASYNC,
		.name = "libvfn",
		.descr = "Use the libvfn NVMe driver",
		.async = &g_xnvme_be_vfio_async,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_SYNC,
		.name = "libvfn",
		.descr = "Use the libvfn NVMe driver",
		.sync = &g_xnvme_be_vfio_sync,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ADMIN,
		.name = "libvfn",
		.descr = "Use the libvfn NVMe driver",
		.admin = &g_xnvme_be_vfio_admin,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_DEV,
		.name = "libvfn",
		.descr = "Use the libvfn NVMe driver",
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
			.name = "libvfn",
#ifdef XNVME_BE_VFIO_ENABLED
			.enabled = 1,
#endif
		},
	.state = {0},
#ifdef XNVME_BE_VFIO_ENABLED
	.nobjs = sizeof g_xnvme_be_mixin_vfio / sizeof *g_xnvme_be_mixin_vfio,
	.objs = g_xnvme_be_mixin_vfio,
#endif
};
