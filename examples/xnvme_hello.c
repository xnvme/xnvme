// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <libxnvme.h>
#include <libxnvmec.h>

static int
sub_hw_example(struct xnvmec *cli)
{
	struct xnvme_dev *dev = NULL;

	dev = xnvme_dev_open(cli->args.uri);
	if (!dev) {
		xnvmec_perror("xnvme_dev_open");
		return 1;
	}

	xnvme_dev_pr(dev, XNVME_PR_DEF);
	xnvme_dev_close(dev);

	return 0;
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvmec_sub subs[] = {
	{
		"hw", "Hello-World Example", "Hello-World Example", sub_hw_example, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
		}
	},
};

static struct xnvmec cli = {
	.title = "libxnvme hello-device example",
	.descr_short = "Open the given device and print its attributes",
	.subs = subs,
	.nsubs = sizeof(subs) / sizeof(subs[0]),
};

int
main(int argc, char **argv)
{
	return xnvmec(&cli, argc, argv, XNVMEC_INIT_NONE);
}
