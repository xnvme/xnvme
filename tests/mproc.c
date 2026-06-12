// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <signal.h>
#include <unistd.h>
#include <libxnvme.h>

#define MPROC_QUEUE_DEPTH 256

static volatile sig_atomic_t g_stop;

static void
on_signal(int XNVME_UNUSED(sig))
{
	g_stop = 1;
}

static int
sub_primary(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint32_t nqueues = cli->args.nqueues ? cli->args.nqueues : 1;
	int err;

	signal(SIGTERM, on_signal);
	signal(SIGINT, on_signal);

	for (uint32_t i = 0; i < nqueues; i++) {
		struct xnvme_queue *queue = NULL;

		err = xnvme_queue_init(dev, MPROC_QUEUE_DEPTH, 0, &queue);
		if (err) {
			xnvme_cli_perr("xnvme_queue_init()", err);
			return err;
		}

		err = xnvme_queue_term(queue);
		if (err) {
			xnvme_cli_perr("xnvme_queue_term()", err);
			return err;
		}
	}

	xnvme_cli_pinf(
		"primary: device open at %s, pool has %u queue(s), waiting for SIGTERM/SIGINT",
		cli->args.uri, nqueues);

	while (!g_stop) {
		sleep(1);
	}

	return 0;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"primary",
		"Open device as primary and wait, allowing secondary processes to attach",
		"Open device as primary and wait, allowing secondary processes to attach",
		sub_primary,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NQUEUES, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Multi-Process Primary",
	.descr_short = "Multi-Process Primary",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
