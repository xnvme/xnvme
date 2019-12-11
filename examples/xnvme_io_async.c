// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvmec.h>
#include <libxnvme_util.h>
#include <libznd.h>
#include <time.h>

#define DEFAULT_QD 8

static int
lba_range(struct xnvmec *cli, uint64_t *slba, uint64_t *elba)
{
	*slba = 0;
	*elba = *slba + ((1 << 27) / cli->args.geo->nbytes) - 1;

	return 0;
}

struct callback_args {
	uint32_t errors;
};

/**
 * Here you process the completed command, 'req' provides the return status of
 * the command and 'cb_arg' can have whatever it was given. In this case,
 * 'struct callback_args'
 */
static void
callback(struct xnvme_req *req, void *cb_arg)
{
	struct callback_args *callback_args = cb_arg;

	if (xnvme_req_cpl_status(req)) {
		xnvme_req_pr(req, XNVME_PR_DEF);
		callback_args->errors = 1;
	}
}

static int
sub_async_read(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid = cli->args.nsid;

	const uint32_t qd = cli->args.qdepth ? cli->args.qdepth : DEFAULT_QD;
	uint64_t slba = cli->args.slba;
	uint64_t elba = cli->args.elba;
	size_t nsect = 0;

	size_t buf_nbytes = 0;
	char *buf = NULL;
	int rcode = 0;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}
	if (!(cli->given[XNVMEC_OPT_SLBA] && cli->given[XNVMEC_OPT_ELBA])) {
		if (lba_range(cli, &slba, &elba)) {
			return -1;
		}
	}

	nsect = elba + 1 - slba;
	buf_nbytes = nsect * geo->nbytes;

	xnvmec_pinfo("Allocating buffer(%zu) ...", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes, NULL);
	if (!buf) {
		xnvmec_perror("xnvme_buf_alloc()");
		rcode = 1;
		goto exit;
	}
	xnvmec_pinfo("Clearing it ...");
	xnvmec_buf_clear(buf, buf_nbytes);

	xnvmec_pinfo("uri: '%s'", cli->args.uri);
	xnvmec_pinfo("Reading: [0x%016x,0x%016x], qd: %d", slba, elba, qd);

	{
		char *payload = buf;
		struct xnvme_async_ctx *ctx = NULL;
		struct callback_args callback_args = {
			.errors = 0
		};

		struct xnvme_req *reqs = malloc(nsect * sizeof(*reqs));
		int res = 0;

		// Initialize ASYNC CMD context
		ctx = xnvme_async_init(dev, DEFAULT_QD, 0x0);
		if (!ctx) {
			perror("could not initialize async context");
			free(reqs);
			return -1;
		}
		for (size_t ri = 0; ri < nsect; ++ri) {
			struct xnvme_req *req = &reqs[ri];

			memset(req, 0, sizeof(*req));
			req->async.ctx = ctx;
			req->async.cb = callback;
			req->async.cb_arg = &callback_args;
		}

		xnvmec_timer_start(cli);

		for (uint64_t sect = 0; sect < nsect; ++sect) {
			struct xnvme_req *req = &reqs[sect];
			int err = 0;

			if (callback_args.errors) {
				XNVME_DEBUG("previous reads-failed...");
				break;
			}

			do {
				err = xnvme_cmd_read(dev, nsid, slba + sect,
						     0, payload, NULL,
						     XNVME_CMD_ASYNC, req);
				if (!err) {
					break;
				}

				switch (errno) {
				case EAGAIN:
				case EBUSY:
					xnvme_async_poke(dev, ctx, 0);
					break;

				default:
					XNVME_DEBUG("the end is nigh...");
					rcode = 1;
					goto exit;
				}
			} while (1);

			payload += geo->nbytes;
		}

		xnvmec_pinfo("xnvme_async_wait: wait for outstanding commands");
		res = xnvme_async_wait(dev, ctx);
		if (res < 0) {
			xnvmec_perror("xnvme_async_wait()");
			rcode = 1;
			goto exit;
		}

		xnvmec_timer_stop(cli);
		if (!callback_args.errors) {
			xnvmec_timer_bw_pr(cli, "Wall-clock-READ",
					   nsect * geo->nbytes);
		}

		xnvmec_pinfo("xnvme_async_term: clean up async. ctx...");
		if (xnvme_async_term(dev, ctx)) {
			xnvmec_perror("xnvme_async_term()");
			rcode = 1;
			goto exit;
		}

		if (callback_args.errors) {
			xnvmec_pinfo("Got errors: %u", callback_args.errors);
			rcode = 1;
			goto exit;
		}
	}

exit:
	if (rcode) {
		xnvmec_pinfo("Completed WITH errors!");
	} else {
		xnvmec_pinfo("Completed successfully");
		if (cli->args.data_output) {
			xnvmec_pinfo("Dumping nbytes: %zu, to: '%s'",
				     buf_nbytes, cli->args.data_output);
			if (xnvmec_buf_to_file(buf, buf_nbytes,
					       cli->args.data_output)) {
				xnvmec_perror("xnvmec_buf_to_file()");
			}
		}
	}

	xnvme_buf_free(dev, buf);

	return rcode;
}


