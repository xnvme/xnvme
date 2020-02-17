// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <libxnvmec.h>

#define XNVME_TESTS_QDEPTH_MAX 512
#define XNVME_TESTS_NQUEUE_MAX 1024

static int
test_init_term(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint64_t count = cli->args.count;
	uint64_t qd = cli->args.qdepth;

	struct xnvme_async_ctx *actx[XNVME_TESTS_NQUEUE_MAX] = { 0 };

	if (count > XNVME_TESTS_NQUEUE_MAX) {
		XNVME_DEBUG("FAILED: count(%zu) out-of-bounds for test", count);
		return 1;
	}
	if (qd > XNVME_TESTS_QDEPTH_MAX) {
		XNVME_DEBUG("FAILED: qd(%zu) out-of-bounds for test", qd);
		return 1;
	}

	{
		// Ask how many queues are supported
		struct xnvme_spec_feat feat = { .val = 0 };
		struct xnvme_req req = { 0 };
		int err;

		err = xnvme_cmd_gfeat(dev, 0x0, XNVME_SPEC_FEAT_NQUEUES,
				      XNVME_SPEC_FEAT_SEL_CURRENT, NULL, 0,
				      &req);
		if (err || xnvme_req_cpl_status(&req)) {
			xnvmec_perr("xnvme_cmd_gfeat()", err);
			xnvme_req_pr(&req, XNVME_PR_DEF);
			return -1;
		}

		feat.val = req.cpl.cdw0;

		xnvmec_pinf("qd: %zu, count: %zu", qd, count);
		xnvme_spec_feat_pr(XNVME_SPEC_FEAT_NQUEUES, feat, XNVME_PR_DEF);

		if (count > feat.nqueues.nsqa) {
			xnvmec_pinf("skipping -- count: %zu > nsqa: %u",
				    count, feat.nqueues.nsqa);
			return 0;
		}
	}

	// Initialize asynchronous contexts
	for (uint64_t qn = 0; qn < count; ++qn) {
		int err;

		err = xnvme_async_init(dev, &actx[qn], qd, 0);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_async_init qn(%zu), qd(%zu)",
				    qn, qd);
			return err;
		}

		if (xnvme_async_get_depth(actx[qn]) != qd) {
			XNVME_DEBUG("FAILED: xnvme_async_get_depth != qd(%zu)",
				    qd);
			return -1;
		}
	}

	// Tear down asynchronous contexts
	for (uint64_t qn = 0; qn < count; ++qn) {
		if (xnvme_async_term(dev, actx[qn])) {
			XNVME_DEBUG("FAILED: xnvme_async_term, qn(%zu)", qn);
			return 1;
		}
	}

	return 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub subs[] = {
	{
		"init_term",
		"Create 'count' contexts with given 'qdepth'",
		"Create 'count' contexts with given 'qdepth'", test_init_term, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_COUNT, XNVMEC_LREQ},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LREQ},
		}
	},
};

static struct xnvmec cli = {
	.title = "Test xNVMe Asynchronous Interface",
	.descr_short = "Test xNVMe Asynchronous Interface",
	.subs = subs,
	.nsubs = sizeof subs / sizeof(*subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
