// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <libznd.h>
#include <libxnvmec.h>

#define XNVME_TESTS_QDEPTH_MAX 512
#define XNVME_TESTS_NQUEUE_MAX 1024

struct callback_args {
	uint32_t outstanding;
	uint32_t completed;
	uint32_t errors;
};

/**
 * Here you process the completed command, 'req' provides the return status of
 * the command and 'cb_arg' can has whatever it was given. In this a pointer to
 * the 'outstanding' counter.
 */
static void
callback(struct xnvme_req *req, void *cb_arg)
{
	struct callback_args *callback_args = cb_arg;

	--(*callback_args).outstanding;
	++(*callback_args).completed;

	if (xnvme_req_cpl_status(req)) {
		xnvme_req_pr(req, XNVME_PR_DEF);
		callback_args->errors = 1;
	}
}

static int
test_io_read(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	uint64_t count = cli->args.count;
	uint64_t qd = cli->args.qdepth;

	uint64_t lba = cli->args.lba;
	uint16_t nlb = cli->args.nlb;

	uint64_t nsid = xnvme_dev_get_nsid(dev);

	struct xnvme_async_ctx *actx[XNVME_TESTS_NQUEUE_MAX] = { 0 };
	void *buf = NULL;
	const size_t buf_nbytes = geo->nbytes * geo->nsect;

	int rcode = 0;

	struct callback_args callback_args = {
		.outstanding = 0,
		.completed = 0,
		.errors = 0
	};

	if (count > XNVME_TESTS_NQUEUE_MAX) {
		XNVME_DEBUG("FAILED: count(%zu) out-of-bounds for test", count);
		return 1;
	}
	if (qd > XNVME_TESTS_QDEPTH_MAX) {
		XNVME_DEBUG("FAILED: qd(%zu) out-of-bounds for test", qd);
		return 1;
	}

	// Initialize asynchronous contexts
	for (uint64_t qn = 0; qn < count; ++qn) {
		actx[qn] = xnvme_async_init(dev, qd, 0x0);
		if (!actx[qn]) {
			XNVME_DEBUG("FAILED: xnvme_async_init qn(%zu), qd(%zu)",
				    qn, qd);
			return 1;
		}

		if (xnvme_async_get_depth(actx[qn]) != qd) {
			XNVME_DEBUG("FAILED: xnvme_async_get_depth != qd(%zu)",
				    qd);
			return 1;
		}
	}

	buf = xnvme_buf_alloc(dev, buf_nbytes, NULL);
	if (!buf) {
		xnvmec_perror("xnvme_buf_alloc()");
		rcode = 1;
		goto exit;
	}
	xnvmec_buf_clear(buf, buf_nbytes);

	for (size_t qi = 0; qi < count; ++qi) {
		struct xnvme_req req = { 0 };

		req.async.ctx = actx[qi];
		req.async.cb = callback;
		req.async.cb_arg = &callback_args;

		++callback_args.outstanding;

		if (xnvme_cmd_read(dev, nsid, lba, nlb, buf, NULL,
				   XNVME_CMD_ASYNC, &req)) {
			xnvmec_perror("xnvme_cmd_read()");
			rcode = 1;
			--callback_args.outstanding;
			goto exit;
		}

		if (xnvme_async_wait(dev, req.async.ctx) < 0) {
			xnvmec_perror("xnvme_async_wait()");
			rcode = 1;
			goto exit;
		}
	}

	if (callback_args.errors) {
		xnvmec_pinfo("Got errors: %u", callback_args.errors);
		rcode = 1;
		goto exit;
	}

	if (callback_args.completed != count) {
		xnvmec_pinfo("ERR: completed != count");
		rcode = 1;
		goto exit;
	}

exit:
	xnvme_buf_free(dev, buf);

	// Tear down asynchronous contexts
	for (uint64_t qi = 0; qi < count; ++qi) {
		if (xnvme_async_term(dev, actx[qi])) {
			XNVME_DEBUG("FAILED: xnvme_async_term, qi(%zu)", qi);
			return 1;
		}
	}

	return rcode;
}

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
			xnvmec_perror("xnvme_cmd_gfeat()");
			xnvme_req_pr(&req, XNVME_PR_DEF);
			return -1;
		}

		feat.val = req.cpl.cdw0;

		xnvmec_pinfo("qd: %zu, count: %zu", qd, count);
		xnvme_spec_feat_pr(XNVME_SPEC_FEAT_NQUEUES, feat, XNVME_PR_DEF);

		if (qd > feat.nqueues.nsqa) {
			xnvmec_pinfo("skipping -- qd: %zu > nsqa: %u",
				     qd, feat.nqueues.nsqa);
			return 0;
		}
	}

	// Initialize asynchronous contexts
	for (uint64_t qn = 0; qn < count; ++qn) {
		actx[qn] = xnvme_async_init(dev, qd, 0x0);
		if (!actx[qn]) {
			XNVME_DEBUG("FAILED: xnvme_async_init qn(%zu), qd(%zu)",
				    qn, qd);
			return 1;
		}

		if (xnvme_async_get_depth(actx[qn]) != qd) {
			XNVME_DEBUG("FAILED: xnvme_async_get_depth != qd(%zu)",
				    qd);
			return 1;
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

	{
		"read",
		"Create 'count' contexts with given 'qdepth' and read 'nlb' from 'lba' via each context",
		"Create 'count' contexts with given 'qdepth' and read 'nlb' from 'lba' via each context", test_io_read, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_COUNT, XNVMEC_LREQ},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LREQ},
			{XNVMEC_OPT_LBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NLB, XNVMEC_LREQ},
		}
	},
};

static struct xnvmec cli = {
	.title = "Test xNVMe Asynchronous Interface",
	.descr_short = "Test xNVMe Asynchronous Interface",
	.subs = subs,
	.nsubs = sizeof(subs) / sizeof(subs[0]),
};

int
main(int argc, char **argv)
{
	return xnvmec(&cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
