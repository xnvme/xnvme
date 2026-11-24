// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#include <xnvme_be_rdma.h>

struct xnvme_be_dev g_xnvme_be_rdma_dev = {
	.id = "rdma",
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
	.ctrlr_init = NULL,
	.ctrlr_term = NULL,
};
