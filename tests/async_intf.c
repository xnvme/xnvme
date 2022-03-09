// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_adm.h>
#include <libxnvmec.h>

#define XNVME_TESTS_QDEPTH_MAX 512
#define XNVME_TESTS_NQUEUE_MAX 1024

static int
test_init_term(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint64_t count = cli->args.count;
	uint64_t qd = cli->args.qdepth;
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

	xnvmec_pinf("count: %zu", count);
	xnvmec_pinf("qdepth: %zu", qd);
	xnvmec_pinf("clear: %d", cli->args.clear);

	if (!cli->args.clear) {
		// Ask how many queues are supported
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		struct xnvme_spec_feat feat = {.val = 0};

		err = xnvme_adm_gfeat(&ctx, 0x0, XNVME_SPEC_FEAT_NQUEUES,
				      XNVME_SPEC_FEAT_SEL_CURRENT, NULL, 0);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvmec_perr("xnvme_adm_gfeat()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			return err;
		}

		feat.val = ctx.cpl.cdw0;

		xnvme_spec_feat_pr(XNVME_SPEC_FEAT_NQUEUES, feat, XNVME_PR_DEF);

		if (count >= (uint64_t)(feat.nqueues.nsqa + 1)) {
			xnvmec_pinf("skipping -- count: %zu > (nsqa + 1): %u", count,
				    feat.nqueues.nsqa);
			return 0;
		}
	}

	// Initialize and check capacity of asynchronous contexts
	for (uint64_t qn = 0; qn < count; ++qn) {
		err = xnvme_queue_init(dev, qd, 0, &queue[qn]);
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

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub g_subs[] = {
	{
		"init_term",
		"Create 'count' contexts with given 'qdepth'",
		"Create 'count' contexts with given 'qdepth'",
		test_init_term,
		{
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_COUNT, XNVMEC_LREQ},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LREQ},
			{XNVMEC_OPT_CLEAR, XNVMEC_LFLG},

			{XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_BE, XNVMEC_LOPT},
			{XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
			{XNVMEC_OPT_ASYNC, XNVMEC_LOPT},
		},
	},
};

static struct xnvmec g_cli = {
	.title = "Test xNVMe Asynchronous Interface",
	.descr_short = "Test xNVMe Asynchronous Interface",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
