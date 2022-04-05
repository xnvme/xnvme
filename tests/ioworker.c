// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvmec.h>

#define QDEPTH_MAX 16
#define QDEPTH_DEF 4

struct iowork_stats {
	uint64_t nerrors;
	uint64_t ncompletions;
	uint64_t nsubmissions;
};

static void
iowork_stats_reset(struct iowork_stats *stats)
{
	stats->nerrors = stats->ncompletions = stats->nsubmissions = 0;
	return;
}

struct iowork_range {
	uint32_t slba;   ///< First LBA of the range
	uint32_t elba;   ///< Last LBA of the range
	uint64_t nbytes; ///< Number of bytes in range
	uint64_t naddr;  ///< Number of addresses (count-from-1)
	uint64_t nlb;    ///< Number of LBAs (count-from-0)
};

/**
 * Per I/O life-time objects, for pre-allocation
 */
struct ioworker {
	int id;
	struct iowork *work;

	int vectored;      ///< Whether or not the vectored I/O path is used
	struct iovec *vec; ///< Array of iovecs for worker to utilize
	int vec_cnt;

	struct iowork_range range; ///< Range for the worker to do I/O within
};

/**
 * Argument setup for I/O work
 */
struct iowork {
	const struct xnvme_geo *geo;
	struct xnvme_dev *dev;
	struct xnvme_queue *queue;
	uint32_t qdepth; ///< Intended queue-pressure / out-standing I/O
	uint32_t nsid;

	struct iowork_range range; ///< Range to confine I/O inside of

	struct {
		size_t nlb;    ///< Number of LBAs per I/O (count-from-0)
		size_t nbytes; ///< Number of bytes pr. I/O
		size_t naddr;  ///< Number of addresses per I/O (count-from-1)
	} io;                  ///< Per I/O args

	struct iowork_stats stats;

	char *wbuf;
	char *rbuf;

	struct {
		char *data;
		uint64_t slba;
	} cur;

	int vectored; ///< Whether or not the vectored I/O path is used
	int vec_cnt;

	struct ioworker workers[QDEPTH_MAX];
	uint32_t nworkers;
};

int
iowork_pp(struct iowork *work)
{
	int wrtn = 0;

	wrtn += printf("iowork:");
	if (!work) {
		wrtn += printf(" ~\n");
		return wrtn;
	}

	printf("\n");
	printf("  io.nbytes: %zu\n", work->io.nbytes);
	printf("  vectored: %d\n", work->vectored);
	printf("  nworkers: %d\n", work->nworkers);
	printf("  range.nbytes: %zu\n", work->range.nbytes);
	printf("  range.naddr: %zu\n", work->range.naddr);
	printf("  range.slba: %u\n", work->range.slba);
	printf("  range.elba: %u\n", work->range.elba);

	return 0;
}

/**
 * On processing of command completion; on success, a new command is submitted, re-using the given
 * 'ctx'. On error, further submissions are aborted and the given 'ctx' yielded back to the queue.
 * Regardless of success/error then stats are updated.
 */
static void
on_completion(struct xnvme_cmd_ctx *ctx, void *cb_arg)
{
	struct ioworker *worker = cb_arg;
	struct iowork *work = worker->work;
	int err;

	work->stats.ncompletions += 1;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		xnvmec_perr("on_completion()", errno);
		xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
		goto error;
	}

	if (work->cur.slba >= work->range.elba) {
		xnvme_queue_put_cmd_ctx(work->queue, ctx);
		return;
	}

	ctx->cmd.nvm.slba = ++(work->cur.slba);

	worker->vec[0].iov_base = work->cur.data + work->cur.slba * work->io.nbytes;

	err = work->vectored
		      ? xnvme_cmd_passv(ctx, worker->vec, worker->vec_cnt, work->io.nbytes, NULL,
					0, 0)
		      : xnvme_cmd_pass(ctx, worker->vec[0].iov_base, work->io.nbytes, NULL, 0);
	if (err) {
		xnvmec_perr("xnvme_cmd_pass{v}()", err);
		goto error;
	}
	work->stats.nsubmissions += 1;

	return;

