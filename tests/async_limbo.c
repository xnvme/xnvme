// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause
#ifdef XNVME_PLATFORM_LINUX_ENABLED
#include <errno.h>
#include <libxnvme.h>
#include <signal.h>
#include <unistd.h>

#define QUEUE_DEPTH 4

volatile sig_atomic_t run = 1;

void
handle_sigint(int XNVME_UNUSED(sig))
{
	run = 0;
}

struct ioctx {
	void *buf;
	struct xnvme_cmd_ctx *ctx;

	uint64_t count;
};

static void
on_completion(struct xnvme_cmd_ctx *ctx, void *cb_arg)
{
	struct ioctx *ioctx = cb_arg;
	int err;

	assert(ctx == ioctx->ctx);

	ioctx->count += 1;

	err = xnvme_cmd_ctx_cpl_status(ctx);
	if (err) {
		xnvme_cli_perr("on_completion()", errno);
		xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
		xnvme_cli_pinf("Stopping... due to completion-error.");
		goto stop;
	}

	if (!run) {
		xnvme_cli_pinf("Stopping... due to signal.");
		goto stop;
	}

	err = xnvme_nvm_read(ctx, 0x1, 0x1, 0x0, ioctx->buf, NULL);
	if (err) {
		xnvme_cli_pinf("Stopping... due to submisson-error.");
		goto stop;
	}

	return;

stop:
	xnvme_queue_put_cmd_ctx(ctx->async.queue, ctx);
}

static int
test_randr(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct ioctx per_callback[QUEUE_DEPTH] = {0};
	struct xnvme_queue *queue;
	int err;

	err = xnvme_queue_init(dev, QUEUE_DEPTH, 0x0, &queue);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_queue_init()");
		return err;
	}

	for (int i = 0; i < (QUEUE_DEPTH); ++i) {
		struct ioctx *ioctx = &per_callback[i];

		ioctx->count = 0;

		ioctx->buf = xnvme_buf_alloc(dev, 4096);
		if (!ioctx->buf) {
			XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
			return -EIO;
		}

		ioctx->ctx = xnvme_queue_get_cmd_ctx(queue);
		if (!ioctx->ctx) {
			XNVME_DEBUG("FAILED: xnvme_queue_get_cmd_ctx");
			return -ENOMEM;
		}
		ioctx->ctx->async.cb = on_completion;
		ioctx->ctx->async.cb_arg = ioctx;

		err = xnvme_nvm_read(ioctx->ctx, 0x1, 0x0, 0x0, ioctx->buf, NULL);
		if (err) {
			XNVME_DEBUG("FAILED: populating queue");
			return -EIO;
		}
	}

	printf("Running indefinitely. Press Ctrl+C to terminate.\n");

	while (xnvme_queue_get_outstanding(queue)) {
		int res;

		res = xnvme_queue_poke(queue, QUEUE_DEPTH);
		if (res >= 0) {
			XNVME_DEBUG("INFO: processed res(%d)..", res);
			continue;
		}

		switch (res) {
		case -EBUSY:
		case -EAGAIN:
			XNVME_DEBUG("INFO: .. busy...");
			break;
		default:
			XNVME_DEBUG("FAILED: poke(); res(%d)", res);
			run = 0;
			break;
		}
	}

	xnvme_queue_term(queue);

	for (int i = 0; i < QUEUE_DEPTH; ++i) {
		struct ioctx *ioctx = &per_callback[i];

		printf("stats: i(%d), count(%" PRIi64 ")\n", i, ioctx->count);
	}

	return 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"randr",
		"Do a bunch of random read via a queue",
		"Do a bunch of random read via a queue",
		test_randr,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LREQ},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Submit I/O until interrupted by signal (e.g. Ctrl + C)",
	.descr_short = "Submit I/O until interrupted by signal",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	struct sigaction sa = {0};
	sa.sa_handler = handle_sigint;
	sigaction(SIGINT, &sa, NULL);

	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
#else
int
main(int argc, char **argv)
{
	return 0;
};
#endif