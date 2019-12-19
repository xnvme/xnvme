// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <libxnvme.h>
#include <libxnvmec.h>
#include <libxnvme_util.h>
#include <libznd.h>
#include <time.h>

#define BE_QUEUE_SIZE 128

static int
arbitrary_zone(struct xnvmec *cli, uint64_t *zlba, enum znd_state state)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct znd_report *report = NULL;
	int err = 0;

	xnvmec_pinfo("Retrieving report ...");
	report = znd_report_from_dev(dev, 0x0, 0);
	if (!report) {
		xnvmec_perror("znd_report_from_dev()");
		return -1;
	}

	xnvmec_pinfo("Scan for a seq.-write-req. zone, in state: %s",
		     znd_state_str(state));

	err = znd_report_find_arbitrary(report, state, zlba, cli->args.seed);
	if (err) {
		xnvmec_pinfo("Could not find a zone");
	}

	xnvme_buf_virt_free(report);

	return err;
}

struct callback_args {
	uint32_t outstanding;
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

	const uint32_t depth = cli->args.qdepth ? cli->args.qdepth : 1;
	uint64_t zlba = cli->args.lba;
	uint16_t nlb = cli->args.nlb;
	size_t nsect = nlb + 1;
	int rcode = 0;

	const size_t buf_nbytes = geo->nbytes * geo->nsect;
	char *buf = NULL;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}
	if (!cli->given[XNVMEC_OPT_LBA]) {
		if (arbitrary_zone(cli, &zlba, ZND_STATE_FULL)) {
			return -1;
		}
	}

	xnvmec_pinfo("Allocating buffer ...");
	buf = xnvme_buf_alloc(dev, buf_nbytes, NULL);
	if (!buf) {
		xnvmec_perror("xnvme_buf_alloc()");
		rcode = 1;
		goto exit;
	}
	xnvmec_pinfo("Clearing it ...");
	xnvmec_buf_clear(buf, buf_nbytes);

	xnvmec_pinfo("uri: '%s'", cli->args.uri);
	xnvmec_pinfo("Read zlba: 0x%016x, nlb: %u, QD(%u)", zlba, nlb, depth);
	xnvmec_pinfo("nsect: %zu, nbytes: %zu", nsect, nsect * geo->nbytes);

	{
		// Asynchronous Reads
		int cmd_opts = XNVME_CMD_ASYNC;
		struct xnvme_async_ctx *ctx = NULL;
		struct callback_args callback_args = {
			.outstanding = 0,
			.errors = 0
		};

		struct xnvme_req *reqs = malloc(geo->nsect * sizeof(*reqs));
		int res = 0;

		// Initialize ASYNC CMD context
		ctx = xnvme_async_init(dev, BE_QUEUE_SIZE, 0x0);
		if (!ctx) {
			perror("could not initialize async context");
			free(reqs);
			goto exit;
		}
		for (size_t ri = 0; ri < geo->nsect; ++ri) {
			struct xnvme_req *req = &reqs[ri];

			memset(req, 0, sizeof(*req));
			req->async.ctx = ctx;
			req->async.cb = callback;
			req->async.cb_arg = &callback_args;
		}

		xnvmec_timer_start(cli);

		for (size_t sect = 0; sect < geo->nsect; sect += nsect) {
			const size_t csect = XNVME_MIN(geo->nsect - sect, nsect);
			const uint64_t off = sect * geo->nbytes;
			const uint64_t slba = zlba + sect;

			struct xnvme_req *req = &reqs[sect];
			int err = 0;

			if (callback_args.errors) {
				XNVME_DEBUG("too bad!");
				break;
			}

			++callback_args.outstanding;
			while (callback_args.outstanding > depth) {
				res = xnvme_async_poke(dev, ctx, 0);
				if (res < 0) {
					--callback_args.outstanding;
					xnvmec_perror("xnvme_async_poke()");
					free(reqs);
					goto exit;
				}
			}

			err = xnvme_cmd_read(dev, nsid, slba, csect - 1,
					     buf + off, NULL, cmd_opts,
					     req);
			if (err) {
				xnvmec_perror("xnvme_cmd_read()");
				xnvmec_pinfo("err: %x, s: %zu, slba: 0x%016x",
					     err, sect, slba);
				rcode = 1;
				--callback_args.outstanding;
				goto exit;
			}
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
			xnvmec_timer_bw_pr(cli, "Wall-clock-Read",
					   geo->nsect * geo->nbytes);
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

	const uint32_t depth = cli->args.qdepth ? cli->args.qdepth : 1;
	uint64_t zlba = cli->args.lba;
	uint16_t nlb = cli->args.nlb;
	size_t nsect = nlb + 1;

	const size_t buf_nbytes = geo->nbytes * geo->nsect;
	char *buf = NULL;
	int rcode = 0;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}
	if (!cli->given[XNVMEC_OPT_LBA]) {
		if (arbitrary_zone(cli, &zlba, ZND_STATE_EMPTY)) {
			return -1;
		}
	}

	xnvmec_pinfo("Allocating buffer ...");
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
	xnvmec_pinfo("Write zlba: 0x%016x, nlb: %u, QD(%u)", zlba, nlb, depth);
	xnvmec_pinfo("nsect: %zu, nbytes: %zu", nsect, nsect * geo->nbytes);

	{
		// Asynchronous Write
		int cmd_opts = XNVME_CMD_ASYNC;
		struct xnvme_async_ctx *ctx = NULL;
		struct callback_args callback_args = {
			.outstanding = 0,
			.errors = 0
		};

		struct xnvme_req *reqs = malloc(geo->nsect * sizeof(*reqs));
		int res = 0;

		// Initialize ASYNC CMD context
		ctx = xnvme_async_init(dev, BE_QUEUE_SIZE, 0x0);
		if (!ctx) {
			perror("could not initialize async context");
			free(reqs);
			return -1;
		}
		for (size_t ri = 0; ri < geo->nsect; ++ri) {
			struct xnvme_req *req = &reqs[ri];

			memset(req, 0, sizeof(*req));
			req->async.ctx = ctx;
			req->async.cb = callback;
			req->async.cb_arg = &callback_args;
		}

		xnvmec_timer_start(cli);

		for (size_t sect = 0; sect < geo->nsect; sect += nsect) {
			const size_t csect = XNVME_MIN(geo->nsect - sect, nsect);
			const uint64_t off = sect * geo->nbytes;
			const uint64_t slba = zlba + sect;

			struct xnvme_req *req = &reqs[sect];
			int err = 0;

			if (callback_args.errors) {
				XNVME_DEBUG("too bad!");
				break;
			}

			++callback_args.outstanding;
			while (callback_args.outstanding > depth) {
				res = xnvme_async_poke(dev, ctx, 0);
				if (res < 0) {
					--callback_args.outstanding;
					xnvmec_perror("xnvme_async_poke()");
					free(reqs);
					goto exit;
				}
			}

			err = xnvme_cmd_write(dev, nsid, slba, csect - 1,
					      buf + off, NULL, cmd_opts,
					      req);
			if (err) {
				xnvmec_perror("xnvme_cmd_write()");
				xnvmec_pinfo("err: %x, s: %zu, slba: 0x%016x",
					     err, sect, slba);
				rcode = 1;
				--callback_args.outstanding;
				goto exit;
			}
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
					   geo->nsect * geo->nbytes);
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

static int
sub_async_append(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid = cli->args.nsid;

	const uint32_t depth = cli->args.qdepth ? cli->args.qdepth : 1;
	uint64_t zlba = cli->args.lba;
	uint16_t nlb = cli->args.nlb;
	size_t nsect = nlb + 1;

	const size_t buf_nbytes = geo->nbytes * geo->nsect;
	char *buf = NULL;
	int rcode = 0;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}
	if (!cli->given[XNVMEC_OPT_LBA]) {
		if (arbitrary_zone(cli, &zlba, ZND_STATE_EMPTY)) {
			return -1;
		}
	}

	xnvmec_pinfo("Allocating buffer ...");
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
	xnvmec_pinfo("Append zlba: 0x%016x, nlb: %u, QD(%u)", zlba, nlb, depth);
	xnvmec_pinfo("nsect: %zu, nbytes: %zu", nsect, nsect * geo->nbytes);

	{
		// Asynchronous Append
		int cmd_opts = XNVME_CMD_ASYNC;
		struct xnvme_async_ctx *ctx = NULL;
		struct callback_args callback_args = {
			.outstanding = 0,
			.errors = 0
		};

		struct xnvme_req *reqs = malloc(geo->nsect * sizeof(*reqs));
		int res = 0;


		// Initialize ASYNC CMD context
		ctx = xnvme_async_init(dev, BE_QUEUE_SIZE, 0x0);
		if (!ctx) {
			perror("could not initialize async context");
			free(reqs);
			return -1;
		}
		for (size_t ri = 0; ri < geo->nsect; ++ri) {
			struct xnvme_req *req = &reqs[ri];

			memset(req, 0, sizeof(*req));
			req->async.ctx = ctx;
			req->async.cb = callback;
			req->async.cb_arg = &callback_args;
		}

		xnvmec_timer_start(cli);

		for (size_t sect = 0; sect < geo->nsect; sect += nsect) {
			const size_t csect = XNVME_MIN(geo->nsect - sect, nsect);
			const uint64_t off = sect * geo->nbytes;

			struct xnvme_req *req = &reqs[sect];
			int err = 0;

			if (callback_args.errors) {
				XNVME_DEBUG("too bad!");
				break;
			}

			++callback_args.outstanding;
			while (callback_args.outstanding > depth) {
				res = xnvme_async_poke(dev, ctx, 0);
				if (res < 0) {
					--callback_args.outstanding;
					xnvmec_perror("xnvme_async_poke()");
					free(reqs);
					goto exit;
				}
			}

			err = znd_cmd_append(dev, nsid, zlba, csect - 1,
					     buf + off, NULL, cmd_opts, req);
			if (err) {
				xnvmec_perror("znd_cmd_append()");
				xnvmec_pinfo("err: %x, s: %zu, zlba: 0x%016x",
					     err, sect, zlba);
				rcode = 1;
				--callback_args.outstanding;
				goto exit;
			}
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
			xnvmec_timer_bw_pr(cli, "Wall-clock-Append",
					   geo->nsect * geo->nbytes);
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
		"read", "Asynchronous Zone Read of an entire Zone",
		"Asynchronous Zone Read of an entire Zone", sub_async_read, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
			{XNVMEC_OPT_LBA, XNVMEC_LOPT},
			{XNVMEC_OPT_NLB, XNVMEC_LOPT},
			{XNVMEC_OPT_SEED, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},

	{
		"write", "Asynchronous Zone Write until full",
		"Zone asynchronous Write until full", sub_async_write, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
			{XNVMEC_OPT_LBA, XNVMEC_LOPT},
			{XNVMEC_OPT_NLB, XNVMEC_LOPT},
			{XNVMEC_OPT_SEED, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
		}
	},

	{
		"append", "Asynchronous Zone Append until full",
		"Zone asynchronous Append until full", sub_async_append, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
			{XNVMEC_OPT_LBA, XNVMEC_LOPT},
			{XNVMEC_OPT_NLB, XNVMEC_LOPT},
			{XNVMEC_OPT_SEED, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
		}
	},
};

static struct xnvmec cli = {
	.title = "Zoned Synchronous IO Example",
	.descr_short =	"Synchronous IO: read / write / append, "
	"using 4k payload at QD1",
	.subs = subs,
	.nsubs = sizeof(subs) / sizeof(subs[0]),
};

int
main(int argc, char **argv)
{
	return xnvmec(&cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
