// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifdef XNVME_BE_LINUX_IOU_ENABLED
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
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
#include <liburing.h>

#include <xnvme_queue.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_iou.h>
#include <xnvme_dev.h>

// TODO: replace this with liburing 0.7 barriers
#define _linux_iou_barrier()  __asm__ __volatile__("":::"memory")

static int g_linux_iou_opcodes[] = {
	IORING_OP_READV,
	IORING_OP_WRITEV,
	IORING_OP_READ_FIXED,
	IORING_OP_WRITE_FIXED,
	IORING_OP_READ,
	IORING_OP_WRITE,
};
int g_linux_iou_nopcodes = sizeof g_linux_iou_opcodes / sizeof(*g_linux_iou_opcodes);

/**
 * Check whether the Kernel supports the io_uring features used by xNVMe
 *
 * @return On success, 0 is returned. On error, negative errno is returned,
 * specifically -ENOSYS;
 */
int
_linux_iou_supported(struct xnvme_dev *XNVME_UNUSED(dev), uint32_t XNVME_UNUSED(opts))
{
	struct io_uring_probe *probe;
	int err = 0;

	probe = io_uring_get_probe();
	if (!probe) {
		XNVME_DEBUG("FAILED: io_uring_get_probe()");
		err = -ENOSYS;
		goto exit;
	}

	for (int i = 0; i < g_linux_iou_nopcodes; ++i) {
		if (!io_uring_opcode_supported(probe, g_linux_iou_opcodes[i])) {
			err = -ENOSYS;
			XNVME_DEBUG("FAILED: Kernel does not support opc: %d",
				    g_linux_iou_opcodes[i]);
			goto exit;
		}
	}

exit:
	free(probe);

	return err ? 0 : 1;
}

int
_linux_iou_init(struct xnvme_queue *queue, int opts)
{
	struct xnvme_be_linux_state *state = (void *)queue->base.dev->be.state;
	struct xnvme_queue_iou *actx = (void *)queue;
	int err = 0;
	int iou_flags = 0;

	if ((opts & XNVME_QUEUE_SQPOLL) || (state->poll_sq)) {
		actx->poll_sq = 1;
	}
	if ((opts & XNVME_QUEUE_IOPOLL) || (state->poll_io)) {
		actx->poll_io = 1;
	}

	// NOTE: Disabling IOPOLL, to avoid lock-up, until fixed in `_poke`
	if (actx->poll_io) {
		printf("ENOSYS: IORING_SETUP_IOPOLL\n");
		actx->poll_io = 0;
	}
	XNVME_DEBUG("actx->poll_sq: %d", actx->poll_sq);
	XNVME_DEBUG("actx->poll_io: %d", actx->poll_io);

	//
	// Ring-initialization
	//
	if (actx->poll_sq) {
		iou_flags |= IORING_SETUP_SQPOLL;
	}
	if (actx->poll_io) {
		iou_flags |= IORING_SETUP_IOPOLL;
	}

	err = io_uring_queue_init(queue->base.depth, &actx->ring, iou_flags);
	if (err) {
		XNVME_DEBUG("FAILED: io_uring_queue_init(), err: %d", err);
		return err;
	}

	if (actx->poll_sq) {
		io_uring_register_files(&actx->ring, &(state->fd), 1);
	}

	return 0;
}

int
_linux_iou_term(struct xnvme_queue *queue)
{
	struct xnvme_queue_iou *actx = (void *)queue;

	if (!queue) {
		XNVME_DEBUG("FAILED: queue: %p", (void *)queue);
		return -EINVAL;
	}

	actx = (void *)queue;

	io_uring_unregister_files(&actx->ring);
	io_uring_queue_exit(&actx->ring);

	return 0;
}

