// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_POSIX_ENABLED
#include <errno.h>
#include <unistd.h>
#include <libxnvme_spec_fs.h>
#include <xnvme_dev.h>
#include <xnvme_be_posix.h>
#include <xnvme_be_psync.h>

int
xnvme_be_posix_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t mbuf_nbytes)
{
	struct xnvme_be_posix_state *state = (void *)ctx->dev->be.state;

	return _xnvme_be_psync_cmd_io(ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes, state->fd);
}
#endif

struct xnvme_be_sync g_xnvme_be_posix_sync_psync = {
	.id = "psync",
#ifdef XNVME_BE_POSIX_ENABLED
	.cmd_io = xnvme_be_posix_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
