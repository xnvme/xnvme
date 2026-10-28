// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <errno.h>
#include <unistd.h>

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#include <xnvme_dev.h>

#include <xnvme_transport_debug.h>
#include <xnvme_transport_ctrlr.h>
#include <xnvme_transport_qpair.h>

#include <xnvme_be_rdma.h>

int
xnvme_be_rdma_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			  void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	return xnvme_transport_cmd_sync(TO_TRANSPORT_DEV_STATE(ctx->dev)->sync_qpair, ctx, dbuf,
					dbuf_nbytes);
}

int
xnvme_be_rdma_sync_cmd_iov(struct xnvme_cmd_ctx *XNVME_UNUSED(ctx),
			   struct iovec *XNVME_UNUSED(dvec), size_t XNVME_UNUSED(dvec_cnt),
			   size_t XNVME_UNUSED(dvec_nbytes), void *XNVME_UNUSED(mbuf),
			   size_t XNVME_UNUSED(mbuf_nbytes))
{
	return -ENOSYS;
}

struct xnvme_be_sync g_xnvme_be_rdma_sync = {
	.id = "rdma",
	.cmd_io = xnvme_be_rdma_sync_cmd_io,
	.cmd_iov = xnvme_be_rdma_sync_cmd_iov,
};
