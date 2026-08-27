// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "io.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <liburing.h>
#include <libxnvme.h>

#ifndef IORING_SETUP_SINGLE_ISSUER
#define IORING_SETUP_SINGLE_ISSUER (1U << 12)
#endif
#ifndef IORING_SETUP_DEFER_TASKRUN
#define IORING_SETUP_DEFER_TASKRUN (1U << 13)
#endif

/*
 * io_uring CQE user_data convention:
 *   bit 63 clear -> ublk tag CQE; user_data is the tag (< depth)
 *   bit 63 set   -> cross-queue MSG_RING; encoded as:
 *                   bits 62-60: type (QUBLK_MSG_*)
 *                   bits 31-16: originating q_id
 *                   bits 15-0 : originating tag
 * The MSG_RING payload uses sqe->off (delivered as target cqe->user_data) for
 * the encoded id and sqe->len (delivered as target cqe->res) for status.
 */
#define QUBLK_UD_MSG_BIT (1ULL << 63)
#define QUBLK_MSG_TYPE_SHIFT 60
#define QUBLK_MSG_TYPE_MASK 0x7ULL
#define QUBLK_MSG_QID_SHIFT 16
#define QUBLK_MSG_QID_MASK 0xFFFFULL
#define QUBLK_MSG_TAG_MASK 0xFFFFULL

enum {
	QUBLK_MSG_SOURCE = 0,     /* source-side CQE for our outgoing MSG_RING */
	QUBLK_MSG_DO_FLUSH = 1,   /* recipient: issue NVMe FLUSH for (orig_q, orig_tag) */
	QUBLK_MSG_FLUSH_DONE = 2, /* originator: a remote FLUSH completed (res = status) */
};

static inline uint64_t
ud_msg(unsigned type, uint16_t qid, uint16_t tag)
{
	return QUBLK_UD_MSG_BIT | ((uint64_t)type << QUBLK_MSG_TYPE_SHIFT) |
	       ((uint64_t)qid << QUBLK_MSG_QID_SHIFT) | (uint64_t)tag;
}

static size_t
page_round_up(size_t v)
{
	size_t pg = (size_t)sysconf(_SC_PAGESIZE);
	return (v + pg - 1) & ~(pg - 1);
}

/*
 * ublk_drv maps each queue's iod array at a fixed stride of
 * round_up(UBLK_MAX_QUEUE_DEPTH * sizeof(ublksrv_io_desc), PAGE_SIZE) from
 * UBLKSRV_CMD_BUF_OFFSET. The stride is the kernel max, not our queue depth.
 */
static size_t
iod_stride(void)
{
	return page_round_up((size_t)UBLK_MAX_QUEUE_DEPTH * sizeof(struct ublksrv_io_desc));
}

/*
 * Per iteration of io_loop we may queue, between submits: one
 * COMMIT_AND_FETCH per local tag, (nr_queues-1) outgoing MSG_RINGs per local
 * barrier op, and (nr_queues-1) FLUSH_DONE replies per remote-served flush.
 * Worst case is depth * (2 * nr_queues - 1) SQEs.
 */
static unsigned
ring_entries_for(const struct qublk_queue *q)
{
	unsigned nq = q->dev->nqueues;
	unsigned entries = (unsigned)q->depth * (2 * nq - 1);

	if (entries < (unsigned)q->depth) {
		entries = q->depth;
	}
	return entries;
}

static int
init_ring(struct qublk_queue *q)
{
	struct io_uring_params p = {0};
	unsigned entries = ring_entries_for(q);
	int rc;

	p.flags = IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;
	rc = io_uring_queue_init_params(entries, &q->ring, &p);
	if (rc == -EINVAL) {
		memset(&p, 0, sizeof(p));
		rc = io_uring_queue_init_params(entries, &q->ring, &p);
	}
	return rc;
}

