// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

static int
sub_open(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	int err;

	xnvme_cli_pinf("Before xnvme_dev_derive_geo()");
	xnvme_dev_pr(dev, XNVME_PR_DEF);

	err = xnvme_dev_derive_geo(dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_dev_derive_geo(), err: %d", err);
		return err;
	}

	xnvme_cli_pinf("After xnvme_dev_derive_geo()");
	xnvme_dev_pr(dev, XNVME_PR_DEF);

	return 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"open",
		"Open and print info, then derive and print",
		"Open and print info, then derive and print",
		sub_open,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_SYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Open without geo-derivation",
	.descr_short = "Open without geo-derivation",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