int
_linux_iou_poke(struct xnvme_queue *queue, uint32_t max)
{
	struct xnvme_queue_iou *actx = (void *)queue;
	struct io_uring_cq *ring = &actx->ring.cq;
	unsigned cq_ring_mask = *ring->kring_mask;
	unsigned completed = 0;
	unsigned head;

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;

	head = *ring->khead;
	do {
		struct io_uring_cqe *cqe;
		struct xnvme_cmd_ctx *ctx;

		_linux_iou_barrier();
		if (head == *ring->ktail) {
			break;
		}
		cqe = &ring->cqes[head & cq_ring_mask];

		ctx = (struct xnvme_cmd_ctx *)(uintptr_t) cqe->user_data;
		if (!ctx) {
			XNVME_DEBUG("-{[THIS SHOULD NOT HAPPEN]}-");
			XNVME_DEBUG("cqe->user_data is NULL! => NO REQ!");
			XNVME_DEBUG("cqe->res: %d", cqe->res);
			XNVME_DEBUG("cqe->flags: %u", cqe->flags);

			io_uring_cqe_seen(&actx->ring, cqe);
			queue->base.outstanding -= 1;

			return -EIO;
		}

		// Map cqe-result to cmd_ctx-completion
		ctx->cpl.status.sc = cqe->res;

		ctx->async.cb(ctx, ctx->async.cb_arg);

		++completed;
		++head;
	} while (completed < max);

	queue->base.outstanding -= completed;
	*ring->khead = head;

	_linux_iou_barrier();

	return completed;
}

int
_linux_iou_wait(struct xnvme_queue *queue)
{
	int acc = 0;

	while (queue->base.outstanding) {
		struct timespec ts1 = {.tv_sec = 0, .tv_nsec = 1000};
		int err;

		err = _linux_iou_poke(queue, 0);
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

int
_linux_iou_cmd_io(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
		  void *dbuf, size_t dbuf_nbytes, void *mbuf,
		  size_t mbuf_nbytes, int XNVME_UNUSED(opts),
		  struct xnvme_cmd_ctx *ctx)
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;
	struct xnvme_queue_iou *actx = (void *)ctx->async.queue;
	struct io_uring_sqe *sqe = NULL;
	int opcode;
	int err = 0;

	switch (cmd->common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		opcode = IORING_OP_WRITE;
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		opcode = IORING_OP_READ;
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported opcode: %d for async",
			    cmd->common.opcode);
		return -ENOSYS;
	}

	if (ctx->async.queue->base.outstanding == ctx->async.queue->base.depth) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}
	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOSYS;
	}

	sqe = io_uring_get_sqe(&actx->ring);
	if (!sqe) {
		return -EAGAIN;
	}

	sqe->opcode = opcode;
	sqe->addr = (unsigned long) dbuf;
	sqe->len = dbuf_nbytes;
	sqe->off = cmd->nvm.slba << dev->ssw;
	sqe->flags = actx->poll_sq ? IOSQE_FIXED_FILE : 0;
	sqe->ioprio = 0;
	// NOTE: we only ever register a single file, the raw device, so the
	// provided index will always be 0
	sqe->fd = actx->poll_sq ? 0 : state->fd;
	sqe->rw_flags = 0;
	sqe->user_data = (unsigned long)ctx;
	sqe->__pad2[0] = sqe->__pad2[1] = sqe->__pad2[2] = 0;

	err = io_uring_submit(&actx->ring);
	if (err < 0) {
		XNVME_DEBUG("io_uring_submit(%d), err: %d", cmd->common.opcode,
			    err);
		return err;
	}

	ctx->async.queue->base.outstanding += 1;

	return 0;
}

struct xnvme_be_async g_linux_iou = {
	.id = "iou",
#ifdef XNVME_BE_LINUX_IOU_ENABLED
	.enabled = 1,
	.cmd_io = _linux_iou_cmd_io,
	.poke = _linux_iou_poke,
	.wait = _linux_iou_wait,
	.init = _linux_iou_init,
	.term = _linux_iou_term,
	.supported = _linux_iou_supported,
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