static int
rflush_pool_init(struct qublk_dev *dev, struct qublk_queue *q)
{
	uint32_t n;

	if (dev->nqueues <= 1) {
		return 0;
	}
	n = (uint32_t)(dev->nqueues - 1) * q->depth;
	q->rflush_pool = calloc(n, sizeof(*q->rflush_pool));
	if (!q->rflush_pool) {
		return -ENOMEM;
	}
	q->rflush_nr = n;
	for (uint32_t i = 0; i < n; i++) {
		q->rflush_pool[i].remote_q = q;
		q->rflush_pool[i].next = q->rflush_free;
		q->rflush_free = &q->rflush_pool[i];
	}
	return 0;
}

static void
rflush_pool_fini(struct qublk_queue *q)
{
	free(q->rflush_pool);
	q->rflush_pool = NULL;
	q->rflush_free = NULL;
	q->rflush_nr = 0;
}

static struct qublk_remote_flush *
rflush_alloc(struct qublk_queue *q)
{
	struct qublk_remote_flush *rf = q->rflush_free;

	if (rf) {
		q->rflush_free = rf->next;
		rf->next = NULL;
	}
	return rf;
}

static void
rflush_release(struct qublk_queue *q, struct qublk_remote_flush *rf)
{
	rf->next = q->rflush_free;
	q->rflush_free = rf;
}

static void
prep_io_uring_cmd(struct io_uring_sqe *sqe, uint32_t cmd_op, const struct ublksrv_io_cmd *cmd,
		  uint64_t user_data)
{
	io_uring_prep_rw(IORING_OP_URING_CMD, sqe, 0, NULL, 0, 0);
	sqe->flags = IOSQE_FIXED_FILE;
	sqe->cmd_op = cmd_op;
	memcpy(sqe->cmd, cmd, sizeof(*cmd));
	sqe->user_data = user_data;
}

static int
submit_fetch(struct qublk_queue *q, struct qublk_io *io)
{
	struct io_uring_sqe *sqe;
	struct ublksrv_io_cmd cmd = {
		.q_id = q->q_id,
		.tag = io->tag,
		.result = -1,
		.addr = (uint64_t)(uintptr_t)io->buf,
	};

	sqe = io_uring_get_sqe(&q->ring);
	if (!sqe) {
		return -EAGAIN;
	}
	prep_io_uring_cmd(sqe, UBLK_U_IO_FETCH_REQ, &cmd, io->tag);
	return 0;
}

static int
submit_commit_and_fetch(struct qublk_queue *q, struct qublk_io *io, int result)
{
	struct io_uring_sqe *sqe;
	struct ublksrv_io_cmd cmd = {
		.q_id = q->q_id,
		.tag = io->tag,
		.result = result,
		.addr = (uint64_t)(uintptr_t)io->buf,
	};

	sqe = io_uring_get_sqe(&q->ring);
	if (!sqe) {
		return -EAGAIN;
	}
	prep_io_uring_cmd(sqe, UBLK_U_IO_COMMIT_AND_FETCH_REQ, &cmd, io->tag);
	return 0;
}

static void
on_xnvme_complete(struct xnvme_cmd_ctx *ctx, void *opaque)
{
	struct qublk_io *io = opaque;
	struct qublk_queue *q = io->q;
	uint8_t op;
	int result;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		result = -EIO;
	} else {
		op = ublksrv_get_op(io->iod);
		if (op == UBLK_IO_OP_READ || op == UBLK_IO_OP_WRITE) {
			result = (int)(io->iod->nr_sectors << 9);
		} else {
			result = 0;
		}
	}

	xnvme_queue_put_cmd_ctx(q->xq, ctx);
	submit_commit_and_fetch(q, io, result);
}

static void
barrier_complete_iod(struct qublk_queue *q, struct qublk_io *io)
{
	int result;

	if (io->barrier_err) {
		result = io->barrier_err;
	} else if (ublksrv_get_op(io->iod) == UBLK_IO_OP_WRITE) {
		result = (int)(io->iod->nr_sectors << 9);
	} else {
		result = 0;
	}
	submit_commit_and_fetch(q, io, result);
}

