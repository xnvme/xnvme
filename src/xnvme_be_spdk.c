// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_SPDK_ENABLED
#include <xnvme_be_spdk.h>

struct xnvme_be_mixin g_xnvme_be_mixin_spdk[] = {
	{
		.mtype = XNVME_BE_MEM,
		.name = "spdk",
		.descr = "Use SPDK DMA allocator",
		.mem = &g_xnvme_be_spdk_mem,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ASYNC,
		.name = "nvme",
		.descr = "Use the SPDK NVMe driver",
		.async = &g_xnvme_be_spdk_async,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_SYNC,
		.name = "nvme",
		.descr = "Use the SPDK NVMe driver",
		.sync = &g_xnvme_be_spdk_sync,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ADMIN,
		.name = "nvme",
		.descr = "Use the SPDK NVMe driver",
		.admin = &g_xnvme_be_spdk_admin,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_DEV,
		.name = "nvme",
		.descr = "Use the SPDK NVMe driver",
		.dev = &g_xnvme_be_spdk_dev,
		.check_support = xnvme_be_supported,
	},
};
static const int
g_xnvme_be_mixin_spdk_nobjs = sizeof g_xnvme_be_mixin_spdk / sizeof * g_xnvme_be_mixin_spdk;
#endif

static const char *g_schemes[] = {
	"pci",
	"pcie",
	"fab",
	"spdk"
};

struct xnvme_be xnvme_be_spdk = {
	.mem = XNVME_BE_NOSYS_MEM,
	.admin = XNVME_BE_NOSYS_ADMIN,
	.sync = XNVME_BE_NOSYS_SYNC,
	.async = XNVME_BE_NOSYS_QUEUE,
	.dev = XNVME_BE_NOSYS_DEV,
	.attr = {
		.name = "spdk",
		.schemes = g_schemes,
		.nschemes = sizeof g_schemes / sizeof(*g_schemes),
#ifdef XNVME_BE_SPDK_ENABLED
		.enabled = 1,
#endif
	},
#ifdef XNVME_BE_SPDK_ENABLED
	.nobjs = g_xnvme_be_mixin_spdk_nobjs,
	.objs = g_xnvme_be_mixin_spdk,
#endif
};
