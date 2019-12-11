// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_async.h>

struct xnvme_async_ctx *
xnvme_async_init(struct xnvme_dev *dev, uint32_t depth, uint16_t flags)
{
	if (!(xnvme_is_pow2(depth) && (depth < 4096))) {
		XNVME_DEBUG("EINVAL: depth: %u", depth);
		errno = EINVAL;
		return NULL;
	}

	return dev->be->async_init(dev, depth, flags);
}

int
xnvme_async_term(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx)
{
	return dev->be->async_term(dev, ctx);
}

int
xnvme_async_wait(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx)
{
	return dev->be->async_wait(dev, ctx);
}

int
xnvme_async_poke(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx,
		 uint32_t max)
{
	return dev->be->async_poke(dev, ctx, max);
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
