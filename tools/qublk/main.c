// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libxnvme.h>
#include <xnvme_util.h>

#include "ctrl.h"
#include "io.h"
#include "qublk.h"

#define QUBLK_DEFAULT_QDEPTH 64
#define QUBLK_DEFAULT_NQUEUES 1
#define QUBLK_DEFAULT_DEV_ID (-1) ///< Let the kernel assign the ublk device identifier
#define QUBLK_DEFAULT_MAX_IO_CAP (1u << 20)

static int
id_in(const char *id, const char **set, size_t n)
{
	for (size_t i = 0; id && i < n; i++) {
		if (!strcmp(id, set[i])) {
			return 1;
		}
	}

	return 0;
}

/*
 * Whether the resolved backend delivers the NVMe 'fua' bit to the device:
 * io_uring and libaio map it to RWF_DSYNC, the passthru async backends carry
 * the command verbatim, and emu/thrpool delegate each command to the sync
 * layer, so there it depends on the sync implementation being a passthru --
 * psync and block issue a plain pwrite() and drop the bit.
 */
static uint8_t
backend_honours_fua(const struct xnvme_dev *xdev)
{
	static const char *async_honours[] = {
		"io_uring", "libaio", "io_uring_cmd", "spdk", "libvfn", "upcie",
	};
	static const char *sync_passthru[] = {
		"nvme",
		"spdk",
		"libvfn",
		"upcie",
	};
	const struct xnvme_opts *opts = xnvme_dev_get_opts(xdev);

	if (id_in(opts->async, async_honours, sizeof(async_honours) / sizeof(*async_honours))) {
		return 1;
	}

	if (opts->async && (!strcmp(opts->async, "emu") || !strcmp(opts->async, "thrpool"))) {
		return (uint8_t)id_in(opts->sync, sync_passthru,
				      sizeof(sync_passthru) / sizeof(*sync_passthru));
	}

	return 0;
}

static int
dev_init(struct qublk_dev *dev, uint32_t want_max_io)
{
	const struct xnvme_spec_idfy_ctrlr *ctrlr;
	uint32_t cap_max;

	dev->geo = xnvme_dev_get_geo(dev->xdev);
	dev->lba_shift = (uint8_t)dev->geo->ssw;
	if (!xnvme_is_pow2(dev->geo->lba_nbytes) || dev->lba_shift < XNVME_UNIVERSAL_SECT_SH) {
		fprintf(stderr, "Failed: %s: unsupported LBA size\n", dev->uri);
		return -EINVAL;
	}

	ctrlr = xnvme_dev_get_ctrlr(dev->xdev);
	dev->has_vwc = ctrlr ? (uint8_t)ctrlr->vwc.present : 1;
	dev->has_fua = backend_honours_fua(dev->xdev);

	cap_max = dev->geo->mdts_nbytes ? dev->geo->mdts_nbytes : QUBLK_DEFAULT_MAX_IO_CAP;
	dev->max_io_buf = (uint32_t)XNVME_MIN_U64(
		want_max_io ? want_max_io : QUBLK_DEFAULT_MAX_IO_CAP, cap_max);
	dev->max_io_buf &= ~(uint32_t)(sysconf(_SC_PAGESIZE) - 1);

	return 0;
}

static int
dev_add(struct qublk_dev *dev, const char *be)
{
	uint64_t feat = 0;
	int rc;

	rc = qublk_ctrl_open(dev);
	if (rc < 0) {
		return rc;
	}

	rc = qublk_ctrl_get_features(dev, &feat);
	if (rc < 0) {
		return rc;
	}

	if (!(feat & UBLK_F_CMD_IOCTL_ENCODE)) {
		xnvme_cli_perr("Failed: kernel lacks UBLK_F_CMD_IOCTL_ENCODE", -ENOSYS);
		return -ENOSYS;
	}

	rc = qublk_ctrl_add_dev(dev);
	if (rc < 0) {
		return rc;
	}

	dev->added = 1;
	fprintf(stderr,
		"qublk: added ublk dev id=%d nqueues=%u qdepth=%u max_io=%u backend=%s uri=%s\n",
		dev->dev_id, dev->nqueues, dev->qdepth, dev->max_io_buf, be ? be : "(auto)",
		dev->uri);

	rc = qublk_ctrl_set_params(dev);
	if (rc < 0) {
		return rc;
	}

	return qublk_io_init(dev);
}

