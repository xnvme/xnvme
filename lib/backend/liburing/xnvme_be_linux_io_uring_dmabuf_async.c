// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>

#ifdef XNVME_BE_LINUX_IO_URING_DMABUF_ENABLED
#include <errno.h>
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_liburing.h>
#include <xnvme_be_linux_io_uring_dmabuf.h>
#include <xnvme_be_nosys.h>

static int
cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_queue_liburing *queue = (void *)ctx->async.queue;
	uint64_t offset;
	uint16_t index;
	int err;

	err = xnvme_io_uring_dmabuf_lookup(queue->dmabuf, dbuf, dbuf_nbytes, &index, &offset);
	if (err == -ENOENT) {
		// Not a mapped dma-buf, so regular io_uring IO
		return xnvme_be_linux_liburing_cmd_io(ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes);
	}
	if (err) {
		return err;
	}

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOTSUP;
	}

	return xnvme_be_linux_io_uring_cmd_io_fixed(ctx, index, offset, dbuf_nbytes);
}

static int
queue_init(struct xnvme_queue *q, int opts)
{
	struct xnvme_be_linux_state *state = (void *)q->base.dev->be.state;
	int err;

	if ((opts & XNVME_QUEUE_IOPOLL) || (state->poll_io)) {
		XNVME_DEBUG("FAILED: dma-buf buffers are not supported on an IOPOLL queue");
		return -ENOTSUP;
	}

	err = xnvme_be_linux_liburing_init(q, opts);
	if (err) {
		return err;
	}

	err = xnvme_io_uring_dmabuf_attach((void *)q);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_io_uring_dmabuf_attach(), err: %d", err);
		xnvme_be_linux_liburing_term(q);
	}

	return err;
}

static int
queue_term(struct xnvme_queue *q)
{
	if (q) {
		xnvme_io_uring_dmabuf_detach((void *)q);
	}

	return xnvme_be_linux_liburing_term(q);
}

struct xnvme_be_async g_xnvme_be_linux_async_io_uring_dmabuf = {
	.id = "io_uring_dmabuf",
	.cmd_io = cmd_io,
	.cmd_iov = xnvme_be_linux_liburing_cmd_iov,
	.poke = xnvme_be_linux_liburing_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = queue_init,
	.term = queue_term,
	.get_completion_fd = xnvme_be_linux_liburing_get_completion_fd,
};
#endif
