// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <inttypes.h>
#include <libxnvme.h>

#define DEFAULT_QD 8

struct cb_args {
	uint32_t ecount;
	uint32_t completed;
	uint32_t submitted;
};

static void
cb_pool(struct xnvme_cmd_ctx *ctx, void *cb_arg)
{
	struct cb_args *cb_args = cb_arg;

	cb_args->completed += 1;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
		cb_args->ecount += 1;
	}

	xnvme_queue_put_cmd_ctx(ctx->async.queue, ctx);
}

/**
 * This example shows how to do asynchronous reads
 *
 * - Allocate buffers for Command Payload
 * - Setup a Command Queue
 *   | Callback functions and callback arguments
 *   | Using asynchronous command-contexts
 * - Submit read commands to read a range of LBAs continously
 *   | Re-submission when busy, reap completion, waiting till empty
 * - Dumping buffers to file
 * - Reporting on IO-errors
 * - Teardown
 */
static int
sub_async_read(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint64_t slba, naddrs;
	size_t nbytes;
	uint32_t nsid, qd;

	struct cb_args cb_args = {0};
	struct xnvme_queue *queue = NULL;

	char *buf = NULL, *payload = NULL;
	int err;

	if (geo->type != XNVME_GEO_CONVENTIONAL) {
		XNVME_DEBUG("FAILED: not nvm, got; %d", geo->type);
		return EINVAL;
	}

	qd = cli->given[XNVME_CLI_OPT_QDEPTH] ? cli->args.qdepth : DEFAULT_QD;
	nsid = cli->given[XNVME_CLI_OPT_NSID] ? cli->args.nsid : xnvme_dev_get_nsid(dev);

	if (cli->args.slba > cli->args.elba) {
		err = -EINVAL;
		xnvme_cli_perr("invalid range; slba > elba", err);
		return err;
	}
	slba = cli->args.slba;
	naddrs = cli->args.elba - cli->args.slba + 1;
	nbytes = naddrs * geo->lba_nbytes;

	xnvme_cli_pinf("Allocating and filling buf of nbytes: %zu", nbytes);
	buf = xnvme_buf_alloc(dev, nbytes);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_fill(buf, nbytes, "zero");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill()", err);
		goto exit;
	}

	xnvme_cli_pinf("Initializing queue and setting default callback function and arguments");
	err = xnvme_queue_init(dev, qd, 0, &queue);
	if (err) {
		xnvme_cli_perr("xnvme_queue_init()", err);
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_pool, &cb_args);

	xnvme_cli_pinf("Read uri: '%s', qd: %d", cli->args.uri, qd);
	xnvme_cli_pinf("Range: {slba: 0x%" PRIx64 ", naddrs: %" PRIu64 ", nbytes: %zu}", slba,
		       naddrs, nbytes);

	xnvme_cli_timer_start(cli);

	payload = buf;
	for (uint64_t curs = 0; (curs < naddrs) && !cb_args.ecount;) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

		struct xnvme_cmd_prep_nvm_read_args args = {
			.args_nbytes = sizeof(args),
			.nsid = nsid,
			.slba = slba + curs,
		};

		err = xnvme_cmd_prep_nvm_read(&ctx->cmd, &args);
		if (err) {
			xnvme_cli_perr("xnvme_cmd_prep_nvm_read()", err);
			xnvme_queue_put_cmd_ctx(queue, ctx);
			goto exit;
		}

submit:
		err = xnvme_cmd_pass(ctx, payload, geo->lba_nbytes, NULL, 0);
		switch (err) {
		case 0:
			cb_args.submitted += 1;
			goto next;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			xnvme_cli_perr("submission-error", err);
			xnvme_queue_put_cmd_ctx(queue, ctx);
			goto exit;
		}

next:
		++curs;
		payload += geo->lba_nbytes;
	}

	err = xnvme_queue_drain(queue);
	if (err < 0) {
		xnvme_cli_perr("xnvme_queue_drain()", err);
		goto exit;
	}

	xnvme_cli_timer_stop(cli);

	if (cb_args.ecount) {
		err = -EIO;
		xnvme_cli_perr("got completion errors", err);
		goto exit;
	}

	xnvme_cli_timer_bw_pr(cli, "wall-clock", naddrs * geo->lba_nbytes);

	if (cli->args.data_output) {
		xnvme_cli_pinf("Dumping nbytes: %zu, to: '%s'", nbytes, cli->args.data_output);

		err = xnvme_buf_to_file(buf, nbytes, cli->args.data_output);
		if (err) {
			xnvme_cli_perr("xnvme_buf_to_file()", err);
		}
	}

exit:
	xnvme_cli_pinf("cb_args: {submitted: %u, completed: %u, ecount: %u}", cb_args.submitted,
		       cb_args.completed, cb_args.ecount);

	if (queue) {
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			xnvme_cli_perr("xnvme_queue_term()", err_exit);
		}
	}
	xnvme_buf_free(dev, buf);

	return err < 0 ? err : 0;
}

