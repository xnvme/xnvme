// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#include <xnvme_be_rdma.h>

struct xnvme_be_mem g_xnvme_be_rdma_mem = {
	.id = "rdma",
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.buf_register = xnvme_be_nosys_buf_register,
	.buf_unregister = xnvme_be_nosys_buf_unregister,
	.buf_get_alignment = NULL,
};