static void
devs_teardown(struct qublk_dev *devs, uint32_t ndevs, struct qublk_thread *threads,
	      uint32_t nthreads)
{
	for (uint32_t d = 0; d < ndevs; d++) {
		// STOP_DEV first, as ubdsrv does: del_gendisk() waits on requests in
		// flight, so the queue threads must still be servicing; the kernel then
		// aborts the pending FETCHes, which is what makes the threads exit
		if (devs[d].started) {
			qublk_ctrl_stop_dev(&devs[d]);
		}

		devs[d].stop = 1;
	}

	qublk_io_threads_join(threads, nthreads);

	for (uint32_t d = 0; d < ndevs; d++) {
		qublk_io_fini(&devs[d]);
		if (devs[d].added) {
			qublk_ctrl_del_dev(&devs[d]);
		}

		qublk_ctrl_close(&devs[d]);
	}
}

static int
sub_run(struct xnvme_cli *cli)
{
	struct xnvme_opts xopts = xnvme_opts_default();
	struct qublk_thread *threads = NULL;
	struct qublk_dev *devs;
	struct xnvme_dev **xdevs;
	const char *be = cli->args.be;
	uint32_t ndevs = (uint32_t)cli->args.posn_count, nthreads = 0;
	uint32_t qdepth = QUBLK_DEFAULT_QDEPTH, nqueues = QUBLK_DEFAULT_NQUEUES;
	uint32_t want_max_io = 0;
	sigset_t blk;
	int err = 0, sig;

	if (!cli->args.posn_count) {
		xnvme_cli_perr("Error: at least one device URI is required", -EINVAL);
		return -EINVAL;
	}

	// Options are optional; only override the defaults for the ones actually given
	if (cli->given[XNVME_CLI_OPT_QDEPTH]) {
		qdepth = cli->args.qdepth;
	}

	if (cli->given[XNVME_CLI_OPT_NQUEUES]) {
		nqueues = cli->args.nqueues;
	}

	if (cli->given[XNVME_CLI_OPT_MAX_IO_BYTES]) {
		want_max_io = cli->args.max_io_bytes;
	}

	// Half of UBLK_MAX_QUEUE_DEPTH: xnvme_queue_init() requires a capacity
	// strictly below 4096, so a qdepth of 4096 would fail only after ADD_DEV
	if (!xnvme_is_pow2(qdepth) || qdepth > (UBLK_MAX_QUEUE_DEPTH / 2)) {
		xnvme_cli_perr("Error: --qdepth must be a power of 2 and within limits", -EINVAL);
		return -EINVAL;
	}

	if (!nqueues || nqueues > UBLK_MAX_NR_QUEUES) {
		xnvme_cli_perr("Error: --nqueues is out of range", -EINVAL);
		return -EINVAL;
	}

	// The identifier becomes the ublk minor; cap it accordingly (MINORBITS)
	if (cli->given[XNVME_CLI_OPT_DEV_ID] &&
	    (uint64_t)cli->args.dev_id + ndevs - 1 >= (1u << 20)) {
		xnvme_cli_perr("Error: --dev-id is out of range", -EINVAL);
		return -EINVAL;
	}

	// max_io_buf is rounded down to a page multiple below; anything smaller
	// than a page would round to zero
	if (cli->given[XNVME_CLI_OPT_MAX_IO_BYTES] &&
	    want_max_io < (uint32_t)sysconf(_SC_PAGESIZE)) {
		xnvme_cli_perr("Error: --max-io-bytes must be at least the page size", -EINVAL);
		return -EINVAL;
	}

	if (cli->args.ncpus > (uint64_t)ndevs * nqueues) {
		xnvme_cli_perr("Error: more CPUs than queues", -EINVAL);
		return -EINVAL;
	}

	// A thread's io_uring needs one entry per tag of each of its queues
	if (cli->args.ncpus &&
	    qdepth * (((uint64_t)ndevs * nqueues + cli->args.ncpus - 1) / cli->args.ncpus) >
		    QUBLK_MAX_RING_ENTRIES) {
		xnvme_cli_perr("Error: too many queues per CPU for --qdepth", -EINVAL);
		return -EINVAL;
	}

	xnvme_cli_to_opts(cli, &xopts);
	xopts.rdwr = 1;
	// All devices share the uPCIe heap, which holds the I/O buffers of every
	// queue. The heap is created when the first device opens, before MDTS is
	// known, so size it for the largest buffer size allowed
	xopts.host_heap_size = xnvme_util_heap_size(
		(size_t)ndevs * nqueues,
		(size_t)qdepth * (want_max_io ? want_max_io : QUBLK_DEFAULT_MAX_IO_CAP));

	devs = calloc(ndevs, sizeof(*devs));
	if (!devs) {
		xnvme_cli_perr("Failed: calloc()", -ENOMEM);
		return -ENOMEM;
	}

	err = xnvme_cli_dev_open_multi(cli->args.posn, (int)ndevs, &xopts, &xdevs);
	if (err) {
		free(devs);
		return err;
	}

	for (uint32_t d = 0; d < ndevs; d++) {
		devs[d].xdev = xdevs[d];
		devs[d].uri = cli->args.posn[d];
		devs[d].ctrl_fd = -1;
		devs[d].ublkc_fd = -1;
		devs[d].dev_id = cli->given[XNVME_CLI_OPT_DEV_ID] ? (int)(cli->args.dev_id + d)
								  : QUBLK_DEFAULT_DEV_ID;
		devs[d].nqueues = nqueues;
		devs[d].qdepth = qdepth;
		devs[d].flags = UBLK_F_CMD_IOCTL_ENCODE;
	}

	for (uint32_t d = 0; d < ndevs; d++) {
		err = dev_init(&devs[d], want_max_io);
		if (err) {
			goto teardown;
		}
	}

	setvbuf(stderr, NULL, _IOLBF, 0);

	sigemptyset(&blk);
	sigaddset(&blk, SIGINT);
	sigaddset(&blk, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &blk, NULL);

	for (uint32_t d = 0; d < ndevs; d++) {
		err = dev_add(&devs[d], be);
		if (err) {
			goto teardown;
		}
	}

	err = qublk_io_threads_start(devs, ndevs, cli->args.cpus, cli->args.ncpus, &threads,
				     &nthreads);
	if (err) {
		goto teardown;
	}

	for (uint32_t d = 0; d < ndevs; d++) {
		err = qublk_ctrl_start_dev(&devs[d]);
		if (err) {
			goto teardown;
		}

		devs[d].started = 1;
		fprintf(stderr, "qublk: /dev/ublkb%d ready (Ctrl-C to stop)\n", devs[d].dev_id);
	}

	sigwait(&blk, &sig);
	fprintf(stderr, "qublk: stopping (signal %d)\n", sig);

teardown:
	devs_teardown(devs, ndevs, threads, nthreads);
	xnvme_cli_dev_close_multi(xdevs, (int)ndevs);
	free(devs);
	return err;
}