static void
on_local_barrier_complete(struct xnvme_cmd_ctx *ctx, void *opaque)
{
	struct qublk_io *io = opaque;
	struct qublk_queue *q = io->q;
	int status = xnvme_cmd_ctx_cpl_status(ctx) ? -EIO : 0;

	xnvme_queue_put_cmd_ctx(q->xq, ctx);
	if (status && io->barrier_err == 0) {
		io->barrier_err = status;
	}
	if (--io->barrier_outstanding == 0) {
		barrier_complete_iod(q, io);
	}
}

static void
msg_post_flush_done(struct qublk_queue *q, uint16_t orig_qid, uint16_t orig_tag, int status)
{
	struct qublk_queue *orig = &q->dev->queues[orig_qid];
	struct io_uring_sqe *sqe;

	sqe = io_uring_get_sqe(&q->ring);
	if (!sqe) {
		fprintf(stderr, "qublk: q%d no SQE for FLUSH_DONE reply\n", q->q_id);
		q->dev->stop = 1;
		return;
	}
	io_uring_prep_msg_ring(sqe, orig->ring.ring_fd, status,
			       ud_msg(QUBLK_MSG_FLUSH_DONE, 0, orig_tag), 0);
	sqe->user_data = ud_msg(QUBLK_MSG_SOURCE, 0, 0);
}

static void
on_remote_flush_complete(struct xnvme_cmd_ctx *ctx, void *opaque)
{
	struct qublk_remote_flush *rf = opaque;
	struct qublk_queue *q = rf->remote_q;
	uint16_t orig_qid = rf->orig_q_id;
	uint16_t orig_tag = rf->orig_tag;
	int status = xnvme_cmd_ctx_cpl_status(ctx) ? -EIO : 0;

	xnvme_queue_put_cmd_ctx(q->xq, ctx);
	rflush_release(q, rf);
	msg_post_flush_done(q, orig_qid, orig_tag, status);
}

static void
handle_msg_do_flush(struct qublk_queue *q, uint16_t orig_qid, uint16_t orig_tag)
{
	struct qublk_dev *dev = q->dev;
	uint32_t nsid = xnvme_dev_get_nsid(dev->xdev);
	struct qublk_remote_flush *rf;
	struct xnvme_cmd_ctx *ctx;
	int rc;

	rf = rflush_alloc(q);
	if (!rf) {
		fprintf(stderr, "qublk: q%d remote-flush pool exhausted\n", q->q_id);
		msg_post_flush_done(q, orig_qid, orig_tag, -ENOMEM);
		return;
	}
	rf->orig_q_id = orig_qid;
	rf->orig_tag = orig_tag;

	for (;;) {
		ctx = xnvme_queue_get_cmd_ctx(q->xq);
		if (ctx) {
			break;
		}
		if (dev->stop) {
			rflush_release(q, rf);
			msg_post_flush_done(q, orig_qid, orig_tag, -ENODEV);
			return;
		}
		xnvme_queue_poke(q->xq, 0);
	}
	memset(&ctx->cmd, 0, sizeof(ctx->cmd));
	xnvme_cmd_ctx_set_cb(ctx, on_remote_flush_complete, rf);
	xnvme_prep_nvm(ctx, XNVME_SPEC_NVM_OPC_FLUSH, nsid, 0, 0);
	rc = xnvme_cmd_pass(ctx, NULL, 0, NULL, 0);
	while (rc == -EBUSY || rc == -EAGAIN) {
		xnvme_queue_poke(q->xq, 0);
		rc = xnvme_cmd_pass(ctx, NULL, 0, NULL, 0);
	}
	if (rc < 0) {
		xnvme_queue_put_cmd_ctx(q->xq, ctx);
		rflush_release(q, rf);
		msg_post_flush_done(q, orig_qid, orig_tag, rc);
	}
}

