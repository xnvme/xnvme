// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <pthread.h>
#include <libxnvme.h>

#define THREADS 8

struct worker {
	struct xnvme_cli *cli;
	struct xnvme_opts opts;
	struct xnvme_dev *dev;
	uint64_t rounds;
	int nerr;
	int idx;
};

static void *
open_close_fn(void *arg)
{
	struct worker *w = arg;

	for (uint64_t round = 0; round < w->rounds; ++round) {
		struct xnvme_dev *dev = xnvme_dev_open(w->cli->args.uri, &w->opts);

		if (!dev) {
			w->nerr += 1;
			continue;
		}
		xnvme_dev_close(dev);
	}

	return NULL;
}

static void *
init_term_fn(void *arg)
{
	struct worker *w = arg;

	for (uint64_t round = 0; round < w->rounds; ++round) {
		struct xnvme_queue *queue = NULL;

		if (xnvme_queue_init(w->dev, w->cli->args.qdepth, 0, &queue)) {
			w->nerr += 1;
			continue;
		}
		if (xnvme_queue_term(queue)) {
			w->nerr += 1;
		}
	}

	return NULL;
}

static void *
idfy_fn(void *arg)
{
	struct worker *w = arg;
	struct xnvme_spec_idfy *idfy;

	idfy = xnvme_buf_alloc(w->dev, sizeof(*idfy));
	if (!idfy) {
		w->nerr += 1;
		return NULL;
	}
	for (uint64_t round = 0; round < w->rounds; ++round) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(w->dev);

		if (xnvme_adm_idfy_ctrlr(&ctx, idfy) || xnvme_cmd_ctx_cpl_status(&ctx)) {
			w->nerr += 1;
		}
	}
	xnvme_buf_free(w->dev, idfy);

	return NULL;
}

/**
 * Half the threads send admin commands while the other half create and delete queues, which
 * the controller also does through its admin queue
 */
static void *
admin_fn(void *arg)
{
	struct worker *w = arg;

	return (w->idx % 2) ? init_term_fn(arg) : idfy_fn(arg);
}

static int
run_workers(struct xnvme_cli *cli, struct xnvme_dev *dev, void *(*fn)(void *), int nthreads)
{
	struct worker workers[THREADS];
	pthread_t tids[THREADS];
	int nerr = 0;
	int err;

	for (int i = 0; i < nthreads; ++i) {
		workers[i] = (struct worker){
			.cli = cli, .dev = dev, .rounds = cli->args.count, .idx = i};
		err = xnvme_cli_to_opts(cli, &workers[i].opts);
		if (err) {
			xnvme_cli_perr("xnvme_cli_to_opts()", err);
			return err;
		}
	}
	for (int i = 0; i < nthreads; ++i) {
		err = pthread_create(&tids[i], NULL, fn, &workers[i]);
		if (err) {
			xnvme_cli_perr("pthread_create()", -err);
			for (int j = 0; j < i; ++j) {
				pthread_join(tids[j], NULL);
			}
			return -err;
		}
	}
	for (int i = 0; i < nthreads; ++i) {
		pthread_join(tids[i], NULL);
		nerr += workers[i].nerr;
	}

	if (nerr) {
		xnvme_cli_pinf("FAILED: %d of %d threads x %zu rounds", nerr, nthreads,
			       cli->args.count);
		return -EIO;
	}
	xnvme_cli_pinf("LGTM: %d threads x %zu rounds", nthreads, cli->args.count);

	return 0;
}

static int
test_open_close(struct xnvme_cli *cli)
{
	return run_workers(cli, NULL, open_close_fn, THREADS);
}

/**
 * Each open follows a close that released the controller, so a backend has to bring the
 * controller up again right after tearing it down
 */
static int
test_reopen(struct xnvme_cli *cli)
{
	return run_workers(cli, NULL, open_close_fn, 1);
}

static int
test_queues(struct xnvme_cli *cli, void *(*fn)(void *))
{
	struct xnvme_opts opts = xnvme_opts_default();
	struct xnvme_dev *dev;
	int nthreads = THREADS;
	int err;

	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}
	dev = xnvme_dev_open(cli->args.uri, &opts);
	if (!dev) {
		err = -errno;
		xnvme_cli_perr("xnvme_dev_open()", err);
		return err;
	}

	{
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		struct xnvme_spec_feat feat = {.val = 0};

		err = xnvme_adm_gfeat(&ctx, 0x0, XNVME_SPEC_FEAT_NQUEUES,
				      XNVME_SPEC_FEAT_SEL_CURRENT, NULL, 0);
		if (!err && !xnvme_cmd_ctx_cpl_status(&ctx)) {
			feat.val = ctx.cpl.cdw0;
			nthreads = XNVME_MIN(THREADS, (int)feat.nqueues.nsqa);
		}
	}
	if (nthreads < 1) {
		xnvme_cli_pinf("SKIPPED: the device has no I/O queue to spare");
		xnvme_dev_close(dev);
		return 0;
	}

	err = run_workers(cli, dev, fn, nthreads);

	xnvme_dev_close(dev);

	return err;
}

static int
test_init_term(struct xnvme_cli *cli)
{
	return test_queues(cli, init_term_fn);
}

static int
test_admin(struct xnvme_cli *cli)
{
	return test_queues(cli, admin_fn);
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"open_close",
		"Open and close 'uri' from 8 threads at once, 'count' rounds each",
		"Open and close 'uri' from 8 threads at once, 'count' rounds each",
		test_open_close,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LREQ},
			XNVME_CLI_ASYNC_OPTS,
		},
	},
	{
		"reopen",
		"Open and close 'uri' 'count' times, each open right after the previous close",
		"Open and close 'uri' 'count' times, each open right after the previous close",
		test_reopen,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LREQ},
			XNVME_CLI_ASYNC_OPTS,
		},
	},
	{
		"init_term",
		"Create and destroy a queue on 'uri' from 8 threads at once, 'count' rounds each",
		"Create and destroy a queue on 'uri' from 8 threads at once, 'count' rounds each",
		test_init_term,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LREQ},
			XNVME_CLI_ASYNC_OPTS,
		},
	},
	{
		"admin",
		"Identify 'uri' from 4 threads while 4 others create and destroy queues on it",
		"Identify 'uri' from 4 threads while 4 others create and destroy queues on it",
		test_admin,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LREQ},
			XNVME_CLI_ASYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Test concurrent use of the device and queue lifecycle",
	.descr_short = "Test concurrent use of the device and queue lifecycle",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
