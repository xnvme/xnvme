// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_LIBURING_ENABLED
#include <pthread.h>
#include <errno.h>
#include <liburing.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <xnvme_queue.h>
#include <xnvme_dev.h>
#include <xnvme_be_linux_liburing.h>
#include <xnvme_be_linux.h>

#ifndef IORING_SETUP_SINGLE_ISSUER
#define IORING_SETUP_SINGLE_ISSUER (1U << 12)
#endif

static struct sqpoll_wq {
	pthread_mutex_t mutex;
	struct io_uring ring;
	bool is_initialized;
	int refcount;
} g_sqpoll_wq = {
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.ring = {0},
	.is_initialized = false,
	.refcount = 0,
};

static int
_init_retry(unsigned entries, struct io_uring *ring, struct io_uring_params *p)
{
	int err;

retry:
	err = io_uring_queue_init_params(entries, ring, p);
	if (err) {
		if (err == -EINVAL && p->flags & IORING_SETUP_SINGLE_ISSUER) {
			p->flags &= ~IORING_SETUP_SINGLE_ISSUER;
			XNVME_DEBUG("FAILED: io_uring_queue_init_params(), retry(!SINGLE_ISSUER)");
			goto retry;
		}

		XNVME_DEBUG("FAILED: io_uring_queue_init(), err: %d", err);
		return err;
	}

	return 0;
}

int
xnvme_be_linux_liburing_init(struct xnvme_queue *q, int opts)
{
	struct xnvme_queue_liburing *queue = (void *)q;
	struct xnvme_be_linux_state *state = (void *)queue->base.dev->be.state;
	struct io_uring_params ring_params = {0};
	int err = 0;

	if ((opts & XNVME_QUEUE_SQPOLL) || (state->poll_sq)) {
		queue->poll_sq = 1;
	}
	if ((opts & XNVME_QUEUE_IOPOLL) || (state->poll_io)) {
		queue->poll_io = 1;
	}

	XNVME_DEBUG("queue->poll_sq: %d", queue->poll_sq);
	XNVME_DEBUG("queue->poll_io: %d", queue->poll_io);

	err = pthread_mutex_lock(&g_sqpoll_wq.mutex);
	if (err) {
		XNVME_DEBUG("FAILED: lock(g_sqpoll_wq.mutex), err: %d", err);
		return -err;
	}

	queue->efd = -1;

	queue->batching = 1;
	if (getenv("XNVME_QUEUE_BATCHING_OFF")) {
		queue->batching = 0;
	}

	//
	// Ring-initialization
	//
	if (queue->poll_sq) {
		char *env;

		if (!((env = getenv("XNVME_QUEUE_SQPOLL_AWQ")) && atoi(env) == 0)) {
			if (!g_sqpoll_wq.is_initialized) {
				struct io_uring_params sqpoll_wq_params = {0};

				env = getenv("XNVME_QUEUE_SQPOLL_CPU");
				if (env) {
					sqpoll_wq_params.flags |= IORING_SETUP_SQ_AFF;
					sqpoll_wq_params.sq_thread_cpu = atoi(env);
				}
				sqpoll_wq_params.flags |= IORING_SETUP_SQPOLL;
				sqpoll_wq_params.flags |= IORING_SETUP_SINGLE_ISSUER;

				err = _init_retry(queue->base.capacity, &g_sqpoll_wq.ring,
						  &sqpoll_wq_params);
				if (err) {
					XNVME_DEBUG(
						"FAILED: "
						"io_uring_queue_init_params(g_sqpoll_wq.ring), "
						"err: %d",
						err);
					goto exit;
				}

				g_sqpoll_wq.is_initialized = true;
			}

			g_sqpoll_wq.refcount += 1;
			ring_params.wq_fd = g_sqpoll_wq.ring.ring_fd;
			ring_params.flags |= IORING_SETUP_ATTACH_WQ;
		}
		ring_params.flags |= IORING_SETUP_SQPOLL;
		ring_params.flags |= IORING_SETUP_SINGLE_ISSUER;
	}
	if (queue->poll_io) {
		ring_params.flags |= IORING_SETUP_IOPOLL;
	}

	if (opts & XNVME_QUEUE_IOU_BIGSQE) {
		ring_params.flags |= IORING_SETUP_SQE128;
		ring_params.flags |= IORING_SETUP_CQE32;
	}

	err = _init_retry(queue->base.capacity, &queue->ring, &ring_params);
	if (err) {
		XNVME_DEBUG("FAILED: _init_retry, err: %d", err);
		goto exit;
	}

	if (queue->poll_sq) {
		err = io_uring_register_files(&queue->ring, &(state->fd), 1);
		if (err) {
			XNVME_DEBUG("FAILED: io_uring_register_files, err: %d", err);
			goto exit;
		}
	}

exit:
	if (err && queue->poll_sq && g_sqpoll_wq.is_initialized && (!(--g_sqpoll_wq.refcount))) {
		io_uring_queue_exit(&g_sqpoll_wq.ring);
		g_sqpoll_wq.is_initialized = false;
	}
	if (pthread_mutex_unlock(&g_sqpoll_wq.mutex)) {
		XNVME_DEBUG("FAILED: unlock(g_sqpoll_wq.mutex)");
	}

	return err;
}