static int
sub_del(struct xnvme_cli *cli)
{
	struct qublk_dev dev = {
		.ctrl_fd = -1,
	};
	int rc;

	if (!cli->given[XNVME_CLI_OPT_DEV_ID]) {
		xnvme_cli_perr("Error: --dev-id is required", -EINVAL);
		return -EINVAL;
	}

	if (cli->args.dev_id >= (1u << 20)) {
		xnvme_cli_perr("Error: --dev-id is out of range", -EINVAL);
		return -EINVAL;
	}

	dev.dev_id = (int)cli->args.dev_id;

	rc = qublk_ctrl_open(&dev);
	if (rc < 0) {
		return rc;
	}

	// A device left behind by a killed server is usually still live; STOP_DEV
	// makes the kernel abort its pending requests so DEL_DEV can proceed. On a
	// device that is already stopped it fails, which is fine to ignore.
	qublk_ctrl_stop_dev(&dev);
	rc = qublk_ctrl_del_dev(&dev);
	qublk_ctrl_close(&dev);
	if (rc == 0) {
		fprintf(stderr, "qublk: deleted ublk dev id=%d\n", dev.dev_id);
	}

	return rc;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"run",
		"Serve a ublk block-device for each of the given xNVMe devices",
		"Serve a ublk block-device for each of the given xNVMe devices",
		sub_run,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSN},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_NQUEUES, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DEV_ID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_MAX_IO_BYTES, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CPUMASK, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CPULIST, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ORCH_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_HOMI_ID, XNVME_CLI_LOPT},
		},
	},
	{
		"del",
		"Delete a ublk device left behind by a killed server",
		"Delete a ublk device left behind by a killed server",
		sub_del,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_DEV_ID, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "qublk - ublk server backed by xNVMe",
	.descr_short = "Expose an xNVMe device as a ublk block-device",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
