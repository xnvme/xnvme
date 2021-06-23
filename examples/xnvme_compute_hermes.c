// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvmec.h>
#include <libxnvme_compute.h>

#define BUF_SIZE 1024 * 1024

static int
sub_compute_example(struct xnvmec *cli)
{
	struct xnvme_dev *dev = xnvme_dev_open(cli->args.uri);
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);

	if (!dev) {
		xnvmec_perr("xnvme_dev_open()", errno);
		return -errno;
	}

	void *prog = malloc(BUF_SIZE * sizeof(char));
	memset(prog, 0x55, BUF_SIZE);

	xnvme_compute_load(&ctx, prog, BUF_SIZE * sizeof(char), 0);
	xnvme_compute_exec(&ctx, 0);

	return 0;
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvmec_sub g_subs[] = {
	{
		"compute", "Compute Example", "Compute Example", sub_compute_example, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
		}
	},
};

static struct xnvmec g_cli = {
	.title = "xNVMe Compute example",
	.descr_short = "Open the given device and send a simple BPF program to it",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_NONE);
}
