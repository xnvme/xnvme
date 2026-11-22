// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/mman.h>
#endif

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

#ifdef __linux__
static double
usecs_since(const struct timespec *t0)
{
	struct timespec t1;

	clock_gettime(CLOCK_MONOTONIC, &t1);

	return (double)(t1.tv_sec - t0->tv_sec) * 1e6 + (double)(t1.tv_nsec - t0->tv_nsec) / 1e3;
}

static int
cmp_double(const void *a, const void *b)
{
	double x = *(const double *)a, y = *(const double *)b;

	return (x > y) - (x < y);
}

static void
pr_usecs(const char *name, double *usecs, uint32_t count)
{
	double median;

	qsort(usecs, count, sizeof(*usecs), cmp_double);
	median = (count % 2) ? usecs[count / 2] : (usecs[count / 2 - 1] + usecs[count / 2]) / 2;
	printf("  %s: {min: %.1f, median: %.1f, max: %.1f}\n", name, usecs[0], median,
	       usecs[count - 1]);
}

static int
register_once(struct xnvme_dev *dev, int fd, void *vaddr, double *reg_usecs, double *unreg_usecs)
{
	struct timespec t0;
	int err;

	clock_gettime(CLOCK_MONOTONIC, &t0);
	err = xnvme_buf_register_dmabuf(dev, fd, vaddr);
	*reg_usecs = usecs_since(&t0);
	if (err) {
		xnvme_cli_perr("xnvme_buf_register_dmabuf()", err);
		return err;
	}

	clock_gettime(CLOCK_MONOTONIC, &t0);
	err = xnvme_buf_unregister(dev, vaddr);
	*unreg_usecs = usecs_since(&t0);
	if (err) {
		xnvme_cli_perr("xnvme_buf_unregister()", err);
	}

	return err;
}

static int
register_dmabuf(struct xnvme_cli *cli, uint32_t count)
{
	struct xnvme_opts opts = {0};
	struct xnvme_dev *dev = NULL;
	double *reg_usecs = NULL, *unreg_usecs = NULL;
	double first_reg_usecs, first_unreg_usecs;
	void *vaddr = MAP_FAILED;
	off_t nbytes;
	int err = 0;

	nbytes = lseek(cli->args.dmabuf_fd, 0, SEEK_END);
	if (nbytes <= 0) {
		err = nbytes < 0 ? -errno : -EINVAL;
		xnvme_cli_perr("Error: --dmabuf-fd is no dma-buf this process holds", err);
		return err;
	}
	// addresses for the dma-buf to be named by, which nothing reads or writes
	vaddr = mmap(NULL, (size_t)nbytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
		     -1, 0);
	reg_usecs = calloc(count, sizeof(*reg_usecs));
	unreg_usecs = calloc(count, sizeof(*unreg_usecs));
	if ((vaddr == MAP_FAILED) || !reg_usecs || !unreg_usecs) {
		err = -ENOMEM;
		xnvme_cli_perr("Error: no addresses for the dma-buf, or no memory", err);
		goto exit;
	}

	xnvme_opts_set_defaults(&opts);
	xnvme_cli_to_opts(cli, &opts);
	dev = xnvme_dev_open(cli->args.uri, &opts);
	if (!dev) {
		err = -errno;
		xnvme_cli_perr("xnvme_dev_open()", err);
		goto exit;
	}

	err = register_once(dev, cli->args.dmabuf_fd, vaddr, &first_reg_usecs, &first_unreg_usecs);
	if (err) {
		goto exit;
	}
	for (uint32_t i = 0; i < count; ++i) {
		err = register_once(dev, cli->args.dmabuf_fd, vaddr, &reg_usecs[i],
				    &unreg_usecs[i]);
		if (err) {
			goto exit;
		}
	}

	printf("xnvme_perf_xfer_register:\n");
	printf("  uri: '%s'\n", cli->args.uri);
	printf("  be: '%s'\n", cli->args.be ? cli->args.be : "");
	printf("  homi_id: %u\n", opts.homi_id);
	printf("  dmabuf_nbytes: %jd\n", (intmax_t)nbytes);
	printf("  count: %u\n", count);
	printf("  first_register_usecs: %.1f\n", first_reg_usecs);
	printf("  first_unregister_usecs: %.1f\n", first_unreg_usecs);
	pr_usecs("register_usecs", reg_usecs, count);
	pr_usecs("unregister_usecs", unreg_usecs, count);

exit:
	xnvme_dev_close(dev);
	if (vaddr != MAP_FAILED) {
		munmap(vaddr, (size_t)nbytes);
	}
	free(unreg_usecs);
	free(reg_usecs);

	return err;
}
#endif

static int
sub_register(struct xnvme_cli *cli)
{
	uint64_t count = cli->given[XNVME_CLI_OPT_COUNT] ? cli->args.count : 100;
	int err;

	if (!count || (count > UINT32_MAX)) {
		err = -EINVAL;
		xnvme_cli_perr("Error: --count must be above 0 and at most UINT32_MAX", err);
		return err;
	}
#ifdef __linux__
	return register_dmabuf(cli, (uint32_t)count);
#else
	err = -ENOTSUP;
	xnvme_cli_perr("Error: --dmabuf-fd takes a dma-buf, which is a Linux interface", err);
	return err;
#endif
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
	{
		"register",
		"Time registering a dma-buf with a device, and unregistering it",
		"Register the dma-buf the process inherited as --dmabuf-fd with the device "
		"and unregister it, once and then --count times, 100 by default. Report the "
		"microseconds of the first registration and unregistering apart, and the "
		"minimum, median and maximum of the --count after them, so that a cost only "
		"the first call pays does not count in those. The dma-buf is named by "
		"addresses reserved with PROT_NONE, which nothing reads or writes; its size is "
		"the exporter's. With --homi-id the registrations go through the server.",
		sub_register,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_DMABUF_FD, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_HOMI_ID, XNVME_CLI_LOPT},
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
