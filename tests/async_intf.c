// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

#define XNVME_TESTS_QDEPTH_MAX 512
#define XNVME_TESTS_NQUEUE_MAX 1024

static int
test_init_term(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint64_t count = cli->args.count;
	uint64_t qd = cli->args.qdepth;
	int opts = cli->args.p2p_cq_mirror ? XNVME_QUEUE_P2P_CQ_MIRROR : 0;
	int err = 0;

	struct xnvme_queue *queue[XNVME_TESTS_NQUEUE_MAX] = {0};

	if (count > XNVME_TESTS_NQUEUE_MAX) {
		XNVME_DEBUG("FAILED: count(%" PRIu64 ") out-of-bounds for test", count);
		return 1;
	}
	if (qd > XNVME_TESTS_QDEPTH_MAX) {
		XNVME_DEBUG("FAILED: qd(%" PRIu64 ") out-of-bounds for test", qd);
		return 1;
	}

	xnvme_cli_pinf("count: %" PRIu64, count);
	xnvme_cli_pinf("qdepth: %" PRIu64, qd);
	xnvme_cli_pinf("clear: %d", cli->args.clear);
	xnvme_cli_pinf("p2p_cq_mirror: %d", cli->args.p2p_cq_mirror);

	if (!cli->args.clear) {
		// Ask how many queues are supported
		struct xnvme_cmd_ctx ctx = {0};
		struct xnvme_spec_feat feat = {.val = 0};

		xnvme_dev_get_cmd_ctx(dev, &ctx);

		err = xnvme_adm_gfeat(&ctx, 0x0, XNVME_SPEC_FEAT_NQUEUES,
				      XNVME_SPEC_FEAT_SEL_CURRENT, NULL, 0);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("xnvme_adm_gfeat()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			return err;
		}

		feat.val = ctx.cpl.cdw0;

		xnvme_spec_feat_pr(XNVME_SPEC_FEAT_NQUEUES, &feat, XNVME_PR_DEF);

		if (count >= (uint64_t)(feat.nqueues.nsqa + 1)) {
			xnvme_cli_pinf("skipping -- count: %" PRIu64 " > (nsqa + 1): %u", count,
				       feat.nqueues.nsqa);
			return 0;
		}
	}

	// Initialize and check capacity of asynchronous contexts
	for (uint64_t qn = 0; qn < count; ++qn) {
		err = xnvme_queue_init(dev, qd, opts, &queue[qn]);
		if (err) {
			XNVME_DEBUG("FAILED: init qn: %" PRIu64 ", qd: %" PRIu64 ", err: %d", qn,
				    qd, err);

			if (!((err == -ENOMEM) && (qn == count - 1))) {
				goto exit;
			}

			XNVME_DEBUG("INFO: expected failure => OK");
			err = 0;
			continue;
		}

		if (xnvme_queue_get_capacity(queue[qn]) != qd) {
			XNVME_DEBUG("FAILED: xnvme_queue_get_capacity() != qd(%" PRIu64 ")", qd);
			err = -EIO;
			goto exit;
		}
	}

exit:
	// Tear down queues
	for (uint64_t qn = 0; qn < count; ++qn) {
		XNVME_DEBUG("INFO: qn: %" PRIu64, qn);
		if (!queue[qn]) {
			continue;
		}
		if (xnvme_queue_term(queue[qn])) {
			XNVME_DEBUG("FAILED: xnvme_queue_term, qn(%" PRIu64 ")", qn);
			err = -EIO;
		}
	}

	return err;
}

static void
cb_count(struct xnvme_cmd_ctx *ctx, void *cb_arg)
{
	uint64_t *completed = cb_arg;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
	} else {
		*completed += 1;
	}
	xnvme_queue_put_cmd_ctx(ctx->async.queue, ctx);
}

