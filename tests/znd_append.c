// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_spec_pp.h>
#include <libxnvme_nvm.h>
#include <libxnvme_znd.h>
#include <libxnvmec.h>

struct cb_args {
	uint64_t zslba;
	uint32_t ecount_offset;
	uint32_t ecount;
	uint32_t completed;
	uint32_t submitted;
};

static void
cb_lbacheck(struct xnvme_req *req, void *cb_arg)
{
	struct cb_args *cb_args = cb_arg;
	uint64_t expected = cb_args->zslba + cb_args->completed;

	cb_args->completed += 1;

	if (xnvme_req_cpl_status(req)) {
		xnvme_req_pr(req, XNVME_PR_DEF);
		cb_args->ecount += 1;
	}

	if (req->cpl.result != expected) {
		xnvmec_pinf("ERR: req->cpl.result: 0x%016lx != 0x%016lx",
			    req->cpl.result, expected);
		cb_args->ecount_offset += 1;
	}
}

static int
cmd_verify(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid = cli->args.nsid;
	struct xnvme_spec_znd_descr zone = {0 };

	int cmd_opts = XNVME_CMD_ASYNC;
	struct cb_args cb_args = { 0 };
	struct xnvme_async_ctx *ctx = NULL;

	size_t buf_nbytes;
	void *dbuf = NULL, *vbuf = NULL;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}
	if (cli->args.clear) {
		err = -EINVAL;
		xnvmec_perr("This test does not support XNVME_CMD_SYNC", -err);
		return err;
	}

	// Find an empty zone
	err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_EMPTY, &zone);
	if (err) {
		xnvmec_perr("xnvme_znd_descr_from_dev()", -err);
		goto exit;
	}
	xnvmec_pinf("Using the following zone:");
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	// Buffers for verification
	buf_nbytes = zone.zcap * geo->lba_nbytes;
	xnvmec_pinf("Allocating buffers...");
	dbuf = xnvme_buf_alloc(dev, buf_nbytes, NULL);
	if (!dbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	vbuf = xnvme_buf_alloc(dev, buf_nbytes, NULL);
	if (!vbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	xnvmec_buf_fill(dbuf, buf_nbytes, "anum");
	xnvmec_buf_fill(vbuf, buf_nbytes, "zero");

	xnvmec_pinf("Using XNVME_CMD_ASYNC mode");

	xnvmec_pinf("Initializing async. context + alloc/init requests");
	err = xnvme_async_init(dev, &ctx, 2, 0);
	if (err) {
		xnvmec_perr("xnvme_async_init()", -err);
		goto exit;
	}

	xnvmec_timer_start(cli);

	cb_args.zslba = zone.zslba;

	for (uint64_t sect = 0; (sect < zone.zcap) && !cb_args.ecount; ++sect) {
		struct xnvme_req req = { 0 };

		req.async.ctx = ctx;
		req.async.cb = cb_lbacheck;
		req.async.cb_arg = &cb_args;

		err = xnvme_znd_append(dev, nsid, zone.zslba, 0,
				       dbuf + sect * geo->lba_nbytes, NULL,
				       cmd_opts, &req);
		switch (err) {
		case 0:
			cb_args.submitted += 1;
			break;

		case -EBUSY:
		case -EAGAIN:
			xnvmec_perr("is busy, should not be possible!", err);
			goto exit;

		default:
			xnvmec_perr("submission-error", EIO);
			goto exit;
		}

		err = xnvme_async_wait(dev, ctx);
		if (err < 0) {
			xnvmec_perr("xnvme_async_wait()", err);
			goto exit;
		}
	}

	xnvmec_timer_stop(cli);
	xnvmec_timer_bw_pr(cli, "Wall-clock", geo->nsect * geo->nbytes);

	if (cb_args.ecount) {
		err = -EIO;
		xnvmec_perr("got completion errors", err);
		goto exit;
	}
	if (cb_args.ecount_offset) {
		err = -EIO;
		xnvmec_perr("got offset error in completion-result", err);
		goto exit;
	}

	// Read zone content into vbuf
	for (uint64_t sect = 0; sect < zone.zcap; ++sect) {
		struct xnvme_req req = { 0 };

		err = xnvme_nvm_read(dev, nsid, zone.zslba + sect, 0,
				     vbuf + sect * geo->lba_nbytes, NULL,
				     XNVME_CMD_SYNC, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			xnvmec_perr("xnvme_nvm_read()", err);
			xnvme_req_pr(&req, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
	}

	// Verify
	{
		size_t diff;

		diff = xnvmec_buf_diff(dbuf, vbuf, buf_nbytes);
		if (diff) {
			xnvmec_pinf("verification failed, diff: %zu", diff);
			xnvmec_buf_diff_pr(dbuf, vbuf, buf_nbytes, XNVME_PR_DEF);
			err = -EIO;
			goto exit;
		}
	}

	xnvmec_pinf("LGTM");

exit:
	xnvmec_pinf("cb_args: {submitted: %u, completed: %u, ecount: %u}",
		    cb_args.submitted, cb_args.completed,
		    cb_args.ecount);

	{
		int err_exit = xnvme_async_term(dev, ctx);
		if (err_exit) {
			xnvmec_perr("xnvme_async_term()", err_exit);
		}
	}
	xnvme_buf_free(dev, dbuf);
	xnvme_buf_free(dev, vbuf);

	return err < 0 ? err : 0;
}


//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub g_subs[] = {
	{
		"verify",
		"Fills a Zone one LBA at a time, checking addr on completion",
		"Fills a Zone one LBA at a time, checking addr on completion",
		cmd_verify, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_CLEAR, XNVMEC_LFLG},
		}
	},
};

static struct xnvmec g_cli = {
	.title = "Tests for Zoned Random Write Area",
	.descr_short = "Tests for Zoned Random Write Area",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