error:
	work->stats.nerrors += 1;
	xnvme_queue_put_cmd_ctx(work->queue, ctx);
}

/**
 * Tear down function for I/O worker arguments
 */
static int
iowork_teardown(struct iowork *work)
{
	int err;

	for (uint32_t i = 0; i < work->nworkers; ++i) {
		xnvme_buf_free(work->dev, work->workers[i].vec);
	}

	xnvme_buf_free(work->dev, work->rbuf);
	xnvme_buf_free(work->dev, work->wbuf);

	err = xnvme_queue_term(work->queue);
	if (err) {
		xnvmec_perr("xnvme_queue_term()", err);
	}

	return 0;
}

static int
iowork_from_cli(struct xnvmec *cli, struct iowork *work)
{
	int err;

	xnvmec_buf_fill(work, sizeof(*work), "zero");

	work->dev = cli->args.dev;
	work->nsid = xnvme_dev_get_nsid(work->dev);
	work->geo = xnvme_dev_get_geo(work->dev);
	work->qdepth = cli->given[XNVMEC_OPT_QDEPTH] ? cli->args.qdepth : QDEPTH_DEF;
	work->vectored = cli->given[XNVMEC_OPT_VEC_CNT] && cli->args.vec_cnt;
	work->vec_cnt = work->vectored ? cli->args.vec_cnt : 1;
	work->nworkers = work->qdepth;

	if (((work->qdepth) < 1) || (work->qdepth > QDEPTH_MAX)) {
		XNVME_DEBUG("FAILED: invalid, work->qdepth: %d", work->qdepth);
		return -EINVAL;
	}

	work->io.nbytes = work->vec_cnt * work->geo->lba_nbytes;
	work->io.naddr = work->vec_cnt;
	work->io.nlb = work->io.naddr - 1;

	// work->range.nbytes = 1 << 27;
	work->range.nbytes = 1 << 24;
	work->range.naddr = work->range.nbytes / work->geo->lba_nbytes;
	work->range.slba = 0;
	work->range.elba = work->range.slba + work->range.naddr - 1;

	work->wbuf = xnvme_buf_alloc(cli->args.dev, work->range.nbytes);
	if (!work->wbuf) {
		err = -errno;
		XNVME_DEBUG("FAILED: xnvme_buf_alloc(data), err: %d", errno);
		goto failed;
	}
	xnvmec_buf_fill(work->wbuf, work->range.nbytes, "rand-t");

	work->rbuf = xnvme_buf_alloc(cli->args.dev, work->range.nbytes);
	if (!work->rbuf) {
		err = -errno;
		XNVME_DEBUG("FAILED: xnvme_buf_alloc(buf), err: %d", errno);
		goto failed;
	}
	xnvmec_buf_fill(work->rbuf, work->io.nbytes, "zero");

	err = xnvme_queue_init(work->dev, work->qdepth * 2, 0, &work->queue);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_queue_init(), err: %d", err);
		goto failed;
	}
	xnvme_queue_set_cb(work->queue, on_completion, work);

	for (uint32_t i = 0; i < work->nworkers; ++i) {
		struct ioworker *worker = &work->workers[i];

		worker->id = i;
		worker->work = work;
		worker->vec_cnt = work->vec_cnt;
		worker->vectored = work->vectored;

		worker->range = work->range;

		worker->vec_cnt = work->vec_cnt;
		worker->vec = xnvme_buf_alloc(work->dev, sizeof(*worker->vec) * worker->vec_cnt);
		if (!worker->vec) {
			err = -errno;
			XNVME_DEBUG("FAILED: xnvme_buf_alloc(vec), err: %d", errno);
			goto failed;
		}
		xnvmec_buf_fill(worker->vec, sizeof(*worker->vec) * worker->vec_cnt, "zero");
	}

	return 0;

failed:
	iowork_teardown(work);
	return err;
}

