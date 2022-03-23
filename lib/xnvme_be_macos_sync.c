// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_MACOS_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_macos.h>
#include <xnvme_be_psync.h>

int
xnvme_be_macos_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t mbuf_nbytes)
{
	struct xnvme_be_macos_state *state = (void *)ctx->dev->be.state;

	return _xnvme_be_psync_cmd_io(ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes, state->fd);
}

int
xnvme_be_macos_sync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			    size_t dvec_nbytes, struct iovec *mvec, size_t mvec_cnt,
			    size_t mvec_nbytes)
{
	struct xnvme_be_macos_state *state = (void *)ctx->dev->be.state;

	return _xnvme_be_psync_cmd_iov(ctx, dvec, dvec_cnt, dvec_nbytes, mvec, mvec_cnt,
				       mvec_nbytes, state->fd);
}
#endif

struct xnvme_be_sync g_xnvme_be_macos_sync_psync = {
	.id = "psync",
#ifdef XNVME_BE_MACOS_ENABLED
	.cmd_io = xnvme_be_macos_sync_cmd_io,
	.cmd_iov = xnvme_be_macos_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