static int
test_init_io_term(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	uint64_t count = cli->args.count;
	uint64_t qd = cli->args.qdepth;
	int opts = cli->args.p2p_cq_mirror ? XNVME_QUEUE_P2P_CQ_MIRROR : 0;
	size_t buf_nbytes = qd * geo->lba_nbytes;
	void *buf;
	int err = 0;

	if (qd > XNVME_TESTS_QDEPTH_MAX) {
		XNVME_DEBUG("FAILED: qd(%" PRIu64 ") out-of-bounds for test", qd);
		return 1;
	}

	xnvme_cli_pinf("count: %" PRIu64, count);
	xnvme_cli_pinf("qdepth: %" PRIu64, qd);
	xnvme_cli_pinf("p2p_cq_mirror: %d", cli->args.p2p_cq_mirror);

	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		return err;
	}

	for (uint64_t round = 0; round < count; ++round) {
		struct xnvme_queue *queue = NULL;
		uint64_t completed = 0;

		err = xnvme_queue_init(dev, qd, opts, &queue);
		if (err) {
			xnvme_cli_perr("xnvme_queue_init()", err);
			goto exit;
		}
		xnvme_queue_set_cb(queue, cb_count, &completed);

		for (uint64_t i = 0; i < qd; ++i) {
			struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);
			uint64_t slba = (round * qd + i) % geo->nsect;

			err = xnvme_nvm_read(ctx, nsid, slba, 0, (char *)buf + i * geo->lba_nbytes,
					     NULL);
			if (err) {
				xnvme_cli_perr("xnvme_nvm_read()", err);
				xnvme_queue_put_cmd_ctx(queue, ctx);
				break;
			}
		}
		if (!err) {
			err = xnvme_queue_drain(queue);
			if (err < 0) {
				xnvme_cli_perr("xnvme_queue_drain()", err);
			} else {
				err = 0;
			}
		}
		if (!err && completed != qd) {
			XNVME_DEBUG("FAILED: round: %" PRIu64 ", completed: %" PRIu64
				    " != qd: %" PRIu64,
				    round, completed, qd);
			err = -EIO;
		}
		if (xnvme_queue_term(queue)) {
			XNVME_DEBUG("FAILED: xnvme_queue_term, round(%" PRIu64 ")", round);
			err = err ? err : -EIO;
		}
		if (err) {
			goto exit;
		}
	}

exit:
	xnvme_buf_free(dev, buf);

	return err;
}

struct record {
	uint64_t slba;
	uint32_t reaped;
};

/**
 * Reaps 'qd' reads at most 'max' at a time, checking that each comes back once with its 'record'
 * and that no call stores past 'max'
 */
static int
reap_round(struct xnvme_queue *queue, uint32_t nsid, const struct xnvme_geo *geo, char *buf,
	   struct record *records, uint64_t qd, uint32_t max)
{
	struct xnvme_cmd_ctx *completions[XNVME_TESTS_QDEPTH_MAX + 1];
	struct xnvme_cmd_ctx unreaped = {0};
	uint64_t reaped = 0;
	int err;

	for (uint64_t i = 0; i < qd; ++i) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

		records[i].slba = i % geo->nsect;
		records[i].reaped = 0;
		xnvme_cmd_ctx_set_data(ctx, &records[i]);

