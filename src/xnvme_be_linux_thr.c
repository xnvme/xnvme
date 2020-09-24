// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#define XNVME_BE_LINUX_THR_NAME "thr"

#ifdef XNVME_BE_LINUX_THR_ENABLED
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
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_thr.h>
#include <xnvme_dev.h>

int
_qp_term(struct _qp *qp)
{
	free(qp);

	return 0;
}

int
_qp_alloc(struct _qp **qp, uint32_t capacity)
{
	const size_t nbytes = capacity * sizeof(*(*qp)->elm) + sizeof(**qp);

	(*qp) = malloc(nbytes);
	if (!(*qp)) {
		return -errno;
	}
	memset((*qp), 0, nbytes);

	STAILQ_INIT(&(*qp)->sq);
	STAILQ_INIT(&(*qp)->cq);
	STAILQ_INIT(&(*qp)->rp);

	(*qp)->capacity = capacity;

	return 0;
}

int
_qp_init(struct _qp *qp)
{
	for (uint32_t i = 0; i < qp->capacity; ++i) {
		STAILQ_INSERT_HEAD(&qp->rp, &qp->elm[i], link);
	}

	return 0;
}

int
_linux_thr_term(struct xnvme_dev *XNVME_UNUSED(dev),
		struct xnvme_async_ctx *ctx)
{
	struct xnvme_async_ctx_thr *actx = (void *)ctx;

	if (!ctx) {
		XNVME_DEBUG("FAILED: ctx: %p", (void *)ctx);
		return -EINVAL;
	}

	_qp_term(actx->qp);

	free(ctx);

	return 0;
}

/**
 * TODO: add a thread-pool consuming the submission-queue, and populating the
 * completion-queue, replacing the submission in _poke consumption of entries in
 * the completion-queue. This should provide better throughput as commands will
 * leave xNVMe and go to the device as new ones are submitted. Additional
 * threads could further improve as overlapping threads can hide the round-trip
 * latency as well as the sw-stack overhead.
 * This should probably be done with an SPMC or something clever.
*/
int
_linux_thr_init(struct xnvme_dev *dev, struct xnvme_async_ctx **ctx,
		uint16_t depth, int XNVME_UNUSED(flags))
{
	struct xnvme_async_ctx_thr **actx = (void *)ctx;

	*ctx = calloc(1, sizeof(**ctx));
	if (!*ctx) {
		XNVME_DEBUG("FAILED: calloc(ctx), errno: %s", strerror(errno));
		return -errno;
	}
	(*ctx)->depth = depth;

	if (_qp_alloc(&(*actx)->qp, depth)) {
		XNVME_DEBUG("FAILED: _qp_alloc()");
		goto failed;
	}
	if (_qp_init((*actx)->qp)) {
		XNVME_DEBUG("FAILED: _qp_init()");
		goto failed;
	}

	return 0;

failed:
	_linux_thr_term(dev, *ctx);

	return 1;
}

int
_linux_thr_poke(struct xnvme_dev *dev,
		struct xnvme_async_ctx *ctx, uint32_t max)
{
	struct xnvme_async_ctx_thr *actx = (void *)ctx;
	struct _qp *qp = actx->qp;
	unsigned completed = 0;

	max = max ? max : actx->outstanding;
	max = max > actx->outstanding ? actx->outstanding : max;

	while (completed < max) {
		struct _entry *entry = STAILQ_FIRST(&qp->sq);
		int err;

		STAILQ_REMOVE_HEAD(&qp->sq, link);

		err = dev->be.sync.cmd_io(entry->dev, &entry->cmd, entry->dbuf,
					  entry->dbuf_nbytes, entry->mbuf,
					  entry->mbuf_nbytes, XNVME_CMD_SYNC,
					  entry->req);
		if (err) {
			XNVME_DEBUG("FAILED: err: %d", err);
			entry->req->cpl.status.sc = err;
		}
		STAILQ_INSERT_TAIL(&qp->rp, entry, link);
		entry->req->async.cb(entry->req, entry->req->async.cb_arg);

		++completed;
	};

	actx->outstanding -= completed;
	return completed;
}

int
_linux_thr_wait(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx)
{
	int acc = 0;

	while (ctx->outstanding) {
		struct timespec ts1 = {.tv_sec = 0, .tv_nsec = 1000};
		int err;

		err = _linux_thr_poke(dev, ctx, 0);
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
_linux_thr_cmd_io(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *dbuf,
		  size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes,
		  int XNVME_UNUSED(opts), struct xnvme_req *req)
{
	struct xnvme_async_ctx_thr *actx = (void *)req->async.ctx;
	struct _qp *qp = actx->qp;
	struct _entry *entry;

	if (actx->outstanding == actx->depth) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

	// Grab entry from rp and push into sq
	entry = STAILQ_FIRST(&qp->rp);
	if (!entry) {
		XNVME_DEBUG("FAILED: should not happen");
		return -EIO;
	}
	STAILQ_REMOVE_HEAD(&qp->rp, link);

	entry->dev = dev;
	entry->cmd = *cmd;
	entry->dbuf = dbuf;
	entry->dbuf_nbytes = dbuf_nbytes;
	entry->mbuf = mbuf;
	entry->mbuf_nbytes = mbuf_nbytes;
	entry->req = req;

	STAILQ_INSERT_TAIL(&qp->sq, entry, link);

	actx->outstanding += 1;

	return 0;
}

int
_linux_thr_supported(struct xnvme_dev *XNVME_UNUSED(dev),
		     uint32_t XNVME_UNUSED(opts))
{
	return 1;
}

struct xnvme_be_async g_linux_thr = {
	.id = "thr",
#ifdef XNVME_BE_LINUX_IOU_ENABLED
	.enabled = 1,
	.cmd_io = _linux_thr_cmd_io,
	.poke = _linux_thr_poke,
	.wait = _linux_thr_wait,
	.init = _linux_thr_init,
	.term = _linux_thr_term,
	.supported = _linux_thr_supported,
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
