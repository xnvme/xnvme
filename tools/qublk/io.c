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
#include <xnvme_util.h>

#ifndef IORING_SETUP_SINGLE_ISSUER
#define IORING_SETUP_SINGLE_ISSUER (1U << 12)
#endif
#ifndef IORING_SETUP_DEFER_TASKRUN
#define IORING_SETUP_DEFER_TASKRUN (1U << 13)
#endif

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

static int
init_ring(struct qublk_thread *t)
{
	struct io_uring_params p = {0};
	unsigned entries = 0;
	int rc;

	// A tag never has more than one ublk command waiting to be submitted, so
	// one SQE per tag, over all queues of the thread, is always enough
	for (uint32_t i = 0; i < t->nqueues; i++) {
		entries += t->queues[i]->depth;
	}

	p.flags = IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_DEFER_TASKRUN;
	rc = io_uring_queue_init_params(entries, &t->ring, &p);
	if (rc == -EINVAL) {
		memset(&p, 0, sizeof(p));
		rc = io_uring_queue_init_params(entries, &t->ring, &p);
	}

	return rc;
}

static uint64_t
to_user_data(const struct qublk_queue *q, uint16_t tag)
{
	return ((uint64_t)q->slot << 16) | tag;
}

static void
prep_io_uring_cmd(struct io_uring_sqe *sqe, const struct qublk_queue *q, uint32_t cmd_op,
		  const struct ublksrv_io_cmd *cmd)
{
	io_uring_prep_rw(IORING_OP_URING_CMD, sqe, (int)q->slot, NULL, 0, 0);
	sqe->flags = IOSQE_FIXED_FILE;
	sqe->cmd_op = cmd_op;
	memcpy(sqe->cmd, cmd, sizeof(*cmd));
	sqe->user_data = to_user_data(q, cmd->tag);
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

	sqe = io_uring_get_sqe(&q->thread->ring);
	if (!sqe) {
		return -EAGAIN;
	}

	prep_io_uring_cmd(sqe, q, UBLK_U_IO_FETCH_REQ, &cmd);
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

	sqe = io_uring_get_sqe(&q->thread->ring);
	if (!sqe) {
		return -EAGAIN;
	}

	prep_io_uring_cmd(sqe, q, UBLK_U_IO_COMMIT_AND_FETCH_REQ, &cmd);
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
			result = (int)(io->iod->nr_sectors << XNVME_UNIVERSAL_SECT_SH);
		} else {
			result = 0;
		}
	}

	xnvme_queue_put_cmd_ctx(q->xq, ctx);
	// The ring holds one SQE per tag and each tag has at most one commit
	// pending, so this cannot fail by sizing; if it does anyway, stop the
	// device rather than leave the request uncommitted and the block
	// layer waiting on it forever
	if (submit_commit_and_fetch(q, io, result) < 0) {
		fprintf(stderr, "qublk: dev%d q%d tag %u: no SQE for commit\n", q->dev->dev_id,
			q->q_id, io->tag);
		q->dev->stop = 1;
	}
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
		bytes = (uint64_t)iod->nr_sectors << XNVME_UNIVERSAL_SECT_SH;
		if (bytes > dev->max_io_buf) {
			XNVME_DEBUG("FAILED: dev%d q%d tag %u: I/O %lu B exceeds max_io_buf %u",
				    dev->dev_id, q->q_id, io->tag, (unsigned long)bytes,
				    dev->max_io_buf);
			return submit_commit_and_fetch(q, io, -EINVAL);
		}

		// A zero-length or sub-LBA-sized transfer would underflow the
		// zero-based 'nlb' computed below
		if (!bytes || (bytes & ((1u << lba_shift) - 1))) {
			XNVME_DEBUG("FAILED: dev%d q%d tag %u: %lu B is not LBA-aligned",
				    dev->dev_id, q->q_id, io->tag, (unsigned long)bytes);
			return submit_commit_and_fetch(q, io, -EINVAL);
		}
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
	case UBLK_IO_OP_FLUSH:
		/*
		 * A flush applies to all commands the controller completed
		 * prior to its submission, regardless of which submission
		 * queue carried them (NVMe Base Specification, "Flush"), and
		 * the block layer issues a flush only after the writes it
		 * covers have completed. One FLUSH on the local queue
		 * therefore covers writes completed on every queue.
		 */
		xnvme_prep_nvm(ctx, XNVME_SPEC_NVM_OPC_FLUSH, nsid, 0, 0);
		rc = xnvme_cmd_pass(ctx, NULL, 0, NULL, 0);
		break;
	case UBLK_IO_OP_READ:
		slba = iod->start_sector >> (lba_shift - XNVME_UNIVERSAL_SECT_SH);
		nlb = (uint16_t)(((iod->nr_sectors << XNVME_UNIVERSAL_SECT_SH) >> lba_shift) - 1);
		rc = xnvme_nvm_read(ctx, nsid, slba, nlb, io->buf, NULL);
		break;
	case UBLK_IO_OP_WRITE:
		slba = iod->start_sector >> (lba_shift - XNVME_UNIVERSAL_SECT_SH);
		nlb = (uint16_t)(((iod->nr_sectors << XNVME_UNIVERSAL_SECT_SH) >> lba_shift) - 1);
		if (iod->op_flags & UBLK_IO_F_FUA) {
			ctx->cmd.nvm.fua = 1;
		}

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
handle_ublk_cqe(struct qublk_thread *t, struct io_uring_cqe *cqe)
{
	struct qublk_queue *q;
	struct qublk_io *io;
	uint64_t slot = cqe->user_data >> 16;
	uint16_t tag = (uint16_t)cqe->user_data;
	int rc;

	if (slot >= t->nqueues || tag >= t->queues[slot]->depth) {
		fprintf(stderr, "qublk: bogus user_data 0x%llx in cqe\n",
			(unsigned long long)cqe->user_data);
		return -EINVAL;
	}

	q = t->queues[slot];
	io = &q->ios[tag];

	if (cqe->res == UBLK_IO_RES_OK) {
		rc = dispatch(q, io);
		while (rc == -EBUSY || rc == -EAGAIN) {
			// -EBUSY is a full xnvme queue, cured by poking it; -EAGAIN
			// is an exhausted io_uring SQ, cured only by submitting it
			xnvme_queue_poke(q->xq, 0);
			io_uring_submit(&t->ring);
			rc = dispatch(q, io);
		}

		return rc;
	}

	if (cqe->res == UBLK_IO_RES_ABORT || cqe->res == -ENODEV) {
		q->dev->stop = 1;
		return 0;
	}

	fprintf(stderr, "qublk: dev%d q%d tag %u unexpected fetch res %d\n", q->dev->dev_id,
		q->q_id, tag, cqe->res);
	q->dev->stop = 1;
	return 0;
}

