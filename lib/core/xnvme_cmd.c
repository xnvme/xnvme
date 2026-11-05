// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

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

	printf("{cdw0: 0x%" PRIx32 ", sc: 0x%" PRIx16 ", sct: 0x%" PRIx16 "}\n", ctx->cpl.cdw0,
	       ctx->cpl.status.sc, ctx->cpl.status.sct);
}

void
xnvme_cmd_ctx_clear(struct xnvme_cmd_ctx *ctx)
{
	memset(ctx, 0x0, sizeof(*ctx));
}

int
xnvme_dev_get_cmd_ctx(struct xnvme_dev *dev, struct xnvme_cmd_ctx *ctx)
{
	if (!dev || !ctx) {
		return -EINVAL;
	}
	*ctx = (struct xnvme_cmd_ctx){.dev = dev, .opts = XNVME_CMD_SYNC};

	return 0;
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
xnvme_cmd_pass_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
		   size_t dvec_nbytes, void *mbuf, size_t mbuf_nbytes)
{
	const int cmd_opts = ctx->opts & XNVME_CMD_MASK;

	switch (cmd_opts & XNVME_CMD_MASK_IOMD) {
	case XNVME_CMD_ASYNC:
		if (ctx->async.queue->base.outstanding == ctx->async.queue->base.capacity) {
			XNVME_DEBUG("FAILED: queue is full; returning -EBUSY");
			return -EBUSY;
		}
		return ctx->dev->be.async.cmd_iov(ctx, dvec, dvec_cnt, dvec_nbytes, mbuf,
						  mbuf_nbytes);
	case XNVME_CMD_SYNC:
		return ctx->dev->be.sync.cmd_iov(ctx, dvec, dvec_cnt, dvec_nbytes, mbuf,
						 mbuf_nbytes);
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

int
xnvme_cmd_pass_pseudo(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
		      size_t mbuf_nbytes)
{
	if (ctx->opts & XNVME_CMD_ASYNC) {
		XNVME_DEBUG("FAILED: PSEUDO-Commands are always sync.");
		return -EINVAL;
	}

	return ctx->dev->be.admin.cmd_pseudo(ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes);
}