static void
handle_msg_flush_done(struct qublk_queue *q, uint16_t orig_tag, int status)
{
	struct qublk_io *io;

	if (orig_tag >= q->depth) {
		fprintf(stderr, "qublk: q%d FLUSH_DONE: bogus tag %u\n", q->q_id, orig_tag);
		return;
	}
	io = &q->ios[orig_tag];
	if (status && io->barrier_err == 0) {
		io->barrier_err = status;
	}
	if (--io->barrier_outstanding == 0) {
		barrier_complete_iod(q, io);
	}
}

static int
dispatch_barrier(struct qublk_queue *q, struct qublk_io *io, uint8_t op)
{
	struct qublk_dev *dev = q->dev;
	const struct ublksrv_io_desc *iod = io->iod;
	uint32_t nsid = xnvme_dev_get_nsid(dev->xdev);
	uint8_t lba_shift = dev->lba_shift;
	struct xnvme_cmd_ctx *ctx;
	int rc;

	ctx = xnvme_queue_get_cmd_ctx(q->xq);
	if (!ctx) {
		return -EBUSY;
	}
	memset(&ctx->cmd, 0, sizeof(ctx->cmd));
	xnvme_cmd_ctx_set_cb(ctx, on_local_barrier_complete, io);

	if (op == UBLK_IO_OP_FLUSH) {
		xnvme_prep_nvm(ctx, XNVME_SPEC_NVM_OPC_FLUSH, nsid, 0, 0);
		rc = xnvme_cmd_pass(ctx, NULL, 0, NULL, 0);
	} else {
		uint64_t slba = iod->start_sector >> (lba_shift - 9);
		uint16_t nlb = (uint16_t)(((iod->nr_sectors << 9) >> lba_shift) - 1);
		ctx->cmd.nvm.fua = 1;
		rc = xnvme_nvm_write(ctx, nsid, slba, nlb, io->buf, NULL);
	}
	if (rc == -EBUSY || rc == -EAGAIN) {
		xnvme_queue_put_cmd_ctx(q->xq, ctx);
		return rc;
	}
	if (rc < 0) {
		xnvme_queue_put_cmd_ctx(q->xq, ctx);
		return submit_commit_and_fetch(q, io, rc);
	}

	io->barrier_outstanding = 1;
	io->barrier_err = 0;

	for (uint16_t r = 0; r < dev->nqueues; r++) {
		struct qublk_queue *rq;
		struct io_uring_sqe *sqe;

		if (r == q->q_id) {
			continue;
		}
		rq = &dev->queues[r];
		sqe = io_uring_get_sqe(&q->ring);
		if (!sqe) {
			fprintf(stderr, "qublk: q%d no SQE for MSG_RING fan-out\n", q->q_id);
			dev->stop = 1;
			return -ENOSPC;
		}
		io_uring_prep_msg_ring(sqe, rq->ring.ring_fd, 0,
				       ud_msg(QUBLK_MSG_DO_FLUSH, q->q_id, io->tag), 0);
		sqe->user_data = ud_msg(QUBLK_MSG_SOURCE, 0, 0);
		io->barrier_outstanding++;
	}
	return 0;
}

