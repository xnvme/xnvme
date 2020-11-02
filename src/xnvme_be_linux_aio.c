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

#ifdef XNVME_BE_LINUX_AIO_ENABLED
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
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_aio.h>
#include <xnvme_dev.h>

int
_linux_aio_init(struct xnvme_dev *XNVME_UNUSED(dev),
		struct xnvme_async_ctx **ctx, uint16_t depth,
		int XNVME_UNUSED(flags))
{
	struct xnvme_async_ctx_aio *actx = NULL;
	int err = 0;

	(*ctx) = calloc(1, sizeof(**ctx));
	if (!(*ctx)) {
		XNVME_DEBUG("FAILED: calloc, ctx: %p, errno: %s",
			    (void *)(*ctx), strerror(errno));
		return -errno;
	}

	(*ctx)->depth = depth;
	actx = (void *)(*ctx);

	actx->aio_ctx = 0;
	actx->entries = depth;
	actx->aio_events = calloc(actx->entries, sizeof(struct io_event));
	actx->iocbs = calloc(actx->entries, sizeof(struct iocb *));

	err = io_queue_init(actx->entries, &actx->aio_ctx);
	if (err) {
		XNVME_DEBUG("FAILED: alloc. qpair");
		free(*ctx);
		return err;
	}

	return 0;
}

int
_linux_aio_term(struct xnvme_dev *XNVME_UNUSED(dev),
		struct xnvme_async_ctx *ctx)
{
	struct xnvme_async_ctx_aio *actx = NULL;

	if (!ctx) {
		XNVME_DEBUG("FAILED: ctx: %p", (void *)ctx);
		return -EINVAL;
	}

	actx = (void *)ctx;

	io_destroy(actx->aio_ctx);
	free(actx->aio_events);
	free(actx->iocbs);
	free(ctx);

	return 0;
}

int
_linux_aio_poke(struct xnvme_dev *XNVME_UNUSED(dev),
		struct xnvme_async_ctx *ctx, uint32_t max)
{
	struct xnvme_async_ctx_aio *actx = (void *)ctx;
	unsigned completed = 0;
	int ret = 0, event = 0;

	max = max ? max : actx->outstanding;
	max = max > actx->outstanding ? actx->outstanding : max;

	do {
		struct io_event *ev;
		struct xnvme_req *req;

		ret = io_getevents(actx->aio_ctx, 1, max, actx->aio_events, NULL);

		for (event = 0; event < ret; event++) {
			ev =  actx->aio_events + event;
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

	actx->outstanding -= completed;
	return completed;
}

int
_linux_aio_wait(struct xnvme_dev *dev,
		struct xnvme_async_ctx *ctx)
{
	int acc = 0;

	while (ctx->outstanding) {
		struct timespec ts1 = {.tv_sec = 0, .tv_nsec = 1000};
		int err;

		err = _linux_aio_poke(dev, ctx, 0);
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

static inline void
_ring_inc(struct xnvme_async_ctx_aio *actx, unsigned int *val, unsigned int add)
{
	*val = (*val + add) & (actx->entries - 1);
}

int
_linux_aio_cmd_io(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
		  void *dbuf, size_t dbuf_nbytes, void *mbuf,
		  size_t mbuf_nbytes, int XNVME_UNUSED(opts),
		  struct xnvme_req *req)
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;
	struct xnvme_async_ctx_aio *actx  = (void *)req->async.ctx;
	struct iocb *iocb = &actx->iocb;
	struct iocb **iocbs;
	int ret = 0;

	switch (cmd->common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		io_prep_pwrite(iocb, state->fd, dbuf, dbuf_nbytes, cmd->nvm.slba << dev->ssw);
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		io_prep_pread(iocb, state->fd, dbuf, dbuf_nbytes, cmd->nvm.slba << dev->ssw);
		break;

	default:
		return -ENOSYS;
	}

	if (actx->outstanding < actx->depth) {
		actx->queued++;
	}

	if (actx->outstanding == actx->depth) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}
	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOSYS;
	}

	iocb->data = (unsigned long *)req;
	actx->iocbs[actx->head] = &actx->iocb;

	_ring_inc(actx, &actx->head, 1);

	actx->outstanding += 1;

	do {
		long nr = actx->queued;
		nr = XNVME_MIN((unsigned int) nr, actx->entries - actx->tail);

		iocbs = actx->iocbs + actx->tail;

		ret = io_submit(actx->aio_ctx, nr, iocbs);

		if (ret > 0) {
			actx->queued -= ret;
			_ring_inc(actx, &actx->tail, ret);
			ret = 0;
		} else if (ret == -EINTR || !ret) {
			continue;
		} else if (ret == -EAGAIN) {
			if (actx->queued) {
				ret = 0;
				break;
			}
			continue;
		} else if (ret == -ENOMEM) {

			if (actx->queued) {
				ret = 0;
			}
			break;
		} else {
			break;
		}
	} while (actx->queued);

	return 0;
}

int
_linux_aio_supported(struct xnvme_dev *XNVME_UNUSED(dev),
		     uint32_t XNVME_UNUSED(opts))
{
	return 1;
}

struct xnvme_be_async g_linux_aio = {
	.id = "aio",
#ifdef XNVME_BE_LINUX_AIO_ENABLED
	.enabled = 1,
	.cmd_io = _linux_aio_cmd_io,
	.poke = _linux_aio_poke,
	.wait = _linux_aio_wait,
	.init = _linux_aio_init,
	.term = _linux_aio_term,
	.supported = _linux_aio_supported,
#else
	.enabled = 0,
	.cmd_io = xnvme_be_nosys_async_cmd_io,
	.poke = xnvme_be_nosys_async_poke,
	.wait = xnvme_be_nosys_async_wait,
	.init = xnvme_be_nosys_async_init,
	.term = xnvme_be_nosys_async_term,
	.supported = xnvme_be_nosys_async_supported,
#endif
};

#endif
