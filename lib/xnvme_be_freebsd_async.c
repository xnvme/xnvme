// Copyright (c) 2023 Fedor Uporov <fuporov.vstack@gmail.com>
//
// SPDX-License-Identifier: BSD-3-Clause
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_FREEBSD_ENABLED
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <aio.h>
#include <sys/event.h>
#include <xnvme_queue.h>
#include <xnvme_be_freebsd.h>
#include <xnvme_dev.h>

struct xnvme_queue_kqueue {
	struct xnvme_queue_base base;

	int kq;
	struct kevent *aio_events;

	TAILQ_HEAD(, kqueue_request) reqs_ready;
	TAILQ_HEAD(, kqueue_request) reqs_outstanding;
	struct kqueue_request *reqs_storage;

	uint8_t poll_io;

	uint8_t rsvd[175];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_kqueue) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

struct kqueue_request {
	struct xnvme_cmd_ctx *ctx;
	struct aiocb aiocb;
	TAILQ_ENTRY(kqueue_request) link;
};

int
xnvme_be_freebsd_kqueue_term(struct xnvme_queue *q)
{
	struct xnvme_queue_kqueue *queue = (void *)q;

	if (!queue) {
		XNVME_DEBUG("FAILED: queue: %p", (void *)queue);
		return -EINVAL;
	}

	close(queue->kq);
	free(queue->aio_events);
	free(queue->reqs_storage);

	return 0;
}

int
xnvme_be_freebsd_kqueue_init(struct xnvme_queue *q, int opts)
{
	struct xnvme_queue_kqueue *queue = (void *)q;
	struct xnvme_be_freebsd_state *state = (void *)queue->base.dev->be.state;
	size_t queue_nbytes = queue->base.capacity * sizeof(struct kqueue_request);

	queue->poll_io = (opts & XNVME_QUEUE_IOPOLL) || state->poll_io;
	queue->aio_events = calloc(queue->base.capacity, sizeof(struct kevent));
	queue->reqs_storage = calloc(1, queue_nbytes);
	if (!queue->reqs_storage) {
		XNVME_DEBUG("FAILED: calloc(reqs_storage), err: %s", strerror(errno));
		return -errno;
	}

	TAILQ_INIT(&queue->reqs_ready);
	for (uint32_t i = 0; i < queue->base.capacity; i++) {
		TAILQ_INSERT_HEAD(&queue->reqs_ready, &queue->reqs_storage[i], link);
	}

	TAILQ_INIT(&queue->reqs_outstanding);

	queue->kq = kqueue();
	if (queue->kq < 0) {
		XNVME_DEBUG("FAILED: kqueue(), err: %s", strerror(errno));
		return -errno;
	}

	return 0;
}

int
xnvme_be_freebsd_kqueue_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_kqueue *queue = (void *)q;
	struct timespec timeout = {.tv_sec = 0, .tv_nsec = queue->poll_io ? 0 : 100000};
	struct kqueue_request *req;
	int completed;
	int err;

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;

	if (!queue->base.outstanding) {
		return 0;
	}

	completed = kevent(queue->kq, NULL, 0, queue->aio_events, max, &timeout);
	if (completed < 0) {
		XNVME_DEBUG("FAILED: completed: %d, errno: %d", completed, errno);
		return -errno;
	}

	for (int event = 0; event < completed; event++) {
		int res = 0;
		struct xnvme_cmd_ctx *ctx;
		struct kevent *ev = &queue->aio_events[event];

		req = (struct kqueue_request *)ev->udata;
		if (!req) {
			XNVME_DEBUG("-{[THIS SHOULD NOT HAPPEN]}-");
			XNVME_DEBUG("event->data is NULL! => NO REQ!");
			XNVME_DEBUG("unprocessed events might remain");

			queue->base.outstanding -= 1;

			return -EIO;
		}

		err = aio_error(&req->aiocb);
		res = aio_return(&req->aiocb);

		ctx = req->ctx;
		ctx->cpl.result = res;
		if (err || (res < 0)) {
			ctx->cpl.result = 0;
			// When aio_error() fails, we use 'err'
			// When aio_return() fails, we use 'errno'
			ctx->cpl.status.sc = err ? err : errno;
			ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
		}

		ctx->async.cb(ctx, ctx->async.cb_arg);
		queue->base.outstanding -= 1;

		// Prepare req for reuse
		memset(&req->aiocb, 0, sizeof(struct aiocb));
		req->ctx = NULL;

		TAILQ_REMOVE(&queue->reqs_outstanding, req, link);
		TAILQ_INSERT_TAIL(&queue->reqs_ready, req, link);

		req = TAILQ_FIRST(&queue->reqs_outstanding);
	}

	return completed;
}

/**
 * Execute a flush and complete the command-context inline
 *
 * The asynchronous aio_fsync() is not usable here: the FreeBSD kernel rejects it with ENOTSUP
 * on the NVMe namespace character-devices which this backend primarily drives, whereas the
 * synchronous fsync() is serviced, as demonstrated by the psync command-interface. Since the
 * flush thus never produces a kqueue-event, it neither consumes a request-object nor bumps
 * the outstanding-counter; the completion-callback is invoked before returning instead.
 *
 * Like a flush submitted to a NVMe controller, this provides no ordering-guarantee with
 * regards to the commands which are still outstanding on the queue.
 */