static int
dispatch(struct qublk_queue *q, struct qublk_io *io)
{
	struct qublk_dev *dev = q->dev;
	const struct ublksrv_io_desc *iod = io->iod;
	struct xnvme_cmd_ctx *ctx;
	uint64_t bytes, slba;
	uint32_t nsid = xnvme_dev_get_nsid(dev->xdev);
	uint16_t nlb;
	uint8_t op = ublksrv_get_op(iod);
	uint8_t lba_shift = dev->lba_shift;
	int rc;

	if (op != UBLK_IO_OP_FLUSH) {
		bytes = (uint64_t)iod->nr_sectors << 9;
		if (bytes > dev->max_io_buf) {
			fprintf(stderr, "qublk: tag %u: I/O %lu B exceeds max_io_buf %u\n",
				io->tag, (unsigned long)bytes, dev->max_io_buf);
			return submit_commit_and_fetch(q, io, -EINVAL);
		}
		// A zero-length or sub-LBA-sized transfer would underflow the
		// zero-based 'nlb' computed below
		if (!bytes || (bytes & ((1u << lba_shift) - 1))) {
			fprintf(stderr,
				"qublk: tag %u: I/O %lu B is not a multiple of the LBA size\n",
				io->tag, (unsigned long)bytes);
			return submit_commit_and_fetch(q, io, -EINVAL);
		}
	}

	if (op == UBLK_IO_OP_FLUSH ||
	    (op == UBLK_IO_OP_WRITE && (iod->op_flags & UBLK_IO_F_FUA))) {
		return dispatch_barrier(q, io, op);
	}

	ctx = xnvme_queue_get_cmd_ctx(q->xq);
	if (!ctx) {
		return -EBUSY;
	}
	/*
	 * The ctx is pooled per xnvme queue and reused across commands.
	 * xnvme_nvm_{read,write} only writes opcode/nsid/slba/nlb, so other
	 * fields in cdw12 (fua, lr, prinfo, ...) carry over from the prior
	 * command. Zero the NVMe command header before each submit.
	 */
	memset(&ctx->cmd, 0, sizeof(ctx->cmd));
	xnvme_cmd_ctx_set_cb(ctx, on_xnvme_complete, io);

	switch (op) {
	case UBLK_IO_OP_READ:
		slba = iod->start_sector >> (lba_shift - 9);
		nlb = (uint16_t)(((iod->nr_sectors << 9) >> lba_shift) - 1);
		rc = xnvme_nvm_read(ctx, nsid, slba, nlb, io->buf, NULL);
		break;
	case UBLK_IO_OP_WRITE:
		slba = iod->start_sector >> (lba_shift - 9);
		nlb = (uint16_t)(((iod->nr_sectors << 9) >> lba_shift) - 1);
		rc = xnvme_nvm_write(ctx, nsid, slba, nlb, io->buf, NULL);
		break;
	default:
		xnvme_queue_put_cmd_ctx(q->xq, ctx);
		return submit_commit_and_fetch(q, io, -EOPNOTSUPP);
	}

	if (rc == -EBUSY || rc == -EAGAIN) {
		xnvme_queue_put_cmd_ctx(q->xq, ctx);
		return rc;
	}
	if (rc < 0) {
		xnvme_queue_put_cmd_ctx(q->xq, ctx);
		return submit_commit_and_fetch(q, io, rc);
	}
	return 0;
}

static int
handle_ublk_cqe(struct qublk_queue *q, struct io_uring_cqe *cqe)
{
	struct qublk_io *io;
	uint16_t tag = (uint16_t)cqe->user_data;
	int rc;

	if (tag >= q->depth) {
		fprintf(stderr, "qublk: bogus tag %u in cqe\n", tag);
		return -EINVAL;
	}
	io = &q->ios[tag];

	if (cqe->res == UBLK_IO_RES_OK) {
		rc = dispatch(q, io);
		while (rc == -EBUSY || rc == -EAGAIN) {
			// -EBUSY is a full xnvme queue, cured by poking it; -EAGAIN
			// is an exhausted io_uring SQ, cured only by submitting it
			xnvme_queue_poke(q->xq, 0);
			io_uring_submit(&q->ring);
			rc = dispatch(q, io);
		}
		return rc;
	}
	if (cqe->res == UBLK_IO_RES_ABORT || cqe->res == -ENODEV) {
		q->dev->stop = 1;
		return 0;
	}
	fprintf(stderr, "qublk: tag %u unexpected fetch res %d\n", tag, cqe->res);
	q->dev->stop = 1;
	return 0;
}