		err = xnvme_nvm_read(ctx, nsid, records[i].slba, 0, buf + i * geo->lba_nbytes,
				     NULL);
		if (err) {
			xnvme_cli_perr("xnvme_nvm_read()", err);
			xnvme_queue_put_cmd_ctx(queue, ctx);
			return err;
		}
	}

	while (xnvme_queue_get_outstanding(queue)) {
		int n;

		for (uint32_t i = 0; i <= max; ++i) {
			completions[i] = &unreaped;
		}

		n = xnvme_queue_reap(queue, completions, max);
		if (n < 0) {
			xnvme_cli_perr("xnvme_queue_reap()", n);
			return n;
		}
		if ((uint32_t)n > max || completions[max] != &unreaped) {
			xnvme_cli_pinf("FAILED: xnvme_queue_reap(max: %u) stored more than max",
				       max);
			return -EIO;
		}

		for (int i = 0; i < n; ++i) {
			struct xnvme_cmd_ctx *ctx = completions[i];
			struct record *record = xnvme_cmd_ctx_get_data(ctx);
			uintptr_t first = (uintptr_t)records, last = (uintptr_t)&records[qd - 1];

			if (ctx == &unreaped) {
				xnvme_cli_pinf("FAILED: xnvme_queue_reap() returned %d, stored %d",
					       n, i);
				return -EIO;
			}
			if ((uintptr_t)record < first || (uintptr_t)record > last) {
				xnvme_cli_pinf("FAILED: data: %p is none of the records",
					       (void *)record);
				return -EIO;
			}
			if (record->slba != ctx->cmd.nvm.slba) {
				xnvme_cli_pinf("FAILED: data: slba: 0x%" PRIx64
					       " != cmd.nvm.slba: 0x%" PRIx64,
					       record->slba, (uint64_t)ctx->cmd.nvm.slba);
				return -EIO;
			}
			if (record->reaped) {
				xnvme_cli_pinf("FAILED: command %zu reaped twice",
					       (size_t)(record - records));
				return -EIO;
			}
			if (xnvme_cmd_ctx_cpl_status(ctx)) {
				xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
				return -EIO;
			}

			record->reaped = 1;
			reaped += 1;
			xnvme_queue_put_cmd_ctx(queue, ctx);
		}
	}

	if (reaped != qd) {
		xnvme_cli_pinf("FAILED: reaped: %" PRIu64 " != submitted: %" PRIu64, reaped, qd);
		return -EIO;
	}
	for (uint64_t i = 0; i < qd; ++i) {
		if (!records[i].reaped) {
			xnvme_cli_pinf("FAILED: command %" PRIu64 " never reaped", i);
			return -EIO;
		}
	}

	err = xnvme_queue_reap(queue, completions, max);
	if (err) {
		xnvme_cli_pinf("FAILED: xnvme_queue_reap() on an idle queue: %d != 0", err);
		return err < 0 ? err : -EIO;
	}

	return 0;
}

static void
cb_fail(struct xnvme_cmd_ctx *XNVME_UNUSED(ctx), void *cb_arg)
{
	int *called = cb_arg;

	*called += 1;
}

static int
test_reap(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	uint64_t qd = cli->args.qdepth;
	int opts = XNVME_QUEUE_REAP | (cli->args.p2p_cq_mirror ? XNVME_QUEUE_P2P_CQ_MIRROR : 0);
	struct xnvme_cmd_ctx *completions[XNVME_TESTS_QDEPTH_MAX];
	struct record records[XNVME_TESTS_QDEPTH_MAX] = {0};
	struct xnvme_queue *queue = NULL;
	struct xnvme_cmd_ctx *ctx;
	int cb_called = 0;
	char *buf = NULL;
	int err;

	if (!qd || qd > XNVME_TESTS_QDEPTH_MAX) {
		XNVME_DEBUG("FAILED: qd(%" PRIu64 ") out-of-bounds for test", qd);
		return 1;
	}

	xnvme_cli_pinf("qdepth: %" PRIu64, qd);
	xnvme_cli_pinf("p2p_cq_mirror: %d", cli->args.p2p_cq_mirror);

	buf = xnvme_buf_alloc(dev, qd * geo->lba_nbytes);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		return err;
	}

	err = xnvme_queue_init(dev, qd, opts, &queue);
	if (err) {
		xnvme_cli_perr("xnvme_queue_init()", err);
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_fail, &cb_called);

	err = xnvme_queue_reap(queue, completions, qd);
	if (err) {
		xnvme_cli_pinf("FAILED: xnvme_queue_reap() on an idle queue: %d != 0", err);
		err = err < 0 ? err : -EIO;
		goto exit;
	}

	err = xnvme_queue_reap(queue, completions, 0);
	if (err != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_queue_reap(max: 0): %d != -EINVAL", err);
		err = -EIO;
		goto exit;
	}

	ctx = xnvme_queue_get_cmd_ctx(queue);
	xnvme_cmd_ctx_set_data(ctx, &records[0]);
	if (xnvme_cmd_ctx_get_data(ctx) != &records[0] || ctx->async.cb_arg != &records[0]) {
		xnvme_cli_pinf("FAILED: xnvme_cmd_ctx_get_data() != xnvme_cmd_ctx_set_data()");
		err = -EIO;
		xnvme_queue_put_cmd_ctx(queue, ctx);
		goto exit;
	}
	xnvme_queue_put_cmd_ctx(queue, ctx);

	for (uint32_t max = 1; max <= qd; max *= 2) {
		xnvme_cli_pinf("reap_round: {qd: %" PRIu64 ", max: %u}", qd, max);
		err = reap_round(queue, nsid, geo, buf, records, qd, max);
		if (err) {
			goto exit;
		}
	}

	if (cb_called) {
		xnvme_cli_pinf("FAILED: the callback was called %d times for reaped commands",
			       cb_called);
		err = -EIO;
	}