int
xnvme_be_linux_liburing_term(struct xnvme_queue *q)
{
	struct xnvme_queue_liburing *queue = (void *)q;
	int err;

	err = pthread_mutex_lock(&g_sqpoll_wq.mutex);
	if (err) {
		XNVME_DEBUG("FAILED: lock(g_sqpoll_wq.mutex), err: %d", err);
		return -err;
	}

	if (!queue) {
		XNVME_DEBUG("FAILED: queue: %p", (void *)queue);
		err = -EINVAL;
		goto exit;
	}
	if (queue->poll_sq) {
		io_uring_unregister_files(&queue->ring);
	}

	if (queue->efd != -1) {
		io_uring_unregister_eventfd(&queue->ring);
		close(queue->efd);
		queue->efd = -1;
	}

	io_uring_queue_exit(&queue->ring);

	if (queue->poll_sq && g_sqpoll_wq.is_initialized && (!(--g_sqpoll_wq.refcount))) {
		io_uring_queue_exit(&g_sqpoll_wq.ring);
		g_sqpoll_wq.is_initialized = false;
	}

exit:
	if (pthread_mutex_unlock(&g_sqpoll_wq.mutex)) {
		XNVME_DEBUG("FAILED: unlock(g_sqpoll_wq.mutex)");
	}

	return err;
}

int
xnvme_be_linux_liburing_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_liburing *queue = (void *)q;
	struct io_uring_cqe *cqe;
	struct xnvme_cmd_ctx *ctx;
	unsigned completed;
	int err;

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;

	if (queue->batching) {
		int err = io_uring_submit(&queue->ring);
		if (err < 0) {
			XNVME_DEBUG("io_uring_submit, err: %d", err);
			return err;
		}
	}

	completed = 0;
	for (uint32_t i = 0; i < max; i++) {
		err = io_uring_peek_cqe(&queue->ring, &cqe);
		if (err == -EAGAIN) {
			return completed;
		}

		ctx = io_uring_cqe_get_data(cqe);

#ifdef XNVME_DEBUG_ENABLED
		if (!ctx) {
			XNVME_DEBUG("-{[THIS SHOULD NOT HAPPEN]}-");
			XNVME_DEBUG("cqe->user_data is NULL! => NO REQ!");
			XNVME_DEBUG("cqe->res: %d", cqe->res);
			XNVME_DEBUG("cqe->flags: %u", cqe->flags);
			return -EIO;
		}
#endif

		ctx->cpl.result = cqe->res;
		if (cqe->res < 0) {
			ctx->cpl.result = 0;
			ctx->cpl.status.sc = -cqe->res;
			ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
		}

		queue->base.outstanding--;

		io_uring_cqe_seen(&queue->ring, cqe);

		ctx->async.cb(ctx, ctx->async.cb_arg);

		completed++;
	}

	return completed;
}

