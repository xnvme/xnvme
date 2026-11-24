// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#include <xnvme_be_rdma.h>

struct xnvme_be_admin g_xnvme_be_rdma_admin = {
	.id = "rdma",
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
};
