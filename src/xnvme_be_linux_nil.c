// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#define XNVME_BE_LINUX_NIL_NAME "nil"

#ifdef XNVME_BE_LINUX_NIL_ENABLED
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

#include <xnvme_queue.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_nil.h>
#include <xnvme_dev.h>

int
_linux_nil_init(struct xnvme_queue *XNVME_UNUSED(queue), int XNVME_UNUSED(opts))
{
	return 0;
}

int
_linux_nil_term(struct xnvme_queue *XNVME_UNUSED(queue))
{
	return 0;
}

int
_linux_nil_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_nil *queue = (void *)q;
	unsigned completed = 0;

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;

	while (completed < max) {
		unsigned cur = queue->base.outstanding - completed - 1;
		struct xnvme_cmd_ctx *ctx;

		ctx = queue->ctx[cur];
		if (!ctx) {
			XNVME_DEBUG("-{[THIS SHOULD NOT HAPPEN]}-");

			++completed;
			queue->base.outstanding -= completed;
			return -EIO;
		}

		ctx->cpl.status.sc = 0;
		ctx->async.cb(ctx, ctx->async.cb_arg);
		queue->ctx[cur] = NULL;

		++completed;
	};

	queue->base.outstanding -= completed;
	return completed;
}

int
_linux_nil_wait(struct xnvme_queue *queue)
{
	int acc = 0;

	while (queue->base.outstanding) {
		struct timespec ts1 = {.tv_sec = 0, .tv_nsec = 1000};
		int err;

		err = _linux_nil_poke(queue, 0);
		if (err >= 0) {
			acc += err;
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
_linux_nil_cmd_io(struct xnvme_cmd_ctx *ctx, void *XNVME_UNUSED(dbuf),
		  size_t XNVME_UNUSED(dbuf_nbytes), void *XNVME_UNUSED(mbuf),
		  size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_queue_nil *queue = (void *)ctx->async.queue;

	if (queue->base.outstanding == queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

	queue->ctx[queue->base.outstanding++] = ctx;

	return 0;
}

int
_linux_nil_supported(struct xnvme_dev *XNVME_UNUSED(dev), uint32_t XNVME_UNUSED(opts))
{
	return 1;
}

struct xnvme_be_async g_linux_nil = {
	.id = "nil",
#ifdef XNVME_BE_LINUX_IOU_ENABLED
	.enabled = 1,
	.cmd_io = _linux_nil_cmd_io,
	.poke = _linux_nil_poke,
	.wait = _linux_nil_wait,
	.init = _linux_nil_init,
	.term = _linux_nil_term,
	.supported = _linux_nil_supported,
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