static void
handle_msg_cqe(struct qublk_queue *q, uint64_t ud, int res)
{
	unsigned type = (unsigned)((ud >> QUBLK_MSG_TYPE_SHIFT) & QUBLK_MSG_TYPE_MASK);
	uint16_t qid = (uint16_t)((ud >> QUBLK_MSG_QID_SHIFT) & QUBLK_MSG_QID_MASK);
	uint16_t tag = (uint16_t)(ud & QUBLK_MSG_TAG_MASK);

	switch (type) {
	case QUBLK_MSG_SOURCE:
		if (res < 0) {
			fprintf(stderr, "qublk: q%d MSG_RING source CQE err=%d\n", q->q_id, res);
			q->dev->stop = 1;
		}
		break;
	case QUBLK_MSG_DO_FLUSH:
		handle_msg_do_flush(q, qid, tag);
		break;
	case QUBLK_MSG_FLUSH_DONE:
		handle_msg_flush_done(q, tag, res);
		break;
	default:
		fprintf(stderr, "qublk: q%d unknown msg type %u\n", q->q_id, type);
		break;
	}
}

static void
handle_cqe(struct qublk_queue *q, struct io_uring_cqe *cqe)
{
	if (cqe->user_data & QUBLK_UD_MSG_BIT) {
		handle_msg_cqe(q, cqe->user_data, cqe->res);
	} else {
		handle_ublk_cqe(q, cqe);
	}
}

static int
queue_init(struct qublk_dev *dev, struct qublk_queue *q, int q_id, int ublkc_fd)
{
	size_t stride = iod_stride();
	off_t map_off;
	int rc;

	q->dev = dev;
	q->q_id = q_id;
	q->depth = dev->qdepth;
	q->ublkc_fd = ublkc_fd;
	q->ios = NULL;
	q->iod_arr = NULL;
	q->xq = NULL;
	q->tid = 0;
	q->init_rc = 0;

	q->iod_arr_bytes = page_round_up((size_t)q->depth * sizeof(struct ublksrv_io_desc));
	map_off = (off_t)UBLKSRV_CMD_BUF_OFFSET + (off_t)q_id * (off_t)stride;
	q->iod_arr = mmap(NULL, q->iod_arr_bytes, PROT_READ, MAP_SHARED, ublkc_fd, map_off);
	if (q->iod_arr == MAP_FAILED) {
		fprintf(stderr, "mmap(iod q%d): %s\n", q_id, strerror(errno));
		q->iod_arr = NULL;
		return -errno;
	}

	q->ios = calloc(q->depth, sizeof(*q->ios));
	if (!q->ios) {
		return -ENOMEM;
	}
	for (uint16_t t = 0; t < q->depth; t++) {
		q->ios[t].tag = t;
		q->ios[t].q = q;
		q->ios[t].iod = &q->iod_arr[t];
		q->ios[t].buf = xnvme_buf_alloc(dev->xdev, dev->max_io_buf);
		if (!q->ios[t].buf) {
			fprintf(stderr, "xnvme_buf_alloc(%u): %s\n", dev->max_io_buf,
				strerror(errno));
			return -ENOMEM;
		}
	}

	rc = xnvme_queue_init(dev->xdev, q->depth, 0, &q->xq);
	if (rc < 0) {
		fprintf(stderr, "xnvme_queue_init(q%d, %u): %s\n", q_id, q->depth, strerror(-rc));
		return rc;
	}

	rc = rflush_pool_init(dev, q);
	if (rc < 0) {
		fprintf(stderr, "rflush_pool_init(q%d): %s\n", q_id, strerror(-rc));
		return rc;
	}

	return 0;
}

static void
queue_fini(struct qublk_dev *dev, struct qublk_queue *q)
{
	if (q->xq) {
		xnvme_queue_drain(q->xq);
		xnvme_queue_term(q->xq);
		q->xq = NULL;
	}
	rflush_pool_fini(q);
	if (q->ios) {
		for (uint16_t t = 0; t < q->depth; t++) {
			if (q->ios[t].buf) {
				xnvme_buf_free(dev->xdev, q->ios[t].buf);
			}
		}
		free(q->ios);
		q->ios = NULL;
	}
	if (q->iod_arr) {
		munmap(q->iod_arr, q->iod_arr_bytes);
		q->iod_arr = NULL;
	}
}

