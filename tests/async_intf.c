// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

#define XNVME_TESTS_QDEPTH_MAX 512
#define XNVME_TESTS_NQUEUE_MAX 1024
#define XNVME_TESTS_EXHAUST_MAX 128

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
		uint32_t feat = 0;
		struct xnvme_cmd_prep_adm_get_features_args gf = {
			.sel = XNVME_SPEC_ADMIN_GET_FEATURES_CDW10_SEL_CURRENT,
			.fid = XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES,
		};

		xnvme_dev_get_cmd_ctx(dev, &ctx);

		err = xnvme_cmd_prep_adm_get_features(&ctx.cmd, &gf);
		if (!err) {
			err = xnvme_cmd_pass_admin(&ctx, NULL, 0, NULL, 0);
		}
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("get_features(NQUEUES)", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			return err;
		}

		feat = ctx.cpl.dw0;

		xnvme_spec_feat_pr(XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES, feat, XNVME_PR_DEF);

		if (count >=
		    xnvme_bits_get(feat, XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES_CQE_DW0_NSQA) +
			    1) {
			xnvme_cli_pinf(
				"skipping -- count: %" PRIu64 " > (nsqa + 1): %u", count,
				(unsigned int)xnvme_bits_get(
					feat,
					XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES_CQE_DW0_NSQA));
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

			struct xnvme_cmd_prep_nvm_read_args rd = {
				.nsid = nsid,
				.slba = slba,
			};

			err = xnvme_cmd_prep_nvm_read(&ctx->cmd, &rd);
			if (!err) {
				err = xnvme_cmd_pass(
					ctx, (char *)buf + i * geo->lba_nbytes,
					(size_t)xnvme_dev_get_geo(ctx->dev)->lba_nbytes, NULL, 0);
			}
			if (err) {
				xnvme_cli_perr("read", err);
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

/**
 * Creates queues until the controller has none left
 *
 * The controller allocates min(NSQA, NCQA) + 1 I/O queue pairs, as Get Features Number of
 * Queues reports them, and the device and other users may hold some of them already. So
 * creating one queue more than the allocation fails, and the failure is -ENOMEM.
 */
static int
test_init_exhaust(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint64_t qd = cli->args.qdepth;
	struct xnvme_queue *queue[XNVME_TESTS_NQUEUE_MAX] = {0};
	struct xnvme_cmd_ctx ctx = {0};
	uint32_t feat = 0;
	struct xnvme_cmd_prep_adm_get_features_args gf = {
		.sel = XNVME_SPEC_ADMIN_GET_FEATURES_CDW10_SEL_CURRENT,
		.fid = XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES,
	};
	uint32_t nsqa, ncqa, nqueues, created = 0;
	int err;

	if (qd > XNVME_TESTS_QDEPTH_MAX) {
		XNVME_DEBUG("FAILED: qd(%" PRIu64 ") out-of-bounds for test", qd);
		return 1;
	}

	xnvme_dev_get_cmd_ctx(dev, &ctx);
	err = xnvme_cmd_prep_adm_get_features(&ctx.cmd, &gf);
	if (!err) {
		err = xnvme_cmd_pass_admin(&ctx, NULL, 0, NULL, 0);
	}
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("get_features(NQUEUES)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		return err ? err : -EIO;
	}
	feat = ctx.cpl.dw0;
	xnvme_spec_feat_pr(XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES, feat, XNVME_PR_DEF);

	nsqa = (uint32_t)xnvme_bits_get(feat,
					XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES_CQE_DW0_NSQA);
	ncqa = (uint32_t)xnvme_bits_get(feat,
					XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES_CQE_DW0_NCQA);
	nqueues = nsqa < ncqa ? nsqa : ncqa;
	nqueues += 1; ///< NSQA and NCQA are 0's based
	xnvme_cli_pinf("nqueues: %u", nqueues);
	xnvme_cli_pinf("qdepth: %" PRIu64, qd);

	// A uPCIe queue takes 4 MiB of the 1 GiB default heap, which past 128 queues can run
	// out first and fail with -ENOMEM as well
	if (nqueues > XNVME_TESTS_EXHAUST_MAX) {
		xnvme_cli_pinf("skipping -- nqueues: %u > %d", nqueues, XNVME_TESTS_EXHAUST_MAX);
		return 0;
	}

	for (uint32_t qn = 0; qn <= nqueues; ++qn) {
		err = xnvme_queue_init(dev, qd, 0, &queue[qn]);
		if (err) {
			break;
		}
		created++;
	}
	xnvme_cli_pinf("created: %u", created);

	if (!err) {
		XNVME_DEBUG("FAILED: created %u queues, with %u allocated", created, nqueues);
		err = -EIO;
	} else if (err != -ENOMEM) {
		xnvme_cli_perr("xnvme_queue_init() past the allocation; expected -ENOMEM", err);
	} else {
		err = 0;
	}

	for (uint32_t qn = 0; qn < created; ++qn) {
		if (xnvme_queue_term(queue[qn])) {
			XNVME_DEBUG("FAILED: xnvme_queue_term, qn(%u)", qn);
			err = err ? err : -EIO;
		}
	}

	return err;
}

struct record {
	uint64_t slba;
	uint32_t reaped;
};

/// Number of calls of cb_fail(); a reap queue calls no callback
static int g_reap_cb_calls;

static void
cb_fail(struct xnvme_cmd_ctx *XNVME_UNUSED(ctx), void *XNVME_UNUSED(cb_arg))
{
	g_reap_cb_calls += 1;
}

/**
 * xnvme_queue_reap(), returning 0 for the errors that leave the queue intact, -EINTR, -EAGAIN
 * and -EBUSY: the caller handles the 'ncompleted' command-contexts and reaps again
 */
static int
reap(struct xnvme_queue *queue, struct xnvme_cmd_ctx **completions, uint32_t max,
     uint32_t *ncompleted)
{
	int err = xnvme_queue_reap(queue, completions, max, ncompleted);

	return (err == -EINTR || err == -EAGAIN || err == -EBUSY) ? 0 : err;
}

/**
 * Reaps 'qd' commands, the first 'nflush' of them flushes and the rest reads, at most 'max' at a
 * time, checking that each comes back once with its 'record' and that no call stores past 'max'
 */
static int
reap_round(struct xnvme_queue *queue, uint32_t nsid, const struct xnvme_geo *geo, char *buf,
	   struct record *records, uint64_t qd, uint64_t nflush, uint32_t max)
{
	struct xnvme_cmd_ctx *completions[XNVME_TESTS_QDEPTH_MAX + 1];
	struct xnvme_cmd_ctx unreaped = {0};
	uint64_t reaped = 0;
	uint32_t n;
	int err;

	for (uint64_t i = 0; i < qd; ++i) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

		records[i].slba = i < nflush ? 0 : i % geo->nsect;
		records[i].reaped = 0;
		ctx->async.cb = cb_fail;
		xnvme_cmd_ctx_set_data(ctx, &records[i]);

		if (i < nflush) {
			struct xnvme_cmd_prep_io_flush_args fl = {
				.nsid = nsid,
			};

			err = xnvme_cmd_prep_io_flush(&ctx->cmd, &fl);
			if (!err) {
				err = xnvme_cmd_pass(ctx, NULL, 0, NULL, 0);
			}
		} else {
			struct xnvme_cmd_prep_nvm_read_args rd = {
				.nsid = nsid,
				.slba = records[i].slba,
			};

			err = xnvme_cmd_prep_nvm_read(&ctx->cmd, &rd);
			if (!err) {
				err = xnvme_cmd_pass(ctx, buf + i * geo->lba_nbytes,
						     geo->lba_nbytes, NULL, 0);
			}
		}
		if (err) {
			xnvme_cli_perr(i < nflush ? "flush" : "read", err);
			xnvme_queue_put_cmd_ctx(queue, ctx);
			return err;
		}
	}

	while (xnvme_queue_get_outstanding(queue)) {
		for (uint32_t i = 0; i <= max; ++i) {
			completions[i] = &unreaped;
		}

		n = UINT32_MAX;
		err = reap(queue, completions, max, &n);
		if (err) {
			xnvme_cli_perr("xnvme_queue_reap()", err);
			return err < 0 ? err : -EIO;
		}
		if (n > max || completions[max] != &unreaped) {
			xnvme_cli_pinf("FAILED: xnvme_queue_reap(max: %u) stored more than max, "
				       "ncompleted: %u",
				       max, n);
			return -EIO;
		}

		for (uint32_t i = 0; i < n; ++i) {
			struct xnvme_cmd_ctx *ctx = completions[i];
			struct record *record = xnvme_cmd_ctx_get_data(ctx);
			uintptr_t first = (uintptr_t)records, last = (uintptr_t)&records[qd - 1];

			if (ctx == &unreaped) {
				xnvme_cli_pinf("FAILED: xnvme_queue_reap() ncompleted: %u, "
					       "stored: %u",
					       n, i);
				return -EIO;
			}
			if ((uintptr_t)record < first || (uintptr_t)record > last) {
				xnvme_cli_pinf("FAILED: data: %p is none of the records",
					       (void *)record);
				return -EIO;
			}
			if (record->slba !=
			    ((uint64_t)ctx->cmd.common.cdw11 << 32 | ctx->cmd.common.cdw10)) {
				xnvme_cli_pinf("FAILED: data: slba: 0x%" PRIx64
					       " != cdw10_11: 0x%" PRIx64,
					       record->slba,
					       ((uint64_t)ctx->cmd.common.cdw11 << 32 |
						ctx->cmd.common.cdw10));
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

	n = UINT32_MAX;
	err = xnvme_queue_reap(queue, completions, max, &n);
	if (err || n) {
		xnvme_cli_pinf("FAILED: xnvme_queue_reap() on an idle queue: err: %d, "
			       "ncompleted: %u",
			       err, n);
		return err < 0 ? err : -EIO;
	}

	return 0;
}

/**
 * Checks xnvme_queue_reap() on a queue of 'qdepth' with reap_round(), reaping at most one
 * command per call, then two, then four, up to 'qdepth'
 */
static int
reap_rounds(struct xnvme_cli *cli, uint64_t nflush)
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
	uint32_t n;
	char *buf = NULL;
	int err;

	if (!qd || qd < nflush || qd > XNVME_TESTS_QDEPTH_MAX) {
		XNVME_DEBUG("FAILED: qd(%" PRIu64 ") out-of-bounds for test", qd);
		return 1;
	}

	xnvme_cli_pinf("qdepth: %" PRIu64, qd);
	xnvme_cli_pinf("nflush: %" PRIu64, nflush);
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
	g_reap_cb_calls = 0;

	n = UINT32_MAX;
	err = xnvme_queue_reap(queue, completions, qd, &n);
	if (err || n) {
		xnvme_cli_pinf("FAILED: xnvme_queue_reap() on an idle queue: err: %d, "
			       "ncompleted: %u",
			       err, n);
		err = err < 0 ? err : -EIO;
		goto exit;
	}

	n = UINT32_MAX;
	err = xnvme_queue_reap(queue, completions, 0, &n);
	if (err != -EINVAL || n) {
		xnvme_cli_pinf("FAILED: xnvme_queue_reap(max: 0): err: %d != -EINVAL or "
			       "ncompleted: %u != 0",
			       err, n);
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
		err = reap_round(queue, nsid, geo, buf, records, qd, nflush, max);
		if (err) {
			goto exit;
		}
	}

	if (g_reap_cb_calls) {
		xnvme_cli_pinf("FAILED: the callback was called %d times for reaped commands",
			       g_reap_cb_calls);
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
test_reap(struct xnvme_cli *cli)
{
	return reap_rounds(cli, 0);
}

/**
 * The kqueue interface completes a flush when it is submitted and holds it for the next reap;
 * with two flushes, a reap of one command leaves one held and a reap of more retrieves both
 */
static int
test_reap_flush(struct xnvme_cli *cli)
{
	return reap_rounds(cli, 2);
}

static int
submit_reads(struct xnvme_queue *queue, uint32_t nsid, const struct xnvme_geo *geo, char *buf,
	     uint64_t qd)
{
	for (uint64_t i = 0; i < qd; ++i) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);
		int err;

		struct xnvme_cmd_prep_nvm_read_args rd = {
			.nsid = nsid,
			.slba = i % geo->nsect,
		};

		err = xnvme_cmd_prep_nvm_read(&ctx->cmd, &rd);
		if (!err) {
			err = xnvme_cmd_pass(ctx, buf + i * geo->lba_nbytes,
					     (size_t)xnvme_dev_get_geo(ctx->dev)->lba_nbytes *
						     (0 + 1),
					     NULL, 0);
		}
		if (err) {
			xnvme_cli_perr("read", err);
			xnvme_queue_put_cmd_ctx(queue, ctx);
			return err;
		}
	}

	return 0;
}

/**
 * A command that fails is a completion, not an error of the reap: of 'qdepth' commands the first
 * is a compare with a payload that differs from the LBA, and xnvme_queue_reap() returns 0 and
 * hands back every command, the compare alone with a status
 */
static int
test_reap_cmd_fail(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	uint64_t qd = cli->args.qdepth;
	struct xnvme_cmd_ctx *completions[XNVME_TESTS_QDEPTH_MAX];
	struct xnvme_cmd_ctx sync = {0};
	struct xnvme_queue *queue = NULL;
	struct xnvme_cmd_ctx *ctx;
	uint64_t reaped = 0, failed = 0;
	char *buf = NULL;
	int compare;
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

	struct xnvme_cmd_prep_nvm_read_args rd = {
		.nsid = nsid,
	};
	struct xnvme_cmd_prep_nvm_compare_args cmp = {
		.nsid = nsid,
	};

	xnvme_dev_get_cmd_ctx(dev, &sync);
	err = xnvme_cmd_prep_nvm_read(&sync.cmd, &rd);
	if (!err) {
		err = xnvme_cmd_pass(&sync, buf, geo->lba_nbytes, NULL, 0);
	}
	if (err || xnvme_cmd_ctx_cpl_status(&sync)) {
		xnvme_cli_perr("read", err);
		err = err ? err : -EIO;
		goto exit;
	}
	for (uint32_t i = 0; i < geo->lba_nbytes; ++i) {
		buf[i] = ~buf[i];
	}

	err = xnvme_queue_init(dev, qd, XNVME_QUEUE_REAP, &queue);
	if (err) {
		xnvme_cli_perr("xnvme_queue_init()", err);
		goto exit;
	}

	ctx = xnvme_queue_get_cmd_ctx(queue);
	xnvme_cmd_ctx_set_data(ctx, &compare);
	err = xnvme_cmd_prep_nvm_compare(&ctx->cmd, &cmp);
	if (!err) {
		err = xnvme_cmd_pass(ctx, buf, geo->lba_nbytes, NULL, 0);
	}
	if (err) {
		xnvme_cli_perr("compare", err);
		xnvme_queue_put_cmd_ctx(queue, ctx);
		goto exit;
	}
	err = submit_reads(queue, nsid, geo, buf + geo->lba_nbytes, qd - 1);
	if (err) {
		goto exit;
	}

	while (xnvme_queue_get_outstanding(queue)) {
		uint32_t n = UINT32_MAX;

		err = reap(queue, completions, qd, &n);
		if (err || n > qd) {
			xnvme_cli_pinf("FAILED: xnvme_queue_reap(): err: %d, ncompleted: %u", err,
				       n);
			err = err < 0 ? err : -EIO;
			goto exit;
		}

		for (uint32_t i = 0; i < n; ++i) {
			int is_compare = xnvme_cmd_ctx_get_data(completions[i]) == &compare;
			int status = xnvme_cmd_ctx_cpl_status(completions[i]);

			if (is_compare ? !status : status) {
				xnvme_cli_pinf("FAILED: the %s was reaped %s a status",
					       is_compare ? "compare" : "read",
					       status ? "with" : "without");
				xnvme_cmd_ctx_pr(completions[i], XNVME_PR_DEF);
				err = -EIO;
			}
			failed += is_compare;
			reaped += 1;
			xnvme_queue_put_cmd_ctx(queue, completions[i]);
		}
		if (err) {
			goto exit;
		}
	}

	if (reaped != qd || failed != 1) {
		xnvme_cli_pinf("FAILED: reaped: %" PRIu64 " of %" PRIu64 ", the compare %" PRIu64
			       " times",
			       reaped, qd, failed);
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
	struct xnvme_cmd_ctx unreaped = {0};
	struct xnvme_cmd_ctx *completions[2] = {NULL, &unreaped};
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

	// One read is outstanding, so a reap of two leaves the second slot of 'completions' free
	err = submit_reads(queue, nsid, geo, buf, 1);
	if (err) {
		goto exit;
	}
	while (xnvme_queue_get_outstanding(queue)) {
		uint32_t n;

		err = reap(queue, completions, 2, &n);
		for (uint32_t i = 0; i < n; ++i) {
			xnvme_queue_put_cmd_ctx(queue, completions[i]);
		}
		if (err) {
			xnvme_cli_perr("xnvme_queue_reap()", err);
			goto exit;
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
	if (!err && completions[1] != &unreaped) {
		xnvme_cli_pinf("FAILED: xnvme_queue_term() stored in the array of a reap");
		err = -EIO;
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

/// Checks for Status Code Type 7 with ENOSYS in the Status Code, the sync backend's refusal
static int
check_errno_status(struct xnvme_cmd_ctx *ctx)
{
	int sct =
		xnvme_bits_get(ctx->cpl.status, XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SCT);
	int sc = xnvme_bits_get(ctx->cpl.status, XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SC);

	if (sct == XNVME_SPEC_BASE_STATUS_CODE_TYPE_VENDOR_SPECIFIC && sc == ENOSYS) {
		return 0;
	}

	xnvme_cli_pinf("FAILED: sct: 0x%x, sc: 0x%x, expected sct: 0x%x, sc: ENOSYS (0x%x)", sct,
		       sc, XNVME_SPEC_BASE_STATUS_CODE_TYPE_VENDOR_SPECIFIC, ENOSYS);
	xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);

	return -EIO;
}

/// Counts, in 'cb_arg', the completions that carry the vendor status
static void
cb_errno(struct xnvme_cmd_ctx *ctx, void *cb_arg)
{
	int *npassed = cb_arg;

	if (!check_errno_status(ctx)) {
		*npassed += 1;
	}
}

/// Submits a Write Uncorrectable, which the ramdisk refuses with -ENOSYS, on 'ctx' as it is;
/// 'round' 2 first plants a device status that only the clearing before the sync call replaces
static int
submit_nosys(struct xnvme_cmd_ctx *ctx, uint32_t nsid, int round)
{
	struct xnvme_cmd_prep_nvm_write_uncorrectable_args wu = {
		.nsid = nsid,
	};
	int err;

	if (round == 2) {
		ctx->cpl.status = xnvme_bits_set(
			0, XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SC,
			XNVME_SPEC_BASE_GENERIC_COMMAND_STATUS_INVALID_FIELD_IN_COMMAND);
	}

	err = xnvme_cmd_prep_nvm_write_uncorrectable(&ctx->cmd, &wu);
	if (err) {
		return err;
	}

	return xnvme_cmd_pass(ctx, NULL, 0, NULL, 0);
}

/// The three rounds of submit_nosys() on one context of a callback queue
static int
errno_rounds_cb(struct xnvme_queue *queue, uint32_t nsid)
{
	struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);
	int npassed = 0;
	int err;

	err = xnvme_queue_set_cb(queue, cb_errno, &npassed);
	if (err) {
		xnvme_cli_perr("xnvme_queue_set_cb()", err);
		return err;
	}

	for (int round = 0; round < 3; ++round) {
		err = submit_nosys(ctx, nsid, round);
		if (err) {
			xnvme_cli_perr("xnvme_cmd_pass()", err);
			return err;
		}
		err = xnvme_queue_drain(queue);
		if (err < 0) {
			xnvme_cli_perr("xnvme_queue_drain()", err);
			return err;
		}
		if (err != 1 || npassed != round + 1) {
			return -EIO;
		}
	}

	return 0;
}

/// The three rounds on one context of a reap queue, reaped until nothing is outstanding, since
/// thrpool completes on a worker thread; a lost completion fails after a million empty reaps
static int
errno_rounds_reap(struct xnvme_queue *queue, uint32_t nsid)
{
	struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);
	struct xnvme_cmd_ctx *completion = NULL;
	int err;

	for (int round = 0; round < 3; ++round) {
		uint32_t reaped = 0;

		err = submit_nosys(ctx, nsid, round);
		if (err) {
			xnvme_cli_perr("xnvme_cmd_pass()", err);
			return err;
		}
		for (int i = 0; xnvme_queue_get_outstanding(queue); ++i) {
			uint32_t n = 0;

			if (i == 1000000) {
				xnvme_cli_pinf("FAILED: round %d: nothing to reap", round);
				return -ETIMEDOUT;
			}
			err = reap(queue, &completion, 1, &n);
			if (err) {
				xnvme_cli_perr("xnvme_queue_reap()", err);
				return err;
			}
			reaped += n;
		}
		if (reaped != 1 || check_errno_status(completion)) {
			return -EIO;
		}
	}

	return 0;
}

/// A command the sync backend refuses before the device sees it completes with Status Code Type 7
/// and the errno in the Status Code, on a callback queue and on a reap queue, also on a context
/// that still carries the status of a failed command
static int
test_cmd_errno(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	int opts[] = {0, XNVME_QUEUE_REAP};
	int err = 0;

	for (size_t i = 0; i < sizeof(opts) / sizeof(*opts); ++i) {
		struct xnvme_queue *queue = NULL;

		err = xnvme_queue_init(dev, cli->args.qdepth, opts[i], &queue);
		if (err) {
			xnvme_cli_perr("xnvme_queue_init()", err);
			break;
		}
		err = opts[i] ? errno_rounds_reap(queue, nsid) : errno_rounds_cb(queue, nsid);
		if (xnvme_queue_term(queue)) {
			XNVME_DEBUG("FAILED: xnvme_queue_term()");
			err = err ? err : -EIO;
		}
		if (err) {
			break;
		}
	}

	return err;
}

/**
 * A queue delivers completions the one way it was initialized for: xnvme_queue_reap() returns
 * -EINVAL for a callback queue, and xnvme_queue_poke() and xnvme_queue_drain() return -EINVAL for
 * a reap queue, idle or with a command outstanding, as does xnvme_queue_set_cb()
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
	uint32_t n;
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

	err = xnvme_queue_set_cb(reapq, cb_count, &completed);
	if (err != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_queue_set_cb(reapq): %d != -EINVAL", err);
		err = -EIO;
		goto exit;
	}
	n = UINT32_MAX;
	err = xnvme_queue_reap(cbq, completions, 1, &n);
	if (err != -EINVAL || n) {
		xnvme_cli_pinf("FAILED: xnvme_queue_reap(cbq) idle: err: %d != -EINVAL or "
			       "ncompleted: %u != 0",
			       err, n);
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
	n = UINT32_MAX;
	err = xnvme_queue_reap(cbq, completions, 1, &n);
	if (err != -EINVAL || n) {
		xnvme_cli_pinf("FAILED: xnvme_queue_reap(cbq) outstanding: err: %d != -EINVAL or "
			       "ncompleted: %u != 0",
			       err, n);
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
		err = reap(reapq, completions, 1, &n);
		for (uint32_t i = 0; i < n; ++i) {
			xnvme_queue_put_cmd_ctx(reapq, completions[i]);
		}
		if (err) {
			xnvme_cli_perr("xnvme_queue_reap(reapq)", err);
			goto exit;
		}
	}

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
		"init_exhaust",
		"Create queues until the controller has none left; the next fails with -ENOMEM",
		"Create queues until the controller has none left; the next fails with -ENOMEM",
		test_init_exhaust,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LREQ},

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
		"reap_flush",
		"As 'reap', with flushes as the first two of the 'qdepth' commands",
		"As 'reap', with flushes as the first two of the 'qdepth' commands",
		test_reap_flush,
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
		"reap_cmd_fail",
		"Reap 'qdepth' commands, the first a compare that fails, the rest reads",
		"Reap 'qdepth' commands, the first a compare that fails, the rest reads",
		test_reap_cmd_fail,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LREQ},

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
		"cmd_errno",
		"Submit a command the sync backend refuses with -ENOSYS, check the vendor status",
		"Submit a command the sync backend refuses with -ENOSYS, check the vendor status",
		test_cmd_errno,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LREQ},

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
