// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Niclas Hedam <n.hedam@samsung.com, nhed@itu.dk>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_HERMES_ENABLED
#include <xnvme_be_posix.h>
#include <xnvme_be_compute.h>

static struct xnvme_be_mixin g_xnvme_be_mixin_hermes[] = {
	{
		.mtype = XNVME_BE_MEM,
		.name = "posix",
		.descr = "Use libc malloc()/free() with sysconf for alignment",
		.mem = &g_xnvme_be_posix_mem,
		.check_support = xnvme_be_supported,
	},
	{
		.mtype = XNVME_BE_SYNC,
		.name = "hermes",
		.descr = "Send BPF programs to a Hermes-enabled device",
		.sync = &g_xnvme_be_compute_sync,
		.check_support = xnvme_be_supported,
	},
	{
		.mtype = XNVME_BE_DEV,
		.name = "hermes",
		.descr = "Use Hermes file/dev handles and no device enumeration",
		.dev = &g_xnvme_be_compute_dev,
		.check_support = xnvme_be_supported,
	},
};
#endif

struct xnvme_be xnvme_be_hermes = {
	.mem = XNVME_BE_NOSYS_MEM,
	.sync = XNVME_BE_NOSYS_SYNC,
	.dev = XNVME_BE_NOSYS_DEV,
	.attr = {
		.name = "hermes",
		.schemes = NULL,
		.nschemes = 0,
#ifdef XNVME_BE_HERMES_ENABLED
		.enabled = 1,
#endif
	},
#ifdef XNVME_BE_HERMES_ENABLED
	.nobjs = sizeof g_xnvme_be_mixin_hermes / sizeof * g_xnvme_be_mixin_hermes,
	.objs = g_xnvme_be_mixin_hermes,
#endif
};
