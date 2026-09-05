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
	int opts = cli->args.cq_gpu ? XNVME_QUEUE_CQ_GPU : 0;
	int err = 0;

	struct xnvme_queue *queue[XNVME_TESTS_NQUEUE_MAX] = {0};

	if (count > XNVME_TESTS_NQUEUE_MAX) {
		XNVME_DEBUG("FAILED: count(%zu) out-of-bounds for test", count);
		return 1;
	}
	if (qd > XNVME_TESTS_QDEPTH_MAX) {
		XNVME_DEBUG("FAILED: qd(%zu) out-of-bounds for test", qd);
		return 1;
	}

	xnvme_cli_pinf("count: %zu", count);
	xnvme_cli_pinf("qdepth: %zu", qd);
	xnvme_cli_pinf("clear: %d", cli->args.clear);
	xnvme_cli_pinf("cq_gpu: %d", cli->args.cq_gpu);

	if (!cli->args.clear) {
		// Ask how many queues are supported
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		struct xnvme_spec_feat feat = {.val = 0};

		err = xnvme_adm_gfeat(&ctx, 0x0, XNVME_SPEC_FEAT_NQUEUES,
				      XNVME_SPEC_FEAT_SEL_CURRENT, NULL, 0);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("xnvme_adm_gfeat()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			return err;
		}

		feat.val = ctx.cpl.cdw0;

		xnvme_spec_feat_pr(XNVME_SPEC_FEAT_NQUEUES, feat, XNVME_PR_DEF);

		if (count >= (uint64_t)(feat.nqueues.nsqa + 1)) {
			xnvme_cli_pinf("skipping -- count: %zu > (nsqa + 1): %u", count,
				       feat.nqueues.nsqa);
			return 0;
		}
	}

	// Initialize and check capacity of asynchronous contexts
	for (uint64_t qn = 0; qn < count; ++qn) {
		err = xnvme_queue_init(dev, qd, opts, &queue[qn]);
		if (err) {
			XNVME_DEBUG("FAILED: init qn: %zu, qd: %zu, err: %d", qn, qd, err);

			if (!((err == -ENOMEM) && (qn == count - 1))) {
				goto exit;
			}

			XNVME_DEBUG("INFO: expected failure => OK");
			err = 0;
			continue;
		}

		if (xnvme_queue_get_capacity(queue[qn]) != qd) {
			XNVME_DEBUG("FAILED: xnvme_queue_get_capacity() != qd(%zu)", qd);
			err = -EIO;
			goto exit;
		}
	}

exit:
	// Tear down queues
	for (uint64_t qn = 0; qn < count; ++qn) {
		XNVME_DEBUG("INFO: qn: %zu", qn);
		if (!queue[qn]) {
			continue;
		}
		if (xnvme_queue_term(queue[qn])) {
			XNVME_DEBUG("FAILED: xnvme_queue_term, qn(%zu)", qn);
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

/**
 * Create a queue, fill it with reads, drain it and tear it down, 'count' times over
 *
 * Every round reuses whatever the backend released in the previous one, so this is
 * where a queue-scoped resource that is not given back, or a shared one that is torn
 * down with the first queue rather than the last, shows up.
 */
static int
test_init_io_term(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	uint64_t count = cli->args.count;
	uint64_t qd = cli->args.qdepth;
	int opts = cli->args.cq_gpu ? XNVME_QUEUE_CQ_GPU : 0;
	size_t buf_nbytes = qd * geo->lba_nbytes;
	void *buf;
	int err = 0;

	if (qd > XNVME_TESTS_QDEPTH_MAX) {
		XNVME_DEBUG("FAILED: qd(%zu) out-of-bounds for test", qd);
		return 1;
	}

	xnvme_cli_pinf("count: %zu", count);
	xnvme_cli_pinf("qdepth: %zu", qd);
	xnvme_cli_pinf("cq_gpu: %d", cli->args.cq_gpu);

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
			XNVME_DEBUG("FAILED: round: %zu, completed: %zu != qd: %zu", round,
				    completed, qd);
			err = -EIO;
		}
		if (xnvme_queue_term(queue)) {
			XNVME_DEBUG("FAILED: xnvme_queue_term, round(%zu)", round);
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
			{XNVME_CLI_OPT_CQ_GPU, XNVME_CLI_LFLG},

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
			{XNVME_CLI_OPT_CQ_GPU, XNVME_CLI_LFLG},

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