static int
kqueue_flush(struct xnvme_cmd_ctx *ctx, int fd)
{
	if (fsync(fd)) {
		XNVME_DEBUG("FAILED: fsync(), err: %d", errno);
		return -errno;
	}

	ctx->cpl.result = 0;
	ctx->cpl.status.val = 0;

	///< Nothing on the queue is touched past this point; the callback is free to re-enter
	ctx->async.cb(ctx, ctx->async.cb_arg);

	return 0;
}

int
xnvme_be_freebsd_kqueue_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			       void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_queue_kqueue *queue = (void *)ctx->async.queue;
	struct xnvme_be_freebsd_state *state = (void *)queue->base.dev->be.state;
	const uint64_t ssw = queue->base.dev->geo.ssw;
	struct kqueue_request *req;
	struct aiocb *aiocb;
	int err;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOTSUP;
	}

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		return kqueue_flush(ctx, state->fd.ns);

	default:
		break;
	}

	req = TAILQ_FIRST(&queue->reqs_ready);
	assert(req != NULL);

	req->ctx = ctx;
	aiocb = &req->aiocb;
	aiocb->aio_fildes = state->fd.ns;
	aiocb->aio_buf = dbuf;
	aiocb->aio_nbytes = dbuf_nbytes;
	aiocb->aio_sigevent.sigev_notify_kqueue = queue->kq;
	aiocb->aio_sigevent.sigev_value.sival_ptr = req;
	aiocb->aio_sigevent.sigev_notify = SIGEV_KEVENT;

	///< Literally convert the NVMe command / sqe memory to an aio-control-block
	///< NOTE: opcode-dispatch (io)
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		aiocb->aio_offset = ctx->cmd.nvm.slba << ssw;
		err = aio_write(aiocb);
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		aiocb->aio_offset = ctx->cmd.nvm.slba << ssw;
		err = aio_read(aiocb);
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		aiocb->aio_offset = ctx->cmd.nvm.slba;
		err = aio_write(aiocb);
		break;

	case XNVME_SPEC_FS_OPC_READ:
		aiocb->aio_offset = ctx->cmd.nvm.slba;
		err = aio_read(aiocb);
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	if (err) {
		XNVME_DEBUG("FAILED: {aio_write(),aio_read()}: err: %d", errno)
		return -errno;
	}

	TAILQ_REMOVE(&queue->reqs_ready, req, link);
	TAILQ_INSERT_TAIL(&queue->reqs_outstanding, req, link);

	queue->base.outstanding += 1;

	return err;
}

int
xnvme_be_freebsd_kqueue_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
				size_t XNVME_UNUSED(dvec_nbytes), void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_queue_kqueue *queue = (void *)ctx->async.queue;
	struct xnvme_be_freebsd_state *state = (void *)queue->base.dev->be.state;
	const uint64_t ssw = queue->base.dev->geo.ssw;
	struct kqueue_request *req;
	struct aiocb *aiocb;
	int err;

	if (queue->base.outstanding == queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOTSUP;
	}

	///< A flush has no payload, thus the vectored path defers to the same helper
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		return kqueue_flush(ctx, state->fd.ns);

	default:
		break;
	}

	req = TAILQ_FIRST(&queue->reqs_ready);
	assert(req != NULL);

	req->ctx = ctx;
	aiocb = &req->aiocb;
	aiocb->aio_fildes = state->fd.ns;
	aiocb->aio_iov = dvec;
	aiocb->aio_iovcnt = dvec_cnt;
	aiocb->aio_sigevent.sigev_notify_kqueue = queue->kq;
	aiocb->aio_sigevent.sigev_value.sival_ptr = req;
	aiocb->aio_sigevent.sigev_notify = SIGEV_KEVENT;

	///< Convert the NVMe command/sqe to an Linux aio io-control-block
	///< NOTE: opcode-dispatch (io)
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		aiocb->aio_offset = ctx->cmd.nvm.slba << ssw;
		err = aio_writev(aiocb);
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		aiocb->aio_offset = ctx->cmd.nvm.slba << ssw;
		err = aio_readv(aiocb);
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		aiocb->aio_offset = ctx->cmd.nvm.slba;
		err = aio_writev(aiocb);
		break;

	case XNVME_SPEC_FS_OPC_READ:
		aiocb->aio_offset = ctx->cmd.nvm.slba;
		err = aio_readv(aiocb);
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	if (err) {
		XNVME_DEBUG("FAILED: {aio_writev(),aio_readv()}: err: %d", errno)
		return -errno;
	}

	TAILQ_REMOVE(&queue->reqs_ready, req, link);
	TAILQ_INSERT_TAIL(&queue->reqs_outstanding, req, link);

	queue->base.outstanding += 1;

	return err;
}
#endif

struct xnvme_be_async g_xnvme_be_freebsd_async = {
	.id = "kqueue",
#ifdef XNVME_PLATFORM_FREEBSD_ENABLED
	.cmd_io = xnvme_be_freebsd_kqueue_cmd_io,
	.cmd_iov = xnvme_be_freebsd_kqueue_cmd_iov,
	.poke = xnvme_be_freebsd_kqueue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_freebsd_kqueue_init,
	.term = xnvme_be_freebsd_kqueue_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
#endif
};