exit:
	if (queue && xnvme_queue_term(queue)) {
		XNVME_DEBUG("FAILED: xnvme_queue_term()");
		err = err ? err : -EIO;
	}
	xnvme_buf_free(dev, buf);

	return err;
}

static int
submit_reads(struct xnvme_queue *queue, uint32_t nsid, const struct xnvme_geo *geo, char *buf,
	     uint64_t qd)
{
	for (uint64_t i = 0; i < qd; ++i) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);
		int err;

		err = xnvme_nvm_read(ctx, nsid, i % geo->nsect, 0, buf + i * geo->lba_nbytes,
				     NULL);
		if (err) {
			xnvme_cli_perr("xnvme_nvm_read()", err);
			xnvme_queue_put_cmd_ctx(queue, ctx);
			return err;
		}
	}

	return 0;
}

/**
 * A backend completing commands while it tears the queue down, as SPDK does when freeing its
 * qpair, must not write through the array of the reap before it
 */
static int
test_term_outstanding(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	uint64_t qd = cli->args.qdepth;
	struct xnvme_queue *queue = NULL;
	struct xnvme_cmd_ctx *completions[1];
	char *buf = NULL;
	int err;

	if (!qd || qd > XNVME_TESTS_QDEPTH_MAX) {
		XNVME_DEBUG("FAILED: qd(%" PRIu64 ") out-of-bounds for test", qd);
		return 1;
	}

	buf = xnvme_buf_alloc(dev, qd * geo->lba_nbytes);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		return err;
	}

	err = xnvme_queue_init(dev, qd, XNVME_QUEUE_REAP, &queue);
	if (err) {
		xnvme_cli_perr("xnvme_queue_init()", err);
		goto exit;
	}

	err = submit_reads(queue, nsid, geo, buf, 1);
	if (err) {
		goto exit;
	}
	while (xnvme_queue_get_outstanding(queue)) {
		int n = (int)xnvme_queue_reap(queue, completions, 1);

		if (n < 0) {
			err = n;
			xnvme_cli_perr("xnvme_queue_reap()", err);
			goto exit;
		}
		for (int i = 0; i < n; ++i) {
			xnvme_queue_put_cmd_ctx(queue, completions[i]);
		}
	}

	err = submit_reads(queue, nsid, geo, buf, qd);
	if (err) {
		goto exit;
	}
	xnvme_cli_pinf("outstanding: %" PRIu32, xnvme_queue_get_outstanding(queue));

exit:
	if (queue) {
		int err_term = xnvme_queue_term(queue);

		if (err_term) {
			xnvme_cli_perr("xnvme_queue_term()", err_term);
			err = err ? err : err_term;
		}
	}
	xnvme_buf_free(dev, buf);

	return err;
}

/**
 * The callback assigned with xnvme_queue_set_cb() is the one xnvme_queue_drain() and
 * xnvme_queue_poke() call, once for every completion
 */
