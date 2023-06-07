// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_CBI_ASYNC_POSIX_ENABLED
#include <inttypes.h>
#include <errno.h>
#include <aio.h>
#include <xnvme_queue.h>
#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>

struct posix_queue {
	struct xnvme_queue_base base;

	TAILQ_HEAD(, posix_request) reqs_ready;
	TAILQ_HEAD(, posix_request) reqs_outstanding;
	struct posix_request *reqs_storage;

	uint8_t rsvd[188];
};
XNVME_STATIC_ASSERT(sizeof(struct posix_queue) == XNVME_BE_QUEUE_STATE_NBYTES, "Incorrect size")

struct posix_request {
	struct xnvme_cmd_ctx *ctx;
	struct aiocb aiocb;
	TAILQ_ENTRY(posix_request) link;
};

static int
posix_term(struct xnvme_queue *q)
{
	struct posix_queue *queue = (void *)q;

	free(queue->reqs_storage);

	return 0;
}

static int
posix_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	struct posix_queue *queue = (void *)q;
	size_t queue_nbytes = queue->base.capacity * sizeof(struct posix_request);

	queue->reqs_storage = calloc(1, queue_nbytes);
	if (!queue->reqs_storage) {
		XNVME_DEBUG("FAILED: calloc(reqs_ready), err: %s", strerror(errno));
		return -errno;
	}
	TAILQ_INIT(&queue->reqs_ready);
	for (uint32_t i = 0; i < queue->base.capacity; i++) {
		TAILQ_INSERT_HEAD(&queue->reqs_ready, &queue->reqs_storage[i], link);
	}

	TAILQ_INIT(&queue->reqs_outstanding);

	return 0;
}

static int
posix_poke(struct xnvme_queue *q, uint32_t max)
{
	struct posix_queue *queue = (void *)q;
	struct posix_request *req;
	struct xnvme_cmd_ctx *ctx;
	size_t completed = 0;

	max = max ? max : queue->base.outstanding;
	max = XNVME_MIN(max, queue->base.outstanding);

	if (!queue->base.outstanding) {
		return 0;
	}

	req = TAILQ_FIRST(&queue->reqs_outstanding);
	assert(req != NULL);

	while (req != NULL && completed < max) {
		ssize_t res = 0;
		int err;

		err = aio_error(&req->aiocb);
		switch (err) {
		case 0:
			res = aio_return(&req->aiocb);
			break;

		case EINPROGRESS:
			req = TAILQ_NEXT(req, link);
			continue;

		case ECANCELED: // Canceled or error, do not grab return-value
		default:
			break;
		}

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

		completed += 1;
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

static int
posix_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
	     size_t mbuf_nbytes)
{
	struct posix_queue *queue = (void *)ctx->async.queue;

	struct xnvme_be_cbi_state *state = (void *)queue->base.dev->be.state;
	const uint64_t ssw = queue->base.dev->geo.ssw;
	struct posix_request *req;
	struct aiocb *aiocb;
	int err;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOSYS;
	}

	req = TAILQ_FIRST(&queue->reqs_ready);
	assert(req != NULL);

	req->ctx = ctx;
	aiocb = &req->aiocb;
	aiocb->aio_fildes = state->fd;
	aiocb->aio_buf = dbuf;
	aiocb->aio_nbytes = dbuf_nbytes;
	aiocb->aio_sigevent.sigev_notify = SIGEV_NONE;

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

	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		// TODO: should this be handled by calling aio_fsync()?
		// err = aio_fsync(_, &aiocb);

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
#endif

struct xnvme_be_async g_xnvme_be_cbi_async_posix = {
	.id = "posix",
#ifdef XNVME_BE_CBI_ASYNC_POSIX_ENABLED
	.cmd_io = posix_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = posix_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = posix_init,
	.term = posix_term,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
#endif
};