int
qublk_io_init(struct qublk_dev *dev)
{
	char path[64];
	int ublkc_fd, rc;

	dev->queues = calloc(dev->nqueues, sizeof(*dev->queues));
	if (!dev->queues) {
		return -ENOMEM;
	}

	snprintf(path, sizeof(path), "/dev/ublkc%d", dev->dev_id);
	ublkc_fd = open(path, O_RDWR | O_CLOEXEC);
	if (ublkc_fd < 0) {
		fprintf(stderr, "open(%s): %s\n", path, strerror(errno));
		rc = -errno;
		goto err;
	}

	for (uint16_t i = 0; i < dev->nqueues; i++) {
		dev->queues[i].ublkc_fd = -1;
	}
	dev->queues[0].ublkc_fd = ublkc_fd;

	for (uint16_t i = 0; i < dev->nqueues; i++) {
		rc = queue_init(dev, &dev->queues[i], i, ublkc_fd);
		if (rc < 0) {
			goto err;
		}
	}

	return 0;

err:
	qublk_io_fini(dev);
	return rc;
}

void
qublk_io_fini(struct qublk_dev *dev)
{
	int ublkc_fd = -1;

	if (!dev->queues) {
		return;
	}
	for (uint16_t i = 0; i < dev->nqueues; i++) {
		if (dev->queues[i].ublkc_fd >= 0) {
			ublkc_fd = dev->queues[i].ublkc_fd;
		}
		queue_fini(dev, &dev->queues[i]);
	}
	if (ublkc_fd >= 0) {
		close(ublkc_fd);
	}
	free(dev->queues);
	dev->queues = NULL;
}

/*
 * Submit initial FETCH_REQs from the I/O thread so that ublk_drv records the
 * I/O thread as the queue's ubq_daemon. Subsequent COMMIT_AND_FETCH commands
 * (also from this thread) then pass the daemon == current check.
 */
static int
submit_initial_fetches(struct qublk_queue *q)
{
	int rc;

	for (uint16_t t = 0; t < q->depth; t++) {
		rc = submit_fetch(q, &q->ios[t]);
		if (rc < 0) {
			fprintf(stderr, "submit_fetch(q%d t%u): %s\n", q->q_id, t, strerror(-rc));
			return rc;
		}
	}
	rc = io_uring_submit(&q->ring);
	if (rc < 0) {
		fprintf(stderr, "io_uring_submit(initial FETCHs q%d): %s\n", q->q_id,
			strerror(-rc));
		return rc;
	}
	return 0;
}

static void
io_loop(struct qublk_queue *q)
{
	struct qublk_dev *dev = q->dev;
	struct __kernel_timespec idle_ts = {.tv_nsec = 100 * 1000 * 1000};
	struct io_uring_cqe *cqe;
	unsigned head, count;
	int xp;

	while (!dev->stop) {
		/*
		 * NVMe completions can only arrive while commands are in flight,
		 * and the upcie backend has no completion fd to wait on -- so we
		 * must busy-poll the xnvme queue whenever it has outstanding work.
		 * When it is empty, the only events that can wake us are a new
		 * ublk request (FETCH) or a peer queue's barrier MSG_RING, both of
		 * which post to this ring's fd; block on it instead of spinning so
		 * an idle queue does not pin a core away from the application. The
		 * wait is bounded so the loop re-checks dev->stop -- shutdown sets
		 * the flag from another thread without posting a CQE here.
		 */
		if (xnvme_queue_get_outstanding(q->xq) == 0) {
			io_uring_submit_and_wait_timeout(&q->ring, &cqe, 1, &idle_ts, NULL);
		} else {
			io_uring_submit_and_get_events(&q->ring);
		}

		count = 0;
		io_uring_for_each_cqe(&q->ring, head, cqe)
		{
			handle_cqe(q, cqe);
			count++;
		}
		if (count) {
			io_uring_cq_advance(&q->ring, count);
		}

		if (xnvme_queue_get_outstanding(q->xq)) {
			xnvme_queue_poke(q->xq, 0);
		}
	}

	/* Drain: keep pumping until xnvme queue empty and no more ublk CQEs. */
	for (int idle = 0; idle < 1024;) {
		io_uring_submit_and_get_events(&q->ring);
		count = 0;
		io_uring_for_each_cqe(&q->ring, head, cqe)
		{
			// Dispatch rather than discard; STOP_DEV waits on requests
			// in flight, and one delivered after 'stop' was set would
			// otherwise never be committed
			handle_cqe(q, cqe);
			count++;
		}
		if (count) {
			io_uring_cq_advance(&q->ring, count);
		}

		xp = xnvme_queue_poke(q->xq, 0);
		if (xnvme_queue_get_outstanding(q->xq) == 0 && count == 0 && xp == 0) {
			idle++;
		} else {
			idle = 0;
		}
	}
}

