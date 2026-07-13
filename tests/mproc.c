// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <signal.h>
#include <unistd.h>
#include <libxnvme.h>

static volatile sig_atomic_t g_stop;

static void
on_signal(int XNVME_UNUSED(sig))
{
	g_stop = 1;
}

static int
sub_primary(struct xnvme_cli *cli)
{
	signal(SIGTERM, on_signal);
	signal(SIGINT, on_signal);

	xnvme_cli_pinf("primary: device open at %s, waiting for SIGTERM/SIGINT", cli->args.uri);

	// Keep process alive, thereby keeping device open until process is killed.
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
