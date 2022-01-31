// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <libxnvme_file.h>
#include <libxnvme_spec.h>
#include <libxnvme_spec_fs.h>
#include <xnvme_be_linux.h>

int
xnvme_file_pread(struct xnvme_cmd_ctx *ctx, void *buf, size_t count, off_t offset)
{
	ctx->cmd.common.opcode = XNVME_SPEC_FS_OPC_READ;
	ctx->cmd.common.nsid = xnvme_dev_get_nsid(ctx->dev);
	ctx->cmd.nvm.slba = offset;

	return xnvme_cmd_pass(ctx, buf, count, NULL, 0);
}

int
xnvme_file_pwrite(struct xnvme_cmd_ctx *ctx, void *buf, size_t count, off_t offset)
{
	ctx->cmd.common.opcode = XNVME_SPEC_FS_OPC_WRITE;
	ctx->cmd.common.nsid = xnvme_dev_get_nsid(ctx->dev);
	ctx->cmd.nvm.slba = offset;

	return xnvme_cmd_pass(ctx, buf, count, NULL, 0);
}

int
xnvme_file_close(struct xnvme_dev *fh)
{
	// TODO: the return value of `close` is not passed all the way back up the
	// stack
	xnvme_dev_close(fh);
	return 0;
}

int
xnvme_file_sync(struct xnvme_dev *fh)
{
	struct xnvme_cmd_ctx ctx = xnvme_file_get_cmd_ctx(fh);

	ctx.cmd.common.opcode = XNVME_SPEC_FS_OPC_FLUSH;
	ctx.cmd.common.nsid = xnvme_dev_get_nsid(ctx.dev);

	return xnvme_cmd_pass(&ctx, NULL, 0, NULL, 0);
}

struct xnvme_cmd_ctx
xnvme_file_get_cmd_ctx(struct xnvme_dev *fh)
{
	return xnvme_cmd_ctx_from_dev(fh);
}

struct xnvme_dev *
xnvme_file_open(const char *pathname, struct xnvme_opts *opts)
{
	return xnvme_dev_open(pathname, opts);
}
