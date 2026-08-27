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

#include "ctrl.h"
#include "io.h"
#include "qublk.h"

#define QUBLK_DEFAULT_QDEPTH 64
#define QUBLK_DEFAULT_NQUEUES 1
#define QUBLK_DEFAULT_DEV_ID (-1) ///< Let the kernel assign the ublk device identifier
#define QUBLK_DEFAULT_MAX_IO_CAP (1u << 20)

static uint8_t
lba_shift_of(uint32_t lba_nbytes)
{
	for (uint8_t s = 0; s < 32; s++) {
		if ((1u << s) == lba_nbytes) {
			return s;
		}
	}
	return 0;
}

static int
sub_run(struct xnvme_cli *cli)
{
	struct qublk_dev dev = {
		.ctrl_fd = -1,
		.dev_id = QUBLK_DEFAULT_DEV_ID,
		.nqueues = QUBLK_DEFAULT_NQUEUES,
		.qdepth = QUBLK_DEFAULT_QDEPTH,
		.flags = UBLK_F_CMD_IOCTL_ENCODE,
	};
	struct xnvme_opts xopts = xnvme_opts_default();
	const char *uri = cli->args.uri;
	const char *be = cli->args.be;
	sigset_t blk;
	uint64_t feat = 0;
	uint32_t want_max_io = 0, cap_max;
	int sig;

	// Options are optional; only override the defaults for the ones actually given
	if (cli->given[XNVME_CLI_OPT_QDEPTH]) {
		dev.qdepth = cli->args.qdepth;
	}
	if (cli->given[XNVME_CLI_OPT_NQUEUES]) {
		dev.nqueues = cli->args.nqueues;
	}
	if (cli->given[XNVME_CLI_OPT_DEV_ID]) {
		dev.dev_id = (int)cli->args.dev_id;
	}
	if (cli->given[XNVME_CLI_OPT_MAX_IO_BYTES]) {
		want_max_io = cli->args.max_io_bytes;
	}

	// Half of QUBLK_MAX_QUEUE_DEPTH: xnvme_queue_init() requires a capacity
	// strictly below 4096, so a qdepth of 4096 would fail only after ADD_DEV
	if (!xnvme_is_pow2(dev.qdepth) || dev.qdepth > (QUBLK_MAX_QUEUE_DEPTH / 2)) {
		xnvme_cli_perr("Error: --qdepth must be a power of 2 and within limits", -EINVAL);
		return -EINVAL;
	}
	if (dev.nqueues > UBLK_MAX_NR_QUEUES) {
		xnvme_cli_perr("Error: --nqueues is out of range", -EINVAL);
		return -EINVAL;
	}
	// The identifier becomes the ublk minor; cap it accordingly (MINORBITS)
	if (cli->given[XNVME_CLI_OPT_DEV_ID] && cli->args.dev_id >= (1u << 20)) {
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

	xnvme_cli_to_opts(cli, &xopts);
	xopts.rdwr = 1;

	dev.xdev = xnvme_dev_open(uri, &xopts);
	if (!dev.xdev) {
		int err = errno ? -errno : -EIO;

		xnvme_cli_perr("Failed: xnvme_dev_open()", err);
		return err;
	}
	dev.geo = xnvme_dev_get_geo(dev.xdev);
	dev.lba_shift = lba_shift_of(dev.geo->lba_nbytes);
	if (dev.lba_shift < 9) {
		xnvme_cli_perr("Failed: unsupported LBA size", -EINVAL);
		goto err_xdev;
	}

	{
		const struct xnvme_spec_idfy_ctrlr *ctrlr = xnvme_dev_get_ctrlr(dev.xdev);
		dev.has_vwc = ctrlr ? (uint8_t)ctrlr->vwc.present : 1;
	}

	cap_max = dev.geo->mdts_nbytes ? dev.geo->mdts_nbytes : QUBLK_DEFAULT_MAX_IO_CAP;
	dev.max_io_buf = want_max_io
				 ? want_max_io
				 : (cap_max < QUBLK_DEFAULT_MAX_IO_CAP ? cap_max
								       : QUBLK_DEFAULT_MAX_IO_CAP);
	if (dev.max_io_buf > cap_max) {
		dev.max_io_buf = cap_max;
	}
	dev.max_io_buf &= ~(uint32_t)(sysconf(_SC_PAGESIZE) - 1);

	setvbuf(stderr, NULL, _IOLBF, 0);

	sigemptyset(&blk);
	sigaddset(&blk, SIGINT);
	sigaddset(&blk, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &blk, NULL);

	if (qublk_ctrl_open(&dev) < 0) {
		goto err_xdev;
	}
	if (qublk_ctrl_get_features(&dev, &feat) < 0) {
		goto err_ctrl;
	}
	if (!(feat & UBLK_F_CMD_IOCTL_ENCODE)) {
		xnvme_cli_perr("Failed: kernel lacks UBLK_F_CMD_IOCTL_ENCODE", -ENOSYS);
		goto err_ctrl;
	}

	if (qublk_ctrl_add_dev(&dev) < 0) {
		goto err_ctrl;
	}
	fprintf(stderr,
		"qublk: added ublk dev id=%d nqueues=%u qdepth=%u max_io=%u backend=%s uri=%s\n",
		dev.dev_id, dev.nqueues, dev.qdepth, dev.max_io_buf, be ? be : "(auto)", uri);

	if (qublk_ctrl_set_params(&dev) < 0) {
		goto err_added;
	}
	if (qublk_io_init(&dev) < 0) {
		goto err_added;
	}
	if (qublk_io_thread_start(&dev) < 0) {
		goto err_io;
	}
	if (qublk_ctrl_start_dev(&dev) < 0) {
		dev.stop = 1;
		qublk_io_thread_join(&dev);
		goto err_io;
	}

	fprintf(stderr, "qublk: /dev/ublkb%d ready (Ctrl-C to stop)\n", dev.dev_id);

	sigwait(&blk, &sig);
	fprintf(stderr, "qublk: stopping (signal %d)\n", sig);
	// STOP_DEV first, as ubdsrv does: del_gendisk() waits on requests in
	// flight, so the queue threads must still be servicing; the kernel then
	// aborts the pending FETCHes, which is what makes the threads exit
	qublk_ctrl_stop_dev(&dev);
	dev.stop = 1;

	qublk_io_thread_join(&dev);
	qublk_io_fini(&dev);
	qublk_ctrl_del_dev(&dev);
	qublk_ctrl_close(&dev);
	xnvme_dev_close(dev.xdev);
	return 0;

err_io:
	qublk_io_fini(&dev);
err_added:
	qublk_ctrl_del_dev(&dev);
err_ctrl:
	qublk_ctrl_close(&dev);
err_xdev:
	xnvme_dev_close(dev.xdev);
	return -EIO;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"run",
		"Serve a ublk block-device backed by the given xNVMe device",
		"Serve a ublk block-device backed by the given xNVMe device",
		sub_run,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_NQUEUES, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DEV_ID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_MAX_IO_BYTES, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ORCH_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_SHM_ID, XNVME_CLI_LOPT},
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