static int
test_cb(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	uint64_t qd = cli->args.qdepth;
	int opts = cli->args.p2p_cq_mirror ? XNVME_QUEUE_P2P_CQ_MIRROR : 0;
	struct xnvme_queue *queue = NULL;
	uint64_t completed = 0;
	char *buf = NULL;
	int err;

	if (!qd || qd > XNVME_TESTS_QDEPTH_MAX) {
		XNVME_DEBUG("FAILED: qd(%" PRIu64 ") out-of-bounds for test", qd);
		return 1;
	}

	xnvme_cli_pinf("qdepth: %" PRIu64, qd);
	xnvme_cli_pinf("p2p_cq_mirror: %d", cli->args.p2p_cq_mirror);

	buf = xnvme_buf_alloc(dev, qd * geo->lba_nbytes);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		return err;
	}

	err = xnvme_queue_init(dev, qd, opts, &queue);
	if (err) {
		xnvme_cli_perr("xnvme_queue_init()", err);
		goto exit;
	}

	err = xnvme_queue_set_cb(queue, cb_count, &completed);
	if (err) {
		xnvme_cli_perr("xnvme_queue_set_cb()", err);
		goto exit;
	}

	err = submit_reads(queue, nsid, geo, buf, qd);
	if (err) {
		goto exit;
	}
	err = xnvme_queue_drain(queue);
	if (err < 0) {
		xnvme_cli_perr("xnvme_queue_drain()", err);
		goto exit;
	}
	if ((uint64_t)err != qd || completed != qd || xnvme_queue_get_outstanding(queue)) {
		xnvme_cli_pinf("FAILED: xnvme_queue_drain(): %d, completed: %" PRIu64
			       ", outstanding: %u, qd: %" PRIu64,
			       err, completed, xnvme_queue_get_outstanding(queue), qd);
		err = -EIO;
		goto exit;
	}

	err = submit_reads(queue, nsid, geo, buf, qd);
	if (err) {
		goto exit;
	}
	while (xnvme_queue_get_outstanding(queue)) {
		err = xnvme_queue_poke(queue, 0);
		if (err < 0) {
			xnvme_cli_perr("xnvme_queue_poke()", err);
			goto exit;
		}
	}
	if (completed != 2 * qd) {
		xnvme_cli_pinf("FAILED: xnvme_queue_poke(): completed: %" PRIu64 " != %" PRIu64,
			       completed, 2 * qd);
		err = -EIO;
		goto exit;
	}
	err = 0;

exit:
	if (queue && xnvme_queue_term(queue)) {
		XNVME_DEBUG("FAILED: xnvme_queue_term()");
		err = err ? err : -EIO;
	}
	xnvme_buf_free(dev, buf);

	return err;
}

/**
 * A queue delivers completions the one way it was initialized for: xnvme_queue_reap() returns
 * -EINVAL for a callback queue, and xnvme_queue_poke() and xnvme_queue_drain() return -EINVAL for
 * a reap queue, idle or with a command outstanding
 */