static int
_submit(struct iowork *work, int opcode)
{
	for (uint32_t i = 0; i < work->nworkers; ++i) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(work->queue);
		struct ioworker *worker = &work->workers[i];
		int err;

		work->cur.slba = work->range.slba + i;

		ctx->cmd.common.nsid = work->nsid;
		ctx->cmd.common.opcode = opcode;
		ctx->cmd.nvm.slba = work->cur.slba;
		ctx->cmd.nvm.nlb = work->io.nlb;

		ctx->async.cb_arg = worker;

		worker->vec[0].iov_base = work->cur.data + work->cur.slba * work->io.nbytes;
		worker->vec[0].iov_len = work->io.nbytes;

submit:
		err = work->vectored ? xnvme_cmd_passv(ctx, worker->vec, worker->vec_cnt,
						       work->io.nbytes, NULL, 0, 0)
				     : xnvme_cmd_pass(ctx, worker->vec[0].iov_base,
						      work->io.nbytes, NULL, 0);
		switch (err) {
		case 0:
			work->stats.nsubmissions += 1;
			continue;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(work->queue, 0);
			goto submit;

		default:
			XNVME_DEBUG("FAILED: xnvme_cmd_pass(), err: %d", err);
			xnvme_queue_put_cmd_ctx(work->queue, ctx);
			return err;
		}
	}

	return 0;
}

static int
test_verify(struct xnvmec *cli)
{
	struct iowork work;
	size_t diff;
	int err;

	err = iowork_from_cli(cli, &work);
	if (err) {
		XNVME_DEBUG("FAILED: iowork_from_cli(), err: %d", err);
		return err;
	}

	iowork_pp(&work);
	xnvme_dev_pr(work.dev, XNVME_PR_DEF);

	for (int i = 0; i < 2; i++) {
		int opc;

		iowork_stats_reset(&work.stats);
		work.cur.data = i ? work.rbuf : work.wbuf;
		opc = i ? XNVME_SPEC_NVM_OPC_READ : XNVME_SPEC_NVM_OPC_WRITE;

		// Fill queue with commands
		err = _submit(&work, opc);
		if (err) {
			xnvmec_perr("_submit()", err);
			goto exit;
		}

		// Process completions, re-submitting upon completion, until done
		while (((!work.stats.nerrors) && (work.stats.ncompletions < work.range.naddr))) {
			int res = xnvme_queue_poke(work.queue, 0);
			if (res < 0) {
				err = res;
				xnvmec_perr("xnvme_queue_poke()", err);
				goto exit;
			}
		}
		xnvmec_pinf("opc: %d, stats: {submitted: %u, completed: %u, ecount: %u}", opc,
			    work.stats.nsubmissions, work.stats.ncompletions, work.stats.nerrors);

		if (work.stats.ncompletions != work.range.naddr) {
			xnvmec_pinf("FAILED: logic-error; stats.ncompletions != range.naddr");
			err = EIO;
			goto exit;
		}
	}

exit:
	xnvmec_pinf("exit-stats: {submitted: %u, completed: %u, ecount: %u}",
		    work.stats.nsubmissions, work.stats.ncompletions, work.stats.nerrors);

	diff = xnvmec_buf_diff(work.wbuf, work.rbuf, work.range.nbytes);
	iowork_teardown(&work);

	if (err || (work.stats.nerrors) || diff) {
		xnvmec_pinf("ERR: {err: %d, nerrs: %zu, diff: %zu}", err, work.stats.nerrors,
			    diff);
		return EIO;
	}

	return 0;
}

static struct xnvmec_sub g_subs[] = {
	{
		"verify",
		"Write, then read and compare",
		"Write, then read and compare",
		test_verify,
		{
			{XNVMEC_OPT_URI, XNVMEC_POSA},

			{XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_BE, XNVMEC_LOPT},
			{XNVMEC_OPT_SYNC, XNVMEC_LOPT},
			{XNVMEC_OPT_ASYNC, XNVMEC_LOPT},
			{XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
			{XNVMEC_OPT_DIRECT, XNVMEC_LOPT},
			{XNVMEC_OPT_VEC_CNT, XNVMEC_LOPT},
		},
	},
};

static struct xnvmec g_cli = {
	.title = "Test xNVMe basic buffer alloc/free",
	.descr_short = "Test xNVMe basic buffer alloc/free",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