int
xnvme_be_linux_liburing_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			       void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_queue_liburing *queue = (void *)ctx->async.queue;
	struct xnvme_be_linux_state *state = (void *)queue->base.dev->be.state;
	uint64_t ssw = 0;
	struct io_uring_sqe *sqe = NULL;

	int opcode = IORING_OP_NOP;
	int err = 0;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOTSUP;
	}

	///< NOTE: opcode-dispatch (io)
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		ssw = queue->base.dev->geo.ssw;
	/* fall through */
	case XNVME_SPEC_FS_OPC_WRITE:
		opcode = IORING_OP_WRITE;
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		ssw = queue->base.dev->geo.ssw;
	/* fall through */
	case XNVME_SPEC_FS_OPC_READ:
		opcode = IORING_OP_READ;
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported opcode: %d for async", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	sqe = io_uring_get_sqe(&queue->ring);
	if (!sqe) {
		return -EAGAIN;
	}

	sqe->opcode = opcode;
	sqe->addr = (unsigned long)dbuf;
	sqe->len = dbuf_nbytes;
	sqe->off = ctx->cmd.nvm.slba << ssw;
	sqe->flags = queue->poll_sq ? IOSQE_FIXED_FILE : 0;
	sqe->ioprio = 0;
	// NOTE: we only ever register a single file, the raw device, so the
	// provided index will always be 0
	sqe->fd = queue->poll_sq ? 0 : state->fd;
	sqe->rw_flags = 0;
	sqe->user_data = (unsigned long)ctx;
	// sqe->__pad2[0] = sqe->__pad2[1] = sqe->__pad2[2] = 0;

	if (queue->batching) {
		goto exit;
	}

	err = io_uring_submit(&queue->ring);
	if (err < 0) {
		XNVME_DEBUG("io_uring_submit(%d), err: %d", ctx->cmd.common.opcode, err);
		return err;
	}

exit:
	queue->base.outstanding += 1;

	return 0;
}

int
xnvme_be_linux_liburing_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
				size_t XNVME_UNUSED(dvec_nbytes), void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_queue_liburing *queue = (void *)ctx->async.queue;
	struct xnvme_be_linux_state *state = (void *)queue->base.dev->be.state;
	uint64_t ssw = 0;
	struct io_uring_sqe *sqe = NULL;

	int fd;
	int err = 0;

	if (queue->base.outstanding == queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOTSUP;
	}

	sqe = io_uring_get_sqe(&queue->ring);
	if (!sqe) {
		return -EAGAIN;
	}

	sqe->flags = queue->poll_sq ? IOSQE_FIXED_FILE : 0;

	// NOTE: we only ever register a single file, the raw device, so the
	// provided index will always be 0
	fd = queue->poll_sq ? 0 : state->fd;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		ssw = queue->base.dev->geo.ssw;
	/* fall through */
	case XNVME_SPEC_FS_OPC_WRITE:
		io_uring_prep_writev(sqe, fd, dvec, dvec_cnt, ctx->cmd.nvm.slba << ssw);
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		ssw = queue->base.dev->geo.ssw;
	/* fall through */
	case XNVME_SPEC_FS_OPC_READ:
		io_uring_prep_readv(sqe, fd, dvec, dvec_cnt, ctx->cmd.nvm.slba << ssw);
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported opcode: %d for async", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	io_uring_sqe_set_data(sqe, ctx);

	if (queue->batching) {
		goto exit;
	}

	err = io_uring_submit(&queue->ring);
	if (err < 0) {
		XNVME_DEBUG("io_uring_submit(%d), err: %d", ctx->cmd.common.opcode, err);
		return err;
	}

exit:
	queue->base.outstanding += 1;

	return 0;
}

int
xnvme_be_linux_liburing_get_completion_fd(struct xnvme_queue *queue)
{
	struct xnvme_queue_liburing *q = (struct xnvme_queue_liburing *)queue;
	int efd, err;

	if (q->efd != -1) {
		return q->efd;
	}

	if (q->base.outstanding) {
		XNVME_DEBUG("FAILED: outstanding I/O found when getting completion_fd");
		return -EBUSY;
	}

	efd = eventfd(0, EFD_CLOEXEC);
	if (efd < 0) {
		XNVME_DEBUG("FAILED: failed to create eventfd");
		return -errno;
	}

	q->efd = efd;

	if (io_uring_register_eventfd(&q->ring, efd)) {
		XNVME_DEBUG("FAILED: failed to register eventfd");
		close(efd);
		return -errno;
	}

	q->batching = 0;
	XNVME_DEBUG("Completion FD enabled, submission on poke (batching) disabled");

	return efd;
}

#endif

struct xnvme_be_async g_xnvme_be_linux_async_liburing = {
	.id = "io_uring",
#ifdef XNVME_BE_LINUX_LIBURING_ENABLED
	.cmd_io = xnvme_be_linux_liburing_cmd_io,
	.cmd_iov = xnvme_be_linux_liburing_cmd_iov,
	.poke = xnvme_be_linux_liburing_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_linux_liburing_init,
	.term = xnvme_be_linux_liburing_term,
	.get_completion_fd = xnvme_be_linux_liburing_get_completion_fd,
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
