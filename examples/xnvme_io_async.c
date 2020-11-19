// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_nvm.h>
#include <libxnvmec.h>
#include <libxnvme_util.h>
#include <time.h>

#define DEFAULT_QD 8

static int
lba_range(struct xnvmec *cli, uint64_t *slba, uint64_t *elba)
{
	*slba = 0;
	*elba = *slba + ((1 << 27) / cli->args.geo->nbytes) - 1;

	return 0;
}

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

	SLIST_INSERT_HEAD(&ctx->pool->head, ctx, link);
}

/**
 * This example shows how to do asynchronous reads
 *
 * - Allocate command-payload buffers
 * - Setup async. context
 * - Setup request pool
 * - Submit read commands
 * - Reap their completion
 * - Teardown
 */
static int
sub_async_read(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid = cli->args.nsid;

	const uint32_t qd = cli->args.qdepth ? cli->args.qdepth : DEFAULT_QD;
	uint64_t slba = cli->args.slba;
	uint64_t elba = cli->args.elba;
	size_t nsect;

	struct cb_args cb_args = { 0 };
	struct xnvme_queue *queue = NULL;
	struct xnvme_cmd_ctx_pool *pool = NULL;

	size_t buf_nbytes;
	char *buf = NULL;
	char *payload = NULL;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}
	if (!(cli->given[XNVMEC_OPT_SLBA] && cli->given[XNVMEC_OPT_ELBA])) {
		err = lba_range(cli, &slba, &elba);
		if (err) {
			xnvmec_perr("lba_range()", err);
			goto exit;
		}
	}

	nsect = elba + 1 - slba;
	buf_nbytes = nsect * geo->nbytes;

	xnvmec_pinf("Allocating and filling buf_nbytes: %zu", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes, NULL);
	if (!buf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvmec_buf_fill(buf, buf_nbytes, "zero");
	if (err) {
		xnvmec_perr("xnvmec_buf_fill()", err);
		goto exit;
	}

	xnvmec_pinf("Initializing async. context + alloc/init requests");
	err = xnvme_queue_init(dev, qd, 0, &queue);
	if (err) {
		xnvmec_perr("xnvme_queue_init()", err);
		goto exit;
	}
	err = xnvme_cmd_ctx_pool_alloc(&pool, qd + 1);
	if (err) {
		xnvmec_perr("xnvme_cmd_ctx_pool_alloc()", err);
		goto exit;
	}
	err = xnvme_cmd_ctx_pool_init(pool, queue, cb_pool, &cb_args);
	if (err) {
		xnvmec_perr("xnvme_cmd_ctx_pool_init()", err);
		goto exit;
	}

	xnvmec_pinf("Read nsect: %zu, [0x%016lx,0x%016lx], qd: %d, uri: '%s'",
		    nsect, slba, elba, qd, cli->args.uri);

	xnvmec_timer_start(cli);

	payload = buf;
	for (uint64_t sect = 0; (sect < nsect) && !cb_args.ecount;) {
		struct xnvme_cmd_ctx *ctx = SLIST_FIRST(&pool->head);

		SLIST_REMOVE_HEAD(&pool->head, link);

submit:
		err = xnvme_nvm_read(dev, nsid, slba + sect, 0, payload,
				     NULL, XNVME_CMD_ASYNC, ctx);
		switch (err) {
		case 0:
			cb_args.submitted += 1;
			goto next;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			xnvmec_perr("submission-error", EIO);
			goto exit;
		}

next:
		++sect;
		payload += geo->nbytes;
	}

	err = xnvme_queue_wait(queue);
	if (err < 0) {
		xnvmec_perr("xnvme_queue_wait()", err);
		goto exit;
	}

	xnvmec_timer_stop(cli);

	if (cb_args.ecount) {
		err = -EIO;
		xnvmec_perr("got completion errors", err);
		goto exit;
	}

	xnvmec_timer_bw_pr(cli, "wall-clock", nsect * geo->nbytes);

	if (cli->args.data_output) {
		xnvmec_pinf("Dumping nbytes: %zu, to: '%s'",
			    buf_nbytes, cli->args.data_output);
		err = xnvmec_buf_to_file(buf, buf_nbytes,
					 cli->args.data_output);
		if (err) {
			xnvmec_perr("xnvmec_buf_to_file()", err);
		}
	}

exit:
	xnvmec_pinf("cb_args: {submitted: %u, completed: %u, ecount: %u}",
		    cb_args.submitted, cb_args.completed,
		    cb_args.ecount);

	{
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			xnvmec_perr("xnvme_queue_term()", err_exit);
		}
	}
	xnvme_cmd_ctx_pool_free(pool);
	xnvme_buf_free(dev, buf);

	return err < 0 ? err : 0;
}

