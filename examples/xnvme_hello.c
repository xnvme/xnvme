// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvmec.h>

static int
sub_hw_example(struct xnvmec *cli)
{
	struct xnvme_opts opts = xnvme_opts_default();
	struct xnvme_dev *dev = NULL;

	if (xnvmec_cli_to_opts(cli, &opts)) {
		xnvmec_perr("xnvmec_cli_to_opts()", errno);
		return -errno;
	}

	dev = xnvme_dev_open(cli->args.uri, &opts);
	if (!dev) {
		xnvmec_perr("xnvme_dev_open()", errno);
		return -errno;
	}

	xnvme_dev_pr(dev, XNVME_PR_DEF);
	xnvme_dev_close(dev);

	return 0;
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvmec_sub g_subs[] = {
	{
		"hw",
		"Hello-World Example",
		"Hello-World Example",
		sub_hw_example,
		{
			{XNVMEC_OPT_POSA_TITLE, XNVMEC_SKIP},
			{XNVMEC_OPT_URI, XNVMEC_POSA},

			XNVMEC_ADMIN_OPTS,
		},
	},
};

static struct xnvmec g_cli = {
	.title = "xNVMe hello-device example",
	.descr_short = "Open the given device and print its attributes",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_NONE);
}
