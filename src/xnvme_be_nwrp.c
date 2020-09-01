// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#define XNVME_BE_NWRP_NAME "nwrp"

#ifdef XNVME_BE_NWRP_ENABLED
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/nvme_ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <paths.h>

#include <xnvme_async.h>
#include <xnvme_be_lioc.h>
#include <xnvme_be_nwrp.h>
#include <xnvme_dev.h>

int
xnvme_be_nwrp_async_init(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_async_ctx **ctx, uint16_t depth,
			 int XNVME_UNUSED(flags))
{
	*ctx = calloc(1, sizeof(**ctx));
	if (!*ctx) {
		XNVME_DEBUG("FAILED: calloc(ctx), errno: %s", strerror(errno));
		return -errno;
	}
	(*ctx)->depth = depth;

	return 0;
}

int
xnvme_be_nwrp_async_term(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_async_ctx *ctx)
{
	if (!ctx) {
		XNVME_DEBUG("FAILED: ctx: %p", (void *)ctx);
		return -EINVAL;
	}

	free(ctx);

	return 0;
}

int
xnvme_be_nwrp_async_poke(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_async_ctx *ctx, uint32_t max)
{
	struct xnvme_async_ctx_nwrp *lctx = (void *)ctx;
	unsigned completed = 0;

	max = max ? max : lctx->outstanding;
	max = max > lctx->outstanding ? lctx->outstanding : max;

	while (completed < max) {
		unsigned cur = lctx->outstanding - completed - 1;
		struct xnvme_req *req;

		req = lctx->reqs[cur];
		if (!req) {
			XNVME_DEBUG("-{[THIS SHOULD NOT HAPPEN]}-");

			++completed;
			ctx->outstanding -= completed;
			return -EIO;
		}

		req->cpl.status.sc = 0;
		req->async.cb(req, req->async.cb_arg);
		lctx->reqs[cur] = NULL;

		++completed;
	};

	lctx->outstanding -= completed;
	return completed;
}

int
xnvme_be_nwrp_async_wait(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx)
{
	int acc = 0;

	while (ctx->outstanding) {
		struct timespec ts1 = {.tv_sec = 0, .tv_nsec = 1000};
		int err;

		err = xnvme_be_nwrp_async_poke(dev, ctx, 0);
		if (!err) {
			acc += 1;
			continue;
		}

		switch (err) {
		case -EAGAIN:
		case -EBUSY:
			nanosleep(&ts1, NULL);
			continue;

		default:
			return err;
		}
	}

	return acc;
}

static inline int
async_io(struct xnvme_dev *XNVME_UNUSED(dev), int XNVME_UNUSED(opcode),
	 struct xnvme_spec_cmd *XNVME_UNUSED(cmd), void *XNVME_UNUSED(dbuf),
	 size_t XNVME_UNUSED(dbuf_nbytes), void *XNVME_UNUSED(mbuf),
	 size_t XNVME_UNUSED(mbuf_nbytes), int XNVME_UNUSED(opts),
	 struct xnvme_req *req)
{
	struct xnvme_async_ctx_nwrp *lctx = (void *)req->async.ctx;

	if (lctx->outstanding == lctx->depth) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

	lctx->reqs[lctx->outstanding++] = req;

	return 0;
}

int
xnvme_be_nwrp_cmd_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
		       void *dbuf, size_t dbuf_nbytes, void *mbuf,
		       size_t mbuf_nbytes, int opts, struct xnvme_req *req)
{
	if (!(opts & XNVME_CMD_ASYNC)) {
		return xnvme_be_lioc_cmd_pass(dev, cmd, dbuf, dbuf_nbytes, mbuf,
					      mbuf_nbytes, opts, req);
	}
	if (!req) {
		XNVME_DEBUG("FAILED: missing req");
		return -EINVAL;
	}

	{
		struct xnvme_async_ctx_nwrp *ctx = (void *)(req->async.ctx);

		if (ctx->outstanding == ctx->depth) {
			XNVME_DEBUG("FAILED: queue is full");
			return -EBUSY;
		}

		ctx->reqs[ctx->outstanding++] = req;
		((char *)dbuf)[0] = 0;
	}

	return 0;
}