/**
 * This example shows how to do asynchronous write
 *
 * - Allocate command-payload buffers
 * - Setup queue
 * - Setup command-context pool
 * - Submit write commands
 * - Reap their completion
 * - Teardown
 */
static int
sub_async_write(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid = cli->args.nsid;

	const uint32_t qd = cli->args.qdepth ? cli->args.qdepth : DEFAULT_QD;
	uint64_t slba = cli->args.slba;
	uint64_t elba = cli->args.elba;
	size_t nsect;

	struct cb_args cb_args = { 0 };
	struct xnvme_queue *queue = NULL;
	struct xnvme_cmd_ctx_pool *pool = NULL;

	size_t buf_nbytes;
	char *buf = NULL;
	char *payload = NULL;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}
	if (!(cli->given[XNVMEC_OPT_SLBA] && cli->given[XNVMEC_OPT_ELBA])) {
		err = lba_range(cli, &slba, &elba);
		if (err) {
			xnvmec_perr("lba_range()", err);
			goto exit;
		}
	}

	nsect = elba + 1 - slba;
	buf_nbytes = nsect * geo->nbytes;

	xnvmec_pinf("Allocating and filling buf_nbytes: %zu", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes, NULL);
	if (!buf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvmec_buf_fill(buf, buf_nbytes, cli->args.data_input ? cli->args.data_input : "anum");
	if (err) {
		xnvmec_perr("xnvmec_buf_fill()", err);
		goto exit;
	}

	xnvmec_pinf("Initializing queue and alloc/init ctx-pool");
	err = xnvme_queue_init(dev, qd, 0, &queue);
	if (err) {
		xnvmec_perr("xnvme_queue_init()", err);
		goto exit;
	}
	err = xnvme_cmd_ctx_pool_alloc(&pool, qd + 1);
	if (err) {
		xnvmec_perr("xnvme_cmd_ctx_pool_alloc()", err);
		goto exit;
	}
	err = xnvme_cmd_ctx_pool_init(pool, queue, cb_pool, &cb_args);
	if (err) {
		xnvmec_perr("xnvme_cmd_ctx_pool_init()", err);
		goto exit;
	}

	xnvmec_pinf("Write nsect: %zu, [0x%016lx,0x%016lx], qd: %d, uri: '%s'",
		    nsect, slba, elba, qd, cli->args.uri);

	xnvmec_timer_start(cli);

	payload = buf;
	for (uint64_t sect = 0; (sect < nsect) && !cb_args.ecount;) {
		struct xnvme_cmd_ctx *ctx = SLIST_FIRST(&pool->head);

		SLIST_REMOVE_HEAD(&pool->head, link);

submit:
		err = xnvme_nvm_write(dev, nsid, slba + sect, 0, payload,
				      NULL, XNVME_CMD_ASYNC, ctx);
		switch (err) {
		case 0:
			cb_args.submitted += 1;
			goto next;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			xnvmec_perr("submission-error", EIO);
			goto exit;
		}

next:
		++sect;
		payload += geo->nbytes;
	}

	err = xnvme_queue_wait(queue);
	if (err < 0) {
		xnvmec_perr("xnvme_queue_wait()", err);
		goto exit;
	}

	xnvmec_timer_stop(cli);

	if (cb_args.ecount) {
		err = -EIO;
		xnvmec_perr("got completion errors", err);
		goto exit;
	}

	xnvmec_timer_bw_pr(cli, "wall-clock", nsect * geo->nbytes);

exit:
	xnvmec_pinf("cb_args: {submitted: %u, completed: %u, ecount: %u}",
		    cb_args.submitted, cb_args.completed,
		    cb_args.ecount);

	{
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			xnvmec_perr("xnvme_queue_term()", err_exit);
		}
	}
	xnvme_cmd_ctx_pool_free(pool);
	xnvme_buf_free(dev, buf);

	return err < 0 ? err : 0;
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvmec_sub g_subs[] = {
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
};

static struct xnvmec g_cli = {
	.title = "xNVMe: Asynchronous IO Example",
	.descr_short = "xNVMe: Asynchronous IO Example: write, read, and verify",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