/**
 * This example shows how to do asynchronous write
 *
 * - Allocate command-payload buffers
 * * - Setup a Command Queue
 *   | Using asynchronous command-contexts
 * - Submit write commands
 * - Reap their completion
 * - Teardown
 */
static int
sub_async_write(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint64_t slba, naddrs;
	size_t nbytes;
	uint32_t nsid, qd;

	struct cb_args cb_args = {0};
	struct xnvme_queue *queue = NULL;

	char *buf = NULL, *payload = NULL;
	int err;

	if (geo->type != XNVME_GEO_CONVENTIONAL) {
		XNVME_DEBUG("FAILED: not nvm, got; %d", geo->type);
		return EINVAL;
	}

	qd = cli->given[XNVME_CLI_OPT_QDEPTH] ? cli->args.qdepth : DEFAULT_QD;
	nsid = cli->given[XNVME_CLI_OPT_NSID] ? cli->args.nsid : xnvme_dev_get_nsid(cli->args.dev);

	if (cli->args.slba > cli->args.elba) {
		err = -EINVAL;
		xnvme_cli_perr("invalid range; slba > elba", err);
		return err;
	}
	slba = cli->args.slba;
	naddrs = cli->args.elba - cli->args.slba + 1;
	nbytes = naddrs * geo->lba_nbytes;

	xnvme_cli_pinf("Allocating and filling buf of nbytes: %zu", nbytes);
	buf = xnvme_buf_alloc(dev, nbytes);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_fill(buf, nbytes, cli->args.data_input ? cli->args.data_input : "anum");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill()", err);
		goto exit;
	}

	xnvme_cli_pinf("Initializing queue and setting default callback function and arguments");
	err = xnvme_queue_init(dev, qd, 0, &queue);
	if (err) {
		xnvme_cli_perr("xnvme_queue_init()", err);
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_pool, &cb_args);

	xnvme_cli_pinf("Write uri: '%s', qd: %d", cli->args.uri, qd);
	xnvme_cli_pinf("Range: {slba: 0x%" PRIx64 ", naddrs: %" PRIu64 ", nbytes: %zu}", slba,
		       naddrs, nbytes);

	xnvme_cli_timer_start(cli);

	payload = buf;
	for (uint64_t sect = 0; (sect < naddrs) && !cb_args.ecount;) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

		struct xnvme_cmd_prep_nvm_write_args args = {
			.args_nbytes = sizeof(args),
			.nsid = nsid,
			.slba = slba + sect,
		};

		err = xnvme_cmd_prep_nvm_write(&ctx->cmd, &args);
		if (err) {
			xnvme_cli_perr("xnvme_cmd_prep_nvm_write()", err);
			xnvme_queue_put_cmd_ctx(queue, ctx);
			goto exit;
		}

submit:
		err = xnvme_cmd_pass(ctx, payload, geo->lba_nbytes, NULL, 0);
		switch (err) {
		case 0:
			cb_args.submitted += 1;
			goto next;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			xnvme_cli_perr("submission-error", err);
			xnvme_queue_put_cmd_ctx(queue, ctx);
			goto exit;
		}

next:
		++sect;
		payload += geo->lba_nbytes;
	}

	err = xnvme_queue_drain(queue);
	if (err < 0) {
		xnvme_cli_perr("xnvme_queue_drain()", err);
		goto exit;
	}

	xnvme_cli_timer_stop(cli);

	if (cb_args.ecount) {
		err = -EIO;
		xnvme_cli_perr("got completion errors", err);
		goto exit;
	}

	xnvme_cli_timer_bw_pr(cli, "wall-clock", nbytes);

exit:
	xnvme_cli_pinf("cb_args: {submitted: %u, completed: %u, ecount: %u}", cb_args.submitted,
		       cb_args.completed, cb_args.ecount);

	if (queue) {
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			xnvme_cli_perr("xnvme_queue_term()", err_exit);
		}
	}
	xnvme_buf_free(dev, buf);

	return err < 0 ? err : 0;
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvme_cli_sub g_subs[] = {
	{
		"read",
		"Read the LBAs [SLBA,ELBA]",
		"Read the LBAs [SLBA,ELBA]",
		sub_async_read,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ELBA, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_SEED, XNVME_CLI_LOPT},

			XNVME_CLI_ASYNC_OPTS,
		},
	},

	{
		"write",
		"Write the LBAs [SLBA,ELBA]",
		"Write the LBAs [SLBA,ELBA]",
		sub_async_write,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ELBA, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_INPUT, XNVME_CLI_LOPT},

			XNVME_CLI_ASYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "xNVMe: Asynchronous IO Example",
	.descr_short = "xNVMe: Asynchronous IO Example: write, read",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
