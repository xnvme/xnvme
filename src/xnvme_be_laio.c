// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#define XNVME_BE_LAIO_NAME "laio"

#ifdef XNVME_BE_LAIO_ENABLED
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
#include <libaio.h>

#include <xnvme_async.h>
#include <xnvme_be_lioc.h>
#include <xnvme_be_laio.h>
#include <xnvme_dev.h>

static inline void
ring_inc(struct xnvme_async_ctx_laio *lctx, unsigned int *val,
	 unsigned int add)
{
	*val = (*val + add) & (lctx->entries - 1);
}

int
xnvme_be_laio_async_init(struct xnvme_dev *XNVME_UNUSED(dev), struct xnvme_async_ctx **ctx,
			 uint16_t depth, int XNVME_UNUSED(flags))
{
	struct xnvme_async_ctx_laio *lctx = NULL;
	int err = 0;

	(*ctx) = calloc(1, sizeof(**ctx));
	if (!(*ctx)) {
		XNVME_DEBUG("FAILED: calloc, ctx: %p, errno: %s",
			    (void *)(*ctx), strerror(errno));
		return -errno;
	}

	(*ctx)->depth = depth;
	lctx = (void *)(*ctx);

	lctx->aio_ctx = 0;
	lctx->entries = depth;
	lctx->aio_events = calloc(lctx->entries, sizeof(struct io_event));
	lctx->iocbs = calloc(lctx->entries, sizeof(struct iocb *));

	err = io_queue_init(lctx->entries, &lctx->aio_ctx);
	if (err) {
		XNVME_DEBUG("FAILED: alloc. qpair");
		free(*ctx);
		return err;
	}

	return 0;
}

int
xnvme_be_laio_async_term(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_async_ctx *ctx)
{
	struct xnvme_async_ctx_laio *lctx = NULL;

	if (!ctx) {
		XNVME_DEBUG("FAILED: ctx: %p", (void *)ctx);
		return -EINVAL;
	}

	lctx = (void *)ctx;

	io_destroy(lctx->aio_ctx);
	free(lctx->aio_events);
	free(lctx->iocbs);
	free(ctx);

	return 0;
}

int
xnvme_be_laio_async_poke(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_async_ctx *ctx, uint32_t max)
{
	struct xnvme_async_ctx_laio *lctx = (void *)ctx;
	unsigned completed = 0;
	int ret = 0, event = 0;

	max = max ? max : lctx->outstanding;
	max = max > lctx->outstanding ? lctx->outstanding : max;

	do {
		struct io_event *ev;
		struct xnvme_req *req;

		ret = io_getevents(lctx->aio_ctx, 1, max, lctx->aio_events, NULL);

		for (event = 0; event < ret; event++) {
			ev =  lctx->aio_events + event;
			req = (struct xnvme_req *)(uintptr_t)ev->data;

			if (!req) {
				XNVME_DEBUG("-{[THIS SHOULD NOT HAPPEN]}-");
				XNVME_DEBUG("event->data is NULL! => NO REQ!");
				XNVME_DEBUG("event->res: %ld", ev->res);

				ctx->outstanding -= 1;

				return -EIO;
			}

			// Map event-result to req-completion
			req->cpl.status.sc = ev->res;
			req->async.cb(req, req->async.cb_arg);

			++completed;
		}

	} while (completed < max);

	lctx->outstanding -= completed;
	return completed;
}

int
xnvme_be_laio_async_wait(struct xnvme_dev *dev,
			 struct xnvme_async_ctx *ctx)
{
	int acc = 0;

	while (ctx->outstanding) {
		struct timespec ts1 = {.tv_sec = 0, .tv_nsec = 1000};
		int err;

		err = xnvme_be_laio_async_poke(dev, ctx, 0);
		if (!err) {
			acc += 1;
			continue;
		}

		switch (err) {
		case 0:
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
async_io(struct xnvme_dev *dev, int opcode, struct xnvme_spec_cmd *cmd,
	 void *dbuf, size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes,
	 int XNVME_UNUSED(opts), struct xnvme_req *req)
{
	struct xnvme_be_laio_state *state = (void *)dev->be.state;
	struct xnvme_async_ctx_laio *lctx  = (void *)req->async.ctx;
	struct iocb *iocb = &lctx->iocb;
	struct iocb **iocbs;
	int ret = 0;

	switch (opcode) {
	case XNVME_SPEC_OPC_WRITE:
		io_prep_pwrite(iocb, state->fd, dbuf, dbuf_nbytes, cmd->lblk.slba << dev->ssw);
		break;

	case XNVME_SPEC_OPC_READ:
		io_prep_pread(iocb, state->fd, dbuf, dbuf_nbytes, cmd->lblk.slba << dev->ssw);
		break;

	default:
		return -ENOSYS;
	}

	if (lctx->outstanding < lctx->depth) {
		lctx->queued++;
	}

	if (lctx->outstanding == lctx->depth) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}
	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOSYS;
	}

	iocb->data = (unsigned long *)req;
	lctx->iocbs[lctx->head] = &lctx->iocb;

	ring_inc(lctx, &lctx->head, 1);

	lctx->outstanding += 1;

	do {
		long nr = lctx->queued;
		nr = XNVME_MIN((unsigned int) nr, lctx->entries - lctx->tail);

		iocbs = lctx->iocbs + lctx->tail;

		ret = io_submit(lctx->aio_ctx, nr, iocbs);

		if (ret > 0) {

			lctx->queued -= ret;
			ring_inc(lctx, &lctx->tail, ret);
			ret = 0;
		} else if (ret == -EINTR || !ret) {
			continue;
		} else if (ret == -EAGAIN) {
			if (lctx->queued) {
				ret = 0;
				break;
			}

			continue;
		} else if (ret == -ENOMEM) {

			if (lctx->queued) {
				ret = 0;
			}
			break;
		} else {
			break;
		}
	} while (lctx->queued);

	return 0;
}

int
xnvme_be_laio_cmd_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
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

