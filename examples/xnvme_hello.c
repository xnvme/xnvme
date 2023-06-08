// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

static int
sub_hw_example(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = xnvme_opts_default();
	struct xnvme_dev *dev = NULL;

	if (xnvme_cli_to_opts(cli, &opts)) {
		xnvme_cli_perr("xnvme_cli_to_opts()", errno);
		return -errno;
	}

	dev = xnvme_dev_open(cli->args.uri, &opts);
	if (!dev) {
		xnvme_cli_perr("xnvme_dev_open()", errno);
		return -errno;
	}

	xnvme_dev_pr(dev, XNVME_PR_DEF);
	xnvme_dev_close(dev);

	return 0;
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvme_cli_sub g_subs[] = {
	{
		"hw",
		"Hello-World Example",
		"Hello-World Example",
		sub_hw_example,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "xNVMe hello-device example",
	.descr_short = "Open the given device and print its attributes",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
