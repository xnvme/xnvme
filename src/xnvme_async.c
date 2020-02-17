// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_async.h>

int
xnvme_async_init(struct xnvme_dev *dev, struct xnvme_async_ctx **ctx,
		 uint16_t depth, int flags)
{
	if (!(xnvme_is_pow2(depth) && (depth < 4096))) {
		XNVME_DEBUG("EINVAL: depth: %u", depth);
		return -EINVAL;
	}

	return dev->be.func.async_init(dev, ctx, depth, flags);
}

int
xnvme_async_term(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx)
{
	return dev->be.func.async_term(ctx);
}

int
xnvme_async_wait(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx)
{
	return dev->be.func.async_wait(ctx);
}

int
xnvme_async_poke(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx,
		 uint32_t max)
{
	return dev->be.func.async_poke(ctx, max);
}

uint32_t
xnvme_async_get_depth(struct xnvme_async_ctx *ctx)
{
	return ctx->depth;
}

uint32_t
xnvme_async_get_outstanding(struct xnvme_async_ctx *ctx)
{
	return ctx->outstanding;
}