static int
test_mode(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct xnvme_queue *cbq = NULL, *reapq = NULL;
	struct xnvme_cmd_ctx *completions[1];
	uint64_t completed = 0;
	char *buf = NULL;
	int err;

	buf = xnvme_buf_alloc(dev, geo->lba_nbytes);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		return err;
	}

	err = xnvme_queue_init(dev, 1, 0, &cbq);
	if (err) {
		xnvme_cli_perr("xnvme_queue_init(cbq)", err);
		goto exit;
	}
	err = xnvme_queue_init(dev, 1, XNVME_QUEUE_REAP, &reapq);
	if (err) {
		xnvme_cli_perr("xnvme_queue_init(reapq)", err);
		goto exit;
	}
	xnvme_queue_set_cb(cbq, cb_count, &completed);

	err = xnvme_queue_reap(cbq, completions, 1);
	if (err != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_queue_reap(cbq) idle: %d != -EINVAL", err);
		err = -EIO;
		goto exit;
	}
	err = xnvme_queue_poke(reapq, 0);
	if (err != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_queue_poke(reapq) idle: %d != -EINVAL", err);
		err = -EIO;
		goto exit;
	}
	err = xnvme_queue_drain(reapq);
	if (err != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_queue_drain(reapq) idle: %d != -EINVAL", err);
		err = -EIO;
		goto exit;
	}

	err = submit_reads(cbq, nsid, geo, buf, 1);
	if (err) {
		goto exit;
	}
	err = xnvme_queue_reap(cbq, completions, 1);
	if (err != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_queue_reap(cbq) outstanding: %d != -EINVAL", err);
		err = -EIO;
		goto exit;
	}
	err = xnvme_queue_drain(cbq);
	if (err != 1 || completed != 1) {
		xnvme_cli_pinf("FAILED: xnvme_queue_drain(cbq): %d, completed: %" PRIu64, err,
			       completed);
		err = -EIO;
		goto exit;
	}

	err = submit_reads(reapq, nsid, geo, buf, 1);
	if (err) {
		goto exit;
	}
	err = xnvme_queue_poke(reapq, 0);
	if (err != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_queue_poke(reapq) outstanding: %d != -EINVAL", err);
		err = -EIO;
		goto exit;
	}
	err = xnvme_queue_drain(reapq);
	if (err != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_queue_drain(reapq) outstanding: %d != -EINVAL", err);
		err = -EIO;
		goto exit;
	}
	while (xnvme_queue_get_outstanding(reapq)) {
		err = (int)xnvme_queue_reap(reapq, completions, 1);
		if (err < 0) {
			xnvme_cli_perr("xnvme_queue_reap(reapq)", err);
			goto exit;
		}
		for (int i = 0; i < err; ++i) {
			xnvme_queue_put_cmd_ctx(reapq, completions[i]);
		}
	}
	err = 0;

exit:
	if (reapq && xnvme_queue_term(reapq)) {
		XNVME_DEBUG("FAILED: xnvme_queue_term(reapq)");
		err = err ? err : -EIO;
	}
	if (cbq && xnvme_queue_term(cbq)) {
		XNVME_DEBUG("FAILED: xnvme_queue_term(cbq)");
		err = err ? err : -EIO;
	}
	xnvme_buf_free(dev, buf);

	return err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"init_term",
		"Create 'count' contexts with given 'qdepth'",
		"Create 'count' contexts with given 'qdepth'",
		test_init_term,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_CLEAR, XNVME_CLI_LFLG},
			{XNVME_CLI_OPT_P2P_CQ_MIRROR, XNVME_CLI_LFLG},

			XNVME_CLI_ASYNC_OPTS,
		},
	},
	{
		"init_io_term",
		"Create a queue, read 'qdepth' LBAs through it and tear it down, 'count' times",
		"Create a queue, read 'qdepth' LBAs through it and tear it down, 'count' times",
		test_init_io_term,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_P2P_CQ_MIRROR, XNVME_CLI_LFLG},

			XNVME_CLI_ASYNC_OPTS,
		},
	},
	{
		"reap",
		"Read 'qdepth' LBAs and reap them, checking what xnvme_queue_reap() hands back",
		"Read 'qdepth' LBAs and reap them, checking what xnvme_queue_reap() hands back",
		test_reap,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_P2P_CQ_MIRROR, XNVME_CLI_LFLG},

			XNVME_CLI_ASYNC_OPTS,
		},
	},
	{
		"term_outstanding",
		"Tear down a queue with 'qdepth' reads outstanding",
		"Tear down a queue with 'qdepth' reads outstanding",
		test_term_outstanding,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LREQ},

			XNVME_CLI_ASYNC_OPTS,
		},
	},
	{
		"cb",
		"Read 'qdepth' LBAs, completing them through the callback of the queue",
		"Read 'qdepth' LBAs, completing them through the callback of the queue",
		test_cb,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_P2P_CQ_MIRROR, XNVME_CLI_LFLG},

			XNVME_CLI_ASYNC_OPTS,
		},
	},
	{
		"mode",
		"Check that a queue delivers completions only the way it was initialized for",
		"Check that a queue delivers completions only the way it was initialized for",
		test_mode,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},

			XNVME_CLI_ASYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Test xNVMe Asynchronous Interface",
	.descr_short = "Test xNVMe Asynchronous Interface",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
