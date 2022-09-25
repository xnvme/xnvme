// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_LIBAIO_ENABLED
#include <errno.h>
#include <libaio.h>
#include <xnvme_queue.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_libaio.h>
#include <xnvme_dev.h>
#include <libxnvme_spec_fs.h>

int
_linux_libaio_term(struct xnvme_queue *q)
{
	struct xnvme_queue_libaio *queue = (void *)q;

	if (!queue) {
		XNVME_DEBUG("FAILED: queue: %p", (void *)queue);
		return -EINVAL;
	}

	io_destroy(queue->aio_ctx);
	free(queue->aio_events);

	return 0;
}

int
_linux_libaio_init(struct xnvme_queue *q, int opts)
{
	struct xnvme_queue_libaio *queue = (void *)q;
	struct xnvme_be_linux_state *state = (void *)queue->base.dev->be.state;
	int err = 0;

	queue->poll_io = (opts & XNVME_QUEUE_IOPOLL) || state->poll_io;

	queue->aio_ctx = 0;
	queue->aio_events = calloc(queue->base.capacity, sizeof(struct io_event));

	err = io_queue_init(queue->base.capacity, &queue->aio_ctx);
	if (err) {
		XNVME_DEBUG("FAILED: io_queue_init(), err: %d", err);
		return err;
	}

	return 0;
}

int
_linux_libaio_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_libaio *queue = (void *)q;
	struct timespec timeout = {.tv_sec = 0, .tv_nsec = 100000};
	int min = queue->poll_io ? 0 : 1;
	int completed = 0;

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;

	completed = io_getevents(queue->aio_ctx, min, max, queue->aio_events, &timeout);
	if (completed < 0) {
		XNVME_DEBUG("FAILED: completed: %d, errno: %d", completed, errno);
		return completed;
	}

	for (int event = 0; event < completed; event++) {
		struct io_event *ev = &queue->aio_events[event];
		struct xnvme_cmd_ctx *ctx = (struct xnvme_cmd_ctx *)(uintptr_t)ev->data;

		if (!ctx) {
			XNVME_DEBUG("-{[THIS SHOULD NOT HAPPEN]}-");
			XNVME_DEBUG("event->data is NULL! => NO REQ!");
			XNVME_DEBUG("event->res: %ld", ev->res);
			XNVME_DEBUG("unprocessed events might remain");

			queue->base.outstanding -= 1;

			return -EIO;
		}

		ctx->cpl.result = ev->res;
		if (((int64_t)ev->res) < 0) {
			XNVME_DEBUG("FAILED: res: %lu, res2: %lu", ev->res, ev->res2);
			ctx->cpl.result = 0;
			ctx->cpl.status.sc = -ev->res;
			ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
		}

		ctx->async.cb(ctx, ctx->async.cb_arg);
	}

	queue->base.outstanding -= completed;
	return completed;
}

int
_linux_libaio_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
		     size_t mbuf_nbytes)
{
	struct xnvme_queue_libaio *queue = (void *)ctx->async.queue;
	struct xnvme_be_linux_state *state = (void *)queue->base.dev->be.state;
	const uint64_t ssw = queue->base.dev->geo.ssw;

	struct iocb *iocb = (void *)&ctx->cmd;
	int err;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOSYS;
	}

	///< Convert the NVMe command/sqe to an Linux aio io-control-block
	///< NOTE: opcode-dispatch (io)
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		io_prep_pwrite(iocb, state->fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba << ssw);
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		io_prep_pread(iocb, state->fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba << ssw);
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		io_prep_pwrite(iocb, state->fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba);
		break;

	case XNVME_SPEC_FS_OPC_READ:
		io_prep_pread(iocb, state->fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba);
		break;

		// TODO: determine how to handle fsync

	default:
		XNVME_DEBUG("FAILED: unsupported opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	iocb->data = (unsigned long *)ctx;

	err = io_submit(queue->aio_ctx, 1, &iocb);
	if (err == 1) {
		ctx->async.queue->base.outstanding += 1;
		return 0;
	}

	XNVME_DEBUG("FAILED: io_submit(), err: %d", err);

	return err;
}

int
_linux_libaio_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
		      size_t XNVME_UNUSED(dvec_nbytes), struct iovec *mvec, size_t mvec_cnt,
		      size_t mvec_nbytes)
{
	struct xnvme_queue_libaio *queue = (void *)ctx->async.queue;
	struct xnvme_be_linux_state *state = (void *)queue->base.dev->be.state;
	const uint64_t ssw = queue->base.dev->geo.ssw;

	struct iocb *iocb = (void *)&ctx->cmd;
	int err;

	if (queue->base.outstanding == queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}
	if (mvec || mvec_cnt || mvec_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOSYS;
	}

	///< Convert the NVMe command/sqe to an Linux aio io-control-block
	///< NOTE: opcode-dispatch (io)
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		io_prep_pwritev(iocb, state->fd, dvec, dvec_cnt, ctx->cmd.nvm.slba << ssw);
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		io_prep_preadv(iocb, state->fd, dvec, dvec_cnt, ctx->cmd.nvm.slba << ssw);
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		io_prep_pwritev(iocb, state->fd, dvec, dvec_cnt, ctx->cmd.nvm.slba);
		break;

	case XNVME_SPEC_FS_OPC_READ:
		io_prep_preadv(iocb, state->fd, dvec, dvec_cnt, ctx->cmd.nvm.slba);
		break;

		// TODO: determine how to handle fsync

	default:
		XNVME_DEBUG("FAILED: unsupported opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	iocb->data = (unsigned long *)ctx;

	err = io_submit(queue->aio_ctx, 1, &iocb);
	if (err == 1) {
		ctx->async.queue->base.outstanding += 1;
		return 0;
	}

	XNVME_DEBUG("FAILED: io_submit(), err: %d", err);

	return err;
}
#endif

struct xnvme_be_async g_xnvme_be_linux_async_libaio = {
	.id = "libaio",
#ifdef XNVME_BE_LINUX_LIBAIO_ENABLED
	.cmd_io = _linux_libaio_cmd_io,
	.cmd_iov = _linux_libaio_cmd_iov,
	.poke = _linux_libaio_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = _linux_libaio_init,
	.term = _linux_libaio_term,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
#endif
};
