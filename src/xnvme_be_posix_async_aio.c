// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_POSIX_ENABLED
#include <aio.h>
#include <errno.h>
#include <xnvme_be_posix.h>
#include <xnvme_queue.h>
#include <xnvme_dev.h>

struct xnvme_queue_posix {
	struct xnvme_queue_base base;
	struct aiocb aiocb;

	uint8_t rsvd[232];
};

int
_posix_async_aio_term(struct xnvme_queue *XNVME_UNUSED(q))
{
	return -ENOSYS;
}

int
_posix_async_aio_init(struct xnvme_queue *XNVME_UNUSED(q), int XNVME_UNUSED(opts))
{
	return -ENOSYS;
}

int
_posix_async_aio_poke(struct xnvme_queue *XNVME_UNUSED(q), uint32_t XNVME_UNUSED(max))
{
	return -ENOSYS;
}

int
_posix_async_aio_wait(struct xnvme_queue *XNVME_UNUSED(queue))
{
	return -ENOSYS;
}

int
_posix_async_aio_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			size_t mbuf_nbytes)
{
	struct xnvme_queue_posix *queue = (void *)ctx->async.queue;
	struct xnvme_be_posix_state *state = (void *)queue->base.dev->be.state;
	const uint64_t ssw = (queue->base.dev->dtype == XNVME_DEV_TYPE_FS_FILE) ? \
			     0 : queue->base.dev->ssw;

	struct aiocb aiocb = { 0 };
	int err;

	if (queue->base.outstanding == queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}
	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOSYS;
	}

	aiocb.aio_fildes = state->fd;
	aiocb.aio_offset = ctx->cmd.nvm.slba << ssw;
	aiocb.aio_buf = dbuf;
	aiocb.aio_nbytes = dbuf_nbytes;

	///< Literally convert the NVMe command / sqe memory to an io-control-block
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		err = aio_write(&aiocb);
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		err = aio_read(&aiocb);
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	XNVME_DEBUG("FAILED: io_submit(), err: %d", err);

	return err;
}
#endif

struct xnvme_be_async g_xnvme_be_posix_async_aio = {
	.id = "posix",
#ifdef XNVME_BE_POSIX_ENABLED
	.cmd_io = _posix_async_aio_cmd_io,
	.poke = _posix_async_aio_poke,
	.wait = _posix_async_aio_wait,
	.init = _posix_async_aio_init,
	.term = _posix_async_aio_term,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
#endif
};
