// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_sgl.h>

/**
 * Calling this requires that opts at least has `XNVME_CMD_SGL_DATA`
 */
static inline void
xnvme_sgl_setup(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *data, void *meta,
		int opts)
{
	struct xnvme_sgl *sgl = data;
	uint64_t phys;

	cmd->common.psdt = XNVME_SPEC_PSDT_SGL_MPTR_CONTIGUOUS;

	if (sgl->ndescr == 1) {
		cmd->common.dptr.sgl = sgl->descriptors[0];
	} else {
		xnvme_buf_vtophys(dev, sgl->descriptors, &phys);

		cmd->common.dptr.sgl.unkeyed.type = XNVME_SPEC_SGL_DESCR_TYPE_LAST_SEGMENT;
		cmd->common.dptr.sgl.unkeyed.len = sgl->ndescr * sizeof(struct xnvme_spec_sgl_descriptor);
		cmd->common.dptr.sgl.addr = phys;
	}

	if ((opts & XNVME_CMD_UPLD_SGLM) && meta) {
		sgl = meta;

		xnvme_buf_vtophys(dev, sgl->descriptors, &phys);

		cmd->common.psdt = XNVME_SPEC_PSDT_SGL_MPTR_SGL;

		if (sgl->ndescr == 1) {
			cmd->common.mptr = phys;
		} else {
			sgl->indirect->unkeyed.type = XNVME_SPEC_SGL_DESCR_TYPE_LAST_SEGMENT;
			sgl->indirect->unkeyed.len = sgl->ndescr * sizeof(struct xnvme_spec_sgl_descriptor);
			sgl->indirect->addr = phys;

			xnvme_buf_vtophys(dev, sgl->indirect, &phys);
			cmd->common.mptr = phys;
		}
	}
}

// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>

void
xnvme_cmd_ctx_pool_free(struct xnvme_cmd_ctx_pool *pool)
{
	free(pool);
}

int
xnvme_cmd_ctx_pool_alloc(struct xnvme_cmd_ctx_pool **pool, uint32_t capacity)
{
	const size_t nbytes = capacity * sizeof(*(*pool)->elm) + sizeof(**pool);

	(*pool) = malloc(nbytes);
	if (!(*pool)) {
		return -errno;
	}
	memset((*pool), 0, nbytes);

	SLIST_INIT(&(*pool)->head);

	(*pool)->capacity = capacity;

	return 0;
}

int
xnvme_cmd_ctx_pool_init(struct xnvme_cmd_ctx_pool *pool,
			struct xnvme_queue *queue,
			xnvme_queue_cb cb,
			void *cb_args)
{
	for (uint32_t i = 0; i < pool->capacity; ++i) {
		pool->elm[i].pool = pool;
		pool->elm[i].async.queue = queue;
		pool->elm[i].async.cb = cb;
		pool->elm[i].async.cb_arg = cb_args;

		SLIST_INSERT_HEAD(&pool->head, &pool->elm[i], link);
	}

	return 0;
}

void
xnvme_cmd_ctx_pr(const struct xnvme_cmd_ctx *ctx, int XNVME_UNUSED(opts))
{
	printf("xnvme_cmd_ctx: ");

	if (!ctx) {
		printf("~\n");
		return;
	}

	printf("{cdw0: 0x%x, sc: 0x%x, sct: 0x%x}\n", ctx->cpl.cdw0,
	       ctx->cpl.status.sc, ctx->cpl.status.sct);
}

void
xnvme_cmd_ctx_clear(struct xnvme_cmd_ctx *cmd_ctx)
{
	memset(cmd_ctx, 0x0, sizeof(*cmd_ctx));
}

int
xnvme_cmd_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *dbuf,
	       size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes, int opts,
	       struct xnvme_cmd_ctx *cmd_ctx)
{
	const int cmd_opts = opts & XNVME_CMD_MASK;

	if ((cmd_opts & XNVME_CMD_MASK_UPLD) && dbuf) {
		xnvme_sgl_setup(dev, cmd, dbuf, mbuf, opts);
	}

	switch (cmd_opts & XNVME_CMD_MASK_IOMD) {
	case XNVME_CMD_ASYNC:
		return dev->be.async.cmd_io(dev, cmd, dbuf, dbuf_nbytes, mbuf,
					    mbuf_nbytes, opts, cmd_ctx);

	case XNVME_CMD_SYNC:
		return dev->be.sync.cmd_io(dev, cmd, dbuf, dbuf_nbytes, mbuf,
					   mbuf_nbytes, opts, cmd_ctx);

	default:
		XNVME_DEBUG("FAILED: command-mode not provided");
		return -EINVAL;
	}
}

int
xnvme_cmd_pass_admin(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
		     void *dbuf, size_t dbuf_nbytes, void *mbuf,
		     size_t mbuf_nbytes, int opts, struct xnvme_cmd_ctx *cmd_ctx)
{
	if (XNVME_CMD_ASYNC & opts) {
		XNVME_DEBUG("FAILED: Admin commands are always sync.");
		return -EINVAL;
	}
	if ((opts & XNVME_CMD_MASK_UPLD) && dbuf) {
		xnvme_sgl_setup(dev, cmd, dbuf, mbuf, opts);
	}

	return dev->be.sync.cmd_admin(dev, cmd, dbuf, dbuf_nbytes, mbuf,
				      mbuf_nbytes, opts, cmd_ctx);
}