static int
queue_init(struct qublk_dev *dev, struct qublk_queue *q, int q_id)
{
	size_t stride = iod_stride();
	off_t map_off;
	int rc;

	q->dev = dev;
	q->q_id = q_id;
	q->depth = dev->qdepth;
	q->slot = 0;
	q->ios = NULL;
	q->iod_arr = NULL;
	q->xq = NULL;
	q->thread = NULL;

	q->iod_arr_bytes = page_round_up((size_t)q->depth * sizeof(struct ublksrv_io_desc));
	map_off = (off_t)UBLKSRV_CMD_BUF_OFFSET + (off_t)q_id * (off_t)stride;
	q->iod_arr = mmap(NULL, q->iod_arr_bytes, PROT_READ, MAP_SHARED, dev->ublkc_fd, map_off);
	if (q->iod_arr == MAP_FAILED) {
		fprintf(stderr, "mmap(iod dev%d q%d): %s\n", dev->dev_id, q_id, strerror(errno));
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
		fprintf(stderr, "xnvme_queue_init(dev%d q%d, %u): %s\n", dev->dev_id, q_id,
			q->depth, strerror(-rc));
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
	int rc;

	dev->queues = calloc(dev->nqueues, sizeof(*dev->queues));
	if (!dev->queues) {
		return -ENOMEM;
	}

	snprintf(path, sizeof(path), "/dev/ublkc%d", dev->dev_id);
	dev->ublkc_fd = open(path, O_RDWR | O_CLOEXEC);
	if (dev->ublkc_fd < 0) {
		fprintf(stderr, "open(%s): %s\n", path, strerror(errno));
		rc = -errno;
		goto err;
	}

	for (uint16_t i = 0; i < dev->nqueues; i++) {
		rc = queue_init(dev, &dev->queues[i], i);
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
	if (!dev->queues) {
		return;
	}

	for (uint16_t i = 0; i < dev->nqueues; i++) {
		queue_fini(dev, &dev->queues[i]);
	}

	if (dev->ublkc_fd >= 0) {
		close(dev->ublkc_fd);
		dev->ublkc_fd = -1;
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
submit_initial_fetches(struct qublk_thread *t)
{
	int rc;

	for (uint32_t i = 0; i < t->nqueues; i++) {
		struct qublk_queue *q = t->queues[i];

		for (uint16_t tag = 0; tag < q->depth; tag++) {
			rc = submit_fetch(q, &q->ios[tag]);
			if (rc < 0) {
				fprintf(stderr, "submit_fetch(dev%d q%d t%u): %s\n",
					q->dev->dev_id, q->q_id, tag, strerror(-rc));
				return rc;
			}
		}
	}

	rc = io_uring_submit(&t->ring);
	if (rc < 0) {
		fprintf(stderr, "io_uring_submit(initial FETCHs): %s\n", strerror(-rc));
		return rc;
	}

	return 0;
}

// Keep running until every device on this thread has stopped, so one device
// stopping does not leave the others unserved
static int
thread_stopped(const struct qublk_thread *t)
{
	for (uint32_t i = 0; i < t->nqueues; i++) {
		if (!t->queues[i]->dev->stop) {
			return 0;
		}
	}

	return 1;
}

static uint32_t
thread_outstanding(const struct qublk_thread *t)
{
	uint32_t n = 0;

	for (uint32_t i = 0; i < t->nqueues; i++) {
		n += xnvme_queue_get_outstanding(t->queues[i]->xq);
	}

	return n;
}

static int
thread_poke(struct qublk_thread *t)
{
	int busy = 0;

	for (uint32_t i = 0; i < t->nqueues; i++) {
		struct xnvme_queue *xq = t->queues[i]->xq;

		if (xnvme_queue_get_outstanding(xq) && xnvme_queue_poke(xq, 0)) {
			busy = 1;
		}
	}

	return busy;
}

static unsigned
thread_reap(struct qublk_thread *t)
{
	struct io_uring_cqe *cqe;
	unsigned head, count = 0;

	io_uring_for_each_cqe(&t->ring, head, cqe)
	{
		// Dispatch rather than discard; STOP_DEV waits on requests
		// in flight, and one delivered after 'stop' was set would
		// otherwise never be committed
		handle_ublk_cqe(t, cqe);
		count++;
	}

	if (count) {
		io_uring_cq_advance(&t->ring, count);
	}

	return count;
}

static void
io_loop(struct qublk_thread *t)
{
	struct __kernel_timespec idle_ts = {.tv_nsec = 100 * 1000 * 1000};
	struct io_uring_cqe *cqe;
	unsigned count;
	int xp;

	while (!thread_stopped(t)) {
		/*
		 * NVMe completions can only arrive while commands are in flight,
		 * and the upcie backend has no completion fd to wait on -- so we
		 * must busy-poll the xnvme queues whenever they have outstanding
		 * work. When they are empty, the only event that can wake us is a
		 * new ublk request (FETCH), which posts to this ring's fd; block
		 * on it instead of spinning so an idle thread does not pin a core
		 * away from the application. The wait is bounded so the loop
		 * re-checks the devices' 'stop' -- shutdown sets it from another
		 * thread without posting a CQE here.
		 */
		if (thread_outstanding(t) == 0) {
			io_uring_submit_and_wait_timeout(&t->ring, &cqe, 1, &idle_ts, NULL);
		} else {
			io_uring_submit_and_get_events(&t->ring);
		}

		thread_reap(t);
		thread_poke(t);
	}

	/* Drain: keep pumping until xnvme queues empty and no more ublk CQEs. */
	for (int idle = 0; idle < 1024;) {
		io_uring_submit_and_get_events(&t->ring);
		count = thread_reap(t);
		xp = thread_poke(t);
		if (thread_outstanding(t) == 0 && count == 0 && xp == 0) {
			idle++;
		} else {
			idle = 0;
		}
	}
}

static void *
io_thread_main(void *arg)
{
	struct qublk_thread *t = arg;
	int *fds;
	int rc;

	if (t->cpu >= 0) {
		rc = xnvme_util_pin_to_cpu(t->cpu);
		if (rc) {
			fprintf(stderr, "xnvme_util_pin_to_cpu(%d): %s; running unpinned\n",
				t->cpu, strerror(rc));
		}
	}

	rc = init_ring(t);
	if (rc < 0) {
		fprintf(stderr, "io_uring_queue_init(): %s\n", strerror(-rc));
		t->init_rc = rc;
		sem_post(t->io_ready);
		return NULL;
	}

	fds = calloc(t->nqueues, sizeof(*fds));
	if (!fds) {
		t->init_rc = -ENOMEM;
		sem_post(t->io_ready);
		io_uring_queue_exit(&t->ring);
		return NULL;
	}

	for (uint32_t i = 0; i < t->nqueues; i++) {
		fds[i] = t->queues[i]->dev->ublkc_fd;
	}

	rc = io_uring_register_files(&t->ring, fds, t->nqueues);
	free(fds);
	if (rc < 0) {
		fprintf(stderr, "io_uring_register_files(): %s\n", strerror(-rc));
		t->init_rc = rc;
		sem_post(t->io_ready);
		io_uring_queue_exit(&t->ring);
		return NULL;
	}

	rc = io_uring_register_ring_fd(&t->ring);
	if (rc < 0) {
		fprintf(stderr, "io_uring_register_ring_fd(): %s\n", strerror(-rc));
		t->init_rc = rc;
		sem_post(t->io_ready);
		io_uring_queue_exit(&t->ring);
		return NULL;
	}

	t->init_rc = submit_initial_fetches(t);
	sem_post(t->io_ready);
	if (t->init_rc == 0) {
		io_loop(t);
	}

	io_uring_queue_exit(&t->ring);
	return NULL;
}

int
qublk_io_threads_start(struct qublk_dev *devs, uint32_t ndevs, const uint16_t *cpus,
		       uint16_t ncpus, struct qublk_thread **threads, uint32_t *nthreads)
{
	struct qublk_thread *thr;
	sem_t io_ready;
	uint32_t total = 0, nthr, started = 0;
	int rc, err = 0;

	for (uint32_t d = 0; d < ndevs; d++) {
		total += devs[d].nqueues;
	}

	nthr = ncpus ? ncpus : total;
	if (!nthr || nthr > total) {
		return -EINVAL;
	}

	thr = calloc(nthr, sizeof(*thr));
	if (!thr) {
		return -ENOMEM;
	}

	for (uint32_t i = 0, d = 0, q = 0; i < nthr; i++) {
		uint32_t count = total / nthr + (i < total % nthr ? 1 : 0);

		thr[i].cpu = ncpus ? cpus[i] : -1;
		thr[i].io_ready = &io_ready;
		thr[i].queues = calloc(count, sizeof(*thr[i].queues));
		if (!thr[i].queues) {
			qublk_io_threads_join(thr, nthr);
			return -ENOMEM;
		}

		for (uint32_t j = 0; j < count; j++) {
			struct qublk_queue *queue = &devs[d].queues[q];

			queue->thread = &thr[i];
			queue->slot = j;
			thr[i].queues[j] = queue;
			thr[i].nqueues++;

			if (++q == devs[d].nqueues) {
				q = 0;
				d++;
			}
		}
	}

	if (sem_init(&io_ready, 0, 0) < 0) {
		err = -errno;
		fprintf(stderr, "sem_init: %s\n", strerror(errno));
		qublk_io_threads_join(thr, nthr);
		return err;
	}

	for (uint32_t i = 0; i < nthr; i++) {
		rc = pthread_create(&thr[i].tid, NULL, io_thread_main, &thr[i]);
		if (rc) {
			fprintf(stderr, "pthread_create(thread %u): %s\n", i, strerror(rc));
			thr[i].tid = 0;
			err = -rc;
			break;
		}

		started++;
	}

	for (uint32_t i = 0; i < started; i++) {
		sem_wait(&io_ready);
	}

	sem_destroy(&io_ready);

	if (err == 0) {
		for (uint32_t i = 0; i < nthr; i++) {
			if (thr[i].init_rc < 0) {
				err = thr[i].init_rc;
				break;
			}
		}
	}

	if (err < 0) {
		for (uint32_t d = 0; d < ndevs; d++) {
			devs[d].stop = 1;
		}

		qublk_io_threads_join(thr, nthr);
		return err;
	}

	*threads = thr;
	*nthreads = nthr;
	return 0;
}

void
qublk_io_threads_join(struct qublk_thread *threads, uint32_t nthreads)
{
	if (!threads) {
		return;
	}

	for (uint32_t i = 0; i < nthreads; i++) {
		if (threads[i].tid) {
			pthread_join(threads[i].tid, NULL);
			threads[i].tid = 0;
		}

		free(threads[i].queues);
	}

	free(threads);
}