static void *
io_thread_main(void *arg)
{
	struct qublk_queue *q = arg;
	struct qublk_dev *dev = q->dev;
	int rc;

	rc = init_ring(q);
	if (rc < 0) {
		fprintf(stderr, "io_uring_queue_init(q%d): %s\n", q->q_id, strerror(-rc));
		q->init_rc = rc;
		sem_post(&dev->io_ready);
		return NULL;
	}

	rc = io_uring_register_files(&q->ring, &q->ublkc_fd, 1);
	if (rc < 0) {
		fprintf(stderr, "io_uring_register_files(q%d): %s\n", q->q_id, strerror(-rc));
		q->init_rc = rc;
		sem_post(&dev->io_ready);
		io_uring_queue_exit(&q->ring);
		return NULL;
	}

	rc = io_uring_register_ring_fd(&q->ring);
	if (rc < 0) {
		fprintf(stderr, "io_uring_register_ring_fd(q%d): %s\n", q->q_id, strerror(-rc));
		q->init_rc = rc;
		sem_post(&dev->io_ready);
		io_uring_queue_exit(&q->ring);
		return NULL;
	}

	q->init_rc = submit_initial_fetches(q);
	sem_post(&dev->io_ready);
	if (q->init_rc == 0) {
		io_loop(q);
	}
	io_uring_queue_exit(&q->ring);
	return NULL;
}

int
qublk_io_thread_start(struct qublk_dev *dev)
{
	uint16_t started = 0;
	int rc, err = 0;

	if (sem_init(&dev->io_ready, 0, 0) < 0) {
		fprintf(stderr, "sem_init: %s\n", strerror(errno));
		return -errno;
	}

	for (uint16_t i = 0; i < dev->nqueues; i++) {
		dev->queues[i].init_rc = 0;
		rc = pthread_create(&dev->queues[i].tid, NULL, io_thread_main, &dev->queues[i]);
		if (rc) {
			fprintf(stderr, "pthread_create(q%u): %s\n", i, strerror(rc));
			dev->queues[i].tid = 0;
			err = -rc;
			break;
		}
		started++;
	}

	for (uint16_t i = 0; i < started; i++) {
		sem_wait(&dev->io_ready);
	}
	sem_destroy(&dev->io_ready);

	if (err == 0) {
		for (uint16_t i = 0; i < dev->nqueues; i++) {
			if (dev->queues[i].init_rc < 0) {
				err = dev->queues[i].init_rc;
				break;
			}
		}
	}

	if (err < 0) {
		dev->stop = 1;
		qublk_io_thread_join(dev);
		return err;
	}
	return 0;
}

void
qublk_io_thread_join(struct qublk_dev *dev)
{
	if (!dev->queues) {
		return;
	}
	for (uint16_t i = 0; i < dev->nqueues; i++) {
		if (dev->queues[i].tid) {
			pthread_join(dev->queues[i].tid, NULL);
			dev->queues[i].tid = 0;
		}
	}
}