void
xnvme_be_nwrp_state_term(struct xnvme_be_lioc_state *state)
{
	if (!state) {
		return;
	}

	close(state->fd);
}

int
xnvme_be_nwrp_state_init(struct xnvme_dev *dev)
{
	struct xnvme_be_nwrp_state *state = (void *)dev->be.state;
	struct stat dev_stat;
	int err;

	state->fd = open(dev->ident.trgt, O_RDWR | O_DIRECT);
	if (state->fd < 0) {
		XNVME_DEBUG("FAILED: open(parts.trgt(%s)), state->fd(%d)\n",
			    dev->ident.trgt, state->fd);
		return -errno;
	}

	err = fstat(state->fd, &dev_stat);
	if (err < 0) {
		return -errno;
	}
	if (!S_ISBLK(dev_stat.st_mode)) {
		XNVME_DEBUG("FAILED: device is not a block device");
		return -ENOTBLK;
	}

	return 0;
}

int
xnvme_be_nwrp_dev_from_ident(const struct xnvme_ident *ident,
			     struct xnvme_dev **dev)
{
	int err;

	err = xnvme_dev_alloc(dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_dev_alloc()");
		return err;
	}
	(*dev)->ident = *ident;
	(*dev)->be = xnvme_be_nwrp;

	err = xnvme_be_nwrp_state_init(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_nwrp_state_init()");
		free(*dev);
		return err;
	}
	err = xnvme_be_lioc_dev_idfy(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_lioc_dev_idfy()");
		xnvme_be_nwrp_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}
	err = xnvme_be_dev_derive_geometry(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_dev_derive_geometry()");
		xnvme_be_nwrp_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}
	// TODO: consider this. Due to Kernel-segment constraint force mdts down
	if (((*dev)->geo.mdts_nbytes / (*dev)->geo.lba_nbytes) > 127) {
		(*dev)->geo.mdts_nbytes = (*dev)->geo.lba_nbytes * 127;
	}

	return 0;
}

void
xnvme_be_nwrp_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_be_lioc_dev_close(dev);
}

int
xnvme_be_nwrp_enumerate(struct xnvme_enumeration *XNVME_UNUSED(list),
			const char *XNVME_UNUSED(sys_uri),
			int XNVME_UNUSED(opts))
{
	return 0;
}
#endif

static const char *g_schemes[] = {
	XNVME_BE_NWRP_NAME,
	"file",
};

struct xnvme_be xnvme_be_nwrp = {
#ifdef XNVME_BE_NWRP_ENABLED
	.func = {
		.cmd_pass = xnvme_be_nwrp_cmd_pass,
		.cmd_pass_admin = xnvme_be_lioc_cmd_pass_admin,

		.async_init = xnvme_be_nwrp_async_init,
		.async_term = xnvme_be_nwrp_async_term,
		.async_poke = xnvme_be_nwrp_async_poke,
		.async_wait = xnvme_be_nwrp_async_wait,

		.buf_alloc = xnvme_be_lioc_buf_alloc,
		.buf_vtophys = xnvme_be_lioc_buf_vtophys,
		.buf_realloc = xnvme_be_lioc_buf_realloc,
		.buf_free = xnvme_be_lioc_buf_free,

		.enumerate = xnvme_be_nwrp_enumerate,

		.dev_from_ident = xnvme_be_nwrp_dev_from_ident,
		.dev_close = xnvme_be_nwrp_dev_close,
	},
#else
	.func = XNVME_BE_NOSYS_FUNC,
#endif
	.attr = {
		.name = XNVME_BE_NWRP_NAME,
#ifdef XNVME_BE_NWRP_ENABLED
		.enabled = 1,
#else
		.enabled = 0,
#endif
		.schemes = g_schemes,
		.nschemes = sizeof g_schemes / sizeof(*g_schemes),
	},
	.state = { 0 },
};
