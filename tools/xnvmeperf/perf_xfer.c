// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdio.h>

#include <libxnvme.h>
#include <xnvme_util.h>
#include <xnvme_vcs.h>

#include "xnvmeperf_gpu.h"

static int
sub_htod(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = {0};
	uint32_t iosize = cli->args.iosize;
	uint32_t nslots = cli->args.qdepth ? cli->args.qdepth : 1;
	uint32_t seconds = cli->args.runtime ? cli->args.runtime : 5;
	double gbps = 0.0;
	int err;

	xnvme_opts_set_defaults(&opts);

	xnvme_cli_to_opts(cli, &opts);

	if (!iosize || !xnvme_is_pow2(iosize)) {
		err = -EINVAL;
		xnvme_cli_perr("Error: --iosize must be a power of 2", err);
		return err;
	}

	printf("xnvme perf-xfer htod: iosize %u, in-flight %u, runtime %u s, gpu %u\n", iosize,
	       nslots, seconds, opts.gpu_id);

	err = xnvmeperf_htod_roofline(opts.gpu_id, iosize, nslots, seconds, &gbps);
	if (err) {
		xnvme_cli_perr("Failed: xnvmeperf_htod_roofline()", err);
		return err;
	}

	printf("\nhost-to-device copy: %.2f GB/s\n", gbps);
	return 0;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"htod",
		"Host-to-device copy bandwidth ceiling (requires a GPU backend build)",
		"Keep --qdepth pinned copies of --iosize in flight to one GPU for --runtime "
		"seconds and report the delivered host-to-device GB/s. Takes no NVMe devices.",
		sub_htod,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_IOSIZE, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_RUNTIME, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_GPU_ID, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "xnvme perf-xfer - data transfer benchmarks between memories",
	.vcs = XNVME_VCS_TAG,
	.descr_short = "Run data transfer benchmarks between memories",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
