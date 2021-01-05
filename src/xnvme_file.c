// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <libxnvme_file.h>

ssize_t
xnvme_file_pread(struct xnvme_cmd_ctx *ctx, void *buf, size_t count, off_t offset)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(ctx->dev);

	// TODO: check alignment of count and offset
	// - Use SSW instead of division

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
	ctx->cmd.common.nsid = xnvme_dev_get_nsid(ctx->dev);
	ctx->cmd.nvm.slba = offset / geo->nbytes;
	ctx->cmd.nvm.nlb = (count / geo->nbytes) - 1;

	return xnvme_cmd_pass(ctx, buf, count, NULL, 0);
}

ssize_t
xnvme_file_pwrite(struct xnvme_cmd_ctx *ctx, void *buf, size_t count, off_t offset)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(ctx->dev);

	// TODO: consider returning -EINVAL when mbuf is provided and namespace
	// have extended-lba in effect

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
	ctx->cmd.common.nsid = xnvme_dev_get_nsid(ctx->dev);
	ctx->cmd.nvm.slba = offset / geo->nbytes;
	ctx->cmd.nvm.nlb = (count / geo->nbytes) - 1;

	return xnvme_cmd_pass(ctx, buf, count, NULL, 0);
}