	return async_io(dev, cmd->common.opcode, cmd, dbuf, dbuf_nbytes,
			mbuf, mbuf_nbytes, opts, req);
}

void
xnvme_be_laio_state_term(struct xnvme_be_lioc_state *state)
{
	if (!state) {
		return;
	}

	close(state->fd);
}

int
xnvme_be_laio_state_init(struct xnvme_dev *dev)
{
	struct xnvme_be_laio_state *state = (void *)dev->be.state;
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
xnvme_be_laio_dev_from_ident(const struct xnvme_ident *ident,
			     struct xnvme_dev **dev)
{
	int err;

	err = xnvme_dev_alloc(dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_dev_alloc()");
		return err;
	}
	(*dev)->ident = *ident;
	(*dev)->be = xnvme_be_laio;

	err = xnvme_be_laio_state_init(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_laio_state_init()");
		free(*dev);
		return err;
	}

	err = xnvme_be_lioc_dev_idfy(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_lioc_dev_idfy()");
		xnvme_be_laio_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}

	err = xnvme_be_dev_derive_geometry(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_dev_derive_geometry()");
		xnvme_be_laio_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}

	return 0;
}

void
xnvme_be_laio_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_be_lioc_dev_close(dev);
}

int
xnvme_be_laio_enumerate(struct xnvme_enumeration *list, int XNVME_UNUSED(opts))
{
	struct dirent **ns = NULL;
	int nns = 0;

	nns = scandir("/sys/block", &ns, xnvme_path_nvme_filter, alphasort);
	for (int ni = 0; ni < nns; ++ni) {
		char uri[XNVME_IDENT_URI_LEN] = { 0 };
		struct xnvme_ident ident = { 0 };
		struct xnvme_dev *dev;

		snprintf(uri, XNVME_IDENT_URI_LEN - 1,
			 XNVME_BE_LAIO_NAME ":" _PATH_DEV "%s",
			 ns[ni]->d_name);
		if (xnvme_ident_from_uri(uri, &ident)) {
			continue;
		}

		if (xnvme_be_laio_dev_from_ident(&ident, &dev)) {
			XNVME_DEBUG("FAILED: xnvme_be_laio_dev_from_ident()");
			continue;
		}

		xnvme_be_laio_dev_close(dev);
		free(dev);

		if (xnvme_enumeration_append(list, &ident)) {
			XNVME_DEBUG("FAILED: adding entry");
		}
	}

	for (int ni = 0; ni < nns; ++ni) {
		free(ns[ni]);
	}
	free(ns);

	return 0;
}
#endif

static const char *g_schemes[] = {
	XNVME_BE_LAIO_NAME,
	"file",
};

struct xnvme_be xnvme_be_laio = {
#ifdef XNVME_BE_LAIO_ENABLED
	.func = {
		.cmd_pass = xnvme_be_laio_cmd_pass,
		.cmd_pass_admin = xnvme_be_lioc_cmd_pass_admin,

		.async_init = xnvme_be_laio_async_init,
		.async_term = xnvme_be_laio_async_term,
		.async_poke = xnvme_be_laio_async_poke,
		.async_wait = xnvme_be_laio_async_wait,

		.buf_alloc = xnvme_be_lioc_buf_alloc,
		.buf_vtophys = xnvme_be_lioc_buf_vtophys,
		.buf_realloc = xnvme_be_lioc_buf_realloc,
		.buf_free = xnvme_be_lioc_buf_free,

		.enumerate = xnvme_be_laio_enumerate,

		.dev_from_ident = xnvme_be_laio_dev_from_ident,
		.dev_close = xnvme_be_laio_dev_close,
	},
#else
	.func = XNVME_BE_NOSYS_FUNC,
#endif
	.attr = {
		.name = XNVME_BE_LAIO_NAME,
#ifdef XNVME_BE_LAIO_ENABLED
		.enabled = 1,
#else
		.enabled = 0,
#endif
		.schemes = g_schemes,
		.nschemes = sizeof g_schemes / sizeof(*g_schemes),
	},
	.state = { 0 },
};