static int
sub_async_write(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid = cli->args.nsid;

	const uint32_t qd = cli->args.qdepth ? cli->args.qdepth : DEFAULT_QD;
	uint64_t slba = cli->args.slba;
	uint64_t elba = cli->args.elba;
	size_t nsect = 0;

	size_t buf_nbytes = 0;
	char *buf = NULL;
	int rcode = 0;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}
	if (!(cli->given[XNVMEC_OPT_SLBA] && cli->given[XNVMEC_OPT_ELBA])) {
		if (lba_range(cli, &slba, &elba)) {
			return -1;
		}
	}

	nsect = elba + 1 - slba;
	buf_nbytes = nsect * geo->nbytes;

	xnvmec_pinfo("Allocating buffer(%zu) ...", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes, NULL);
	if (!buf) {
		xnvmec_perror("xnvme_buf_alloc()");
		rcode = 1;
		goto exit;
	}
	if (cli->args.data_input) {
		xnvmec_pinfo("Filling buf with content of '%s'",
			     cli->args.data_input);
		if (xnvmec_buf_from_file(buf, buf_nbytes,
					 cli->args.data_input)) {
			xnvmec_perror("xnvmec_buf_from_file()");
			xnvme_buf_free(dev, buf);
			return -1;
		}
	} else {
		xnvmec_pinfo("Filling buf with random data");
		xnvmec_buf_fill(buf, buf_nbytes);
	}

	xnvmec_pinfo("uri: '%s'", cli->args.uri);
	xnvmec_pinfo("Writing: [0x%016x,0x%016x], qd: %d", slba, elba, qd);

	{
		char *payload = buf;
		struct xnvme_async_ctx *ctx = NULL;
		struct callback_args callback_args = {
			.errors = 0
		};

		struct xnvme_req *reqs = malloc(nsect * sizeof(*reqs));
		int res = 0;

		// Initialize ASYNC CMD context
		ctx = xnvme_async_init(dev, DEFAULT_QD, 0x0);
		if (!ctx) {
			perror("could not initialize async context");
			free(reqs);
			return -1;
		}
		for (size_t ri = 0; ri < nsect; ++ri) {
			struct xnvme_req *req = &reqs[ri];

			memset(req, 0, sizeof(*req));
			req->async.ctx = ctx;
			req->async.cb = callback;
			req->async.cb_arg = &callback_args;
		}

		xnvmec_timer_start(cli);

		for (uint64_t sect = 0; sect < nsect; ++sect) {
			struct xnvme_req *req = &reqs[sect];
			int err = 0;

			if (callback_args.errors) {
				XNVME_DEBUG("previous writes-failed...");
				break;
			}

			do {
				err = xnvme_cmd_write(dev, nsid, slba + sect,
						      0, payload, NULL,
						      XNVME_CMD_ASYNC, req);
				if (!err) {
					break;
				}

				switch (errno) {
				case EAGAIN:
				case EBUSY:
					xnvme_async_poke(dev, ctx, 0);
					break;

				default:
					XNVME_DEBUG("the end is nigh...");
					rcode = 1;
					goto exit;
				}
			} while (1);

			payload += geo->nbytes;
		}

		xnvmec_pinfo("xnvme_async_wait: wait for outstanding commands");
		res = xnvme_async_wait(dev, ctx);
		if (res < 0) {
			xnvmec_perror("xnvme_async_wait()");
			rcode = 1;
			goto exit;
		}

		xnvmec_timer_stop(cli);
		if (!callback_args.errors) {
			xnvmec_timer_bw_pr(cli, "Wall-clock-WRITE",
					   nsect * geo->nbytes);
		}

		xnvmec_pinfo("xnvme_async_term: clean up async. ctx...");
		if (xnvme_async_term(dev, ctx)) {
			xnvmec_perror("xnvme_async_term()");
			rcode = 1;
			goto exit;
		}

		if (callback_args.errors) {
			xnvmec_pinfo("Got errors: %u", callback_args.errors);
			rcode = 1;
			goto exit;
		}
	}

exit:
	if (rcode) {
		xnvmec_pinfo("Completed WITH errors!");
	} else {
		xnvmec_pinfo("Completed successfully");
	}

	xnvme_buf_free(dev, buf);

	return rcode;
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvmec_sub subs[] = {
	{
		"read",
		"Read the LBAs [SLBA,ELBA]",
		"Read the LBAs [SLBA,ELBA]",
		sub_async_read, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
			{XNVMEC_OPT_SLBA, XNVMEC_LOPT},
			{XNVMEC_OPT_ELBA, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_SEED, XNVMEC_LOPT},
		}
	},

	{
		"write",
		"Write the LBAs [SLBA,ELBA]",
		"Write the LBAs [SLBA,ELBA]",
		sub_async_write, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
			{XNVMEC_OPT_SLBA, XNVMEC_LOPT},
			{XNVMEC_OPT_ELBA, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
		}
	},
	/*
		{
			"verify",
			"Write, then Read the LBAs [SLBA,ELBA], and Verify",
			"Write, then Read the LBAs [SLBA,ELBA], and Verify",
			sub_async_write, {
				{XNVMEC_OPT_URI, XNVMEC_POSA},
				{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
				{XNVMEC_OPT_SLBA, XNVMEC_LOPT},
				{XNVMEC_OPT_ELBA, XNVMEC_LOPT},
				{XNVMEC_OPT_SEED, XNVMEC_LOPT},
			}
		},*/
};

static struct xnvmec cli = {
	.title = "xNVMe: Asynchronous IO Example",
	.descr_short = "xNVMe: Asynchronous IO Example: write, read, and verify",
	.subs = subs,
	.nsubs = sizeof(subs) / sizeof(subs[0]),
};

int
main(int argc, char **argv)
{
	return xnvmec(&cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
