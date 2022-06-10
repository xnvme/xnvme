// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_cmd.h>
#include <xnvme_dev.h>
#include <xnvme_queue.h>

void
xnvme_cmd_ctx_pr(const struct xnvme_cmd_ctx *ctx, int XNVME_UNUSED(opts))
{
	printf("xnvme_cmd_ctx: ");

	if (!ctx) {
		printf("~\n");
		return;
	}

	printf("{cdw0: 0x%x, sc: 0x%x, sct: 0x%x}\n", ctx->cpl.cdw0, ctx->cpl.status.sc,
	       ctx->cpl.status.sct);
}

void
xnvme_cmd_ctx_clear(struct xnvme_cmd_ctx *ctx)
{
	memset(ctx, 0x0, sizeof(*ctx));
}

struct xnvme_cmd_ctx
xnvme_cmd_ctx_from_dev(struct xnvme_dev *dev)
{
	struct xnvme_cmd_ctx ctx = {.dev = dev, .opts = XNVME_CMD_SYNC};

	return ctx;
}

struct xnvme_cmd_ctx *
xnvme_cmd_ctx_from_queue(struct xnvme_queue *queue)
{
	return xnvme_queue_get_cmd_ctx(queue);
}

int
xnvme_cmd_pass(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
	       size_t mbuf_nbytes)
{
	const int cmd_opts = ctx->opts & XNVME_CMD_MASK;

	switch (cmd_opts & XNVME_CMD_MASK_IOMD) {
	case XNVME_CMD_ASYNC:
		if (ctx->async.queue->base.outstanding == ctx->async.queue->base.capacity) {
			XNVME_DEBUG("FAILED: queue is full; returning -EBUSY");
			return -EBUSY;
		}
		return ctx->dev->be.async.cmd_io(ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes);

	case XNVME_CMD_SYNC:
		return ctx->dev->be.sync.cmd_io(ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes);

	default:
		XNVME_DEBUG("FAILED: command-mode not provided");
		return -EINVAL;
	}
}

int
xnvme_cmd_passv(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt, size_t dvec_nbytes,
		struct iovec *mvec, size_t mvec_cnt, size_t mvec_nbytes)
{
	const int cmd_opts = ctx->opts & XNVME_CMD_MASK;

	switch (cmd_opts & XNVME_CMD_MASK_IOMD) {
	case XNVME_CMD_ASYNC:
		if (ctx->async.queue->base.outstanding == ctx->async.queue->base.capacity) {
			XNVME_DEBUG("FAILED: queue is full; returning -EBUSY");
			return -EBUSY;
		}
		return ctx->dev->be.async.cmd_iov(ctx, dvec, dvec_cnt, dvec_nbytes, mvec, mvec_cnt,
						  mvec_nbytes);
	case XNVME_CMD_SYNC:
		return ctx->dev->be.sync.cmd_iov(ctx, dvec, dvec_cnt, dvec_nbytes, mvec, mvec_cnt,
						 mvec_nbytes);
	default:
		XNVME_DEBUG("FAILED: command-mode not provided");
		return -EINVAL;
	}
}

int
xnvme_cmd_pass_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
		     size_t mbuf_nbytes)
{
	if (ctx->opts & XNVME_CMD_ASYNC) {
		XNVME_DEBUG("FAILED: Admin commands are always sync.");
		return -EINVAL;
	}

	return ctx->dev->be.admin.cmd_admin(ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes);
}
