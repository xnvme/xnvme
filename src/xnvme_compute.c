// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Niclas Hedam <n.hedam@samsung.com, nhed@itu.dk>
// SPDX-License-Identifier: Apache-2.0
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <libxnvme_compute.h>

int
xnvme_compute_load(struct xnvme_cmd_ctx *ctx, void *buf, size_t buf_nbytes, uint32_t slot)
{
	ctx->cmd.compute.slot = slot;
	ctx->cmd.compute.opcode = XNVME_SPEC_COMPUTE_OPC_LOAD;

	return xnvme_cmd_pass(ctx, buf, buf_nbytes, 0x0, 0);
}

int
xnvme_compute_unload(struct xnvme_cmd_ctx *ctx, uint32_t slot)
{
	ctx->cmd.compute.slot = slot;
	ctx->cmd.compute.opcode = XNVME_SPEC_COMPUTE_OPC_UNLOAD;

	return xnvme_cmd_pass(ctx, NULL, 0, 0x0, 0);
}

int
xnvme_compute_exec(struct xnvme_cmd_ctx *ctx, uint32_t slot)
{
	ctx->cmd.compute.slot = slot;
	ctx->cmd.compute.opcode = XNVME_SPEC_COMPUTE_OPC_EXEC;

	return xnvme_cmd_pass(ctx, NULL, 0, 0x0, 0);
}

int
xnvme_compute_push(struct xnvme_cmd_ctx *ctx, void *buf, size_t buf_nbytes, uint32_t slot)
{
	ctx->cmd.compute.slot = slot;
	ctx->cmd.compute.opcode = XNVME_SPEC_COMPUTE_OPC_PUSH;

	return xnvme_cmd_pass(ctx, buf, buf_nbytes, 0x0, 0);
}

int
xnvme_compute_pull(struct xnvme_cmd_ctx *ctx, void *buf, size_t buf_nbytes, uint32_t slot)
{
	ctx->cmd.compute.slot = slot;
	ctx->cmd.compute.opcode = XNVME_SPEC_COMPUTE_OPC_PULL;

	return xnvme_cmd_pass(ctx, buf, buf_nbytes, 0x0, 0);
}
