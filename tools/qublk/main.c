// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#define _GNU_SOURCE
#include <errno.h>
#include <getopt.h>
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

static bool
is_pow2(uint32_t v)
{
	return v && (v & (v - 1)) == 0;
}

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

static const struct option qublk_opts[] = {
	{"be", required_argument, NULL, 'b'},
	{"depth", required_argument, NULL, 'd'},
	{"dev-id", required_argument, NULL, 'i'},
	{"max-io-bytes", required_argument, NULL, 'm'},
	{"nr-queues", required_argument, NULL, 'q'},
	{"help", no_argument, NULL, 'h'},
	{0},
};

static void
usage(const char *prog)
{
	fprintf(stderr,
		"usage: %s URI [--be BE] [--depth N] [--dev-id N] [--max-io-bytes N] "
		"[--nr-queues N]\n"
		"  URI            xNVMe device URI (e.g. /dev/nvme0n1, 0000:01:00.0)\n"
		"  --be BE        xNVMe backend (upcie, io_uring, io_uring_cmd, libaio, ...)\n"
		"  --depth N      per-queue depth, power of 2 (default: 64)\n"
		"  --dev-id N     requested ublk dev id (default: -1 / auto)\n"
		"  --max-io-bytes N  per-IO buffer size (default: min(1MiB, MDTS))\n"
		"  --nr-queues N  number of ublk hardware queues (default: 1)\n",
		prog);
}

int
main(int argc, char **argv)
{
	struct qublk_dev dev = {
		.ctrl_fd = -1,
		.dev_id = -1,
		.nr_queues = 1,
		.depth = 64,
		.flags = UBLK_F_CMD_IOCTL_ENCODE,
	};
	struct xnvme_opts xopts;
	const char *uri = NULL, *be = NULL;
	sigset_t blk;
	uint64_t feat = 0;
	uint32_t want_max_io = 0, cap_max;
	int c, sig;

	while ((c = getopt_long(argc, argv, "b:d:i:m:q:h", qublk_opts, NULL)) != -1) {
		switch (c) {
		case 'b':
			be = optarg;
			break;
		case 'd':
			dev.depth = (uint16_t)atoi(optarg);
			break;
		case 'i':
			dev.dev_id = atoi(optarg);
			break;
		case 'm':
			want_max_io = (uint32_t)strtoul(optarg, NULL, 0);
			break;
		case 'q':
			dev.nr_queues = (uint16_t)atoi(optarg);
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			usage(argv[0]);
			return 2;
		}
	}
	if (optind >= argc) {
		usage(argv[0]);
		return 2;
	}
	uri = argv[optind];
	if (!is_pow2(dev.depth) || dev.depth > QUBLK_MAX_QUEUE_DEPTH) {
		fprintf(stderr, "qublk: --depth must be power of 2, <= %u\n",
			QUBLK_MAX_QUEUE_DEPTH);
		return 2;
	}
	if (dev.nr_queues < 1 || dev.nr_queues > UBLK_MAX_NR_QUEUES) {
		fprintf(stderr, "qublk: --nr-queues must be in [1, %u]\n", UBLK_MAX_NR_QUEUES);
		return 2;
	}

	xopts = xnvme_opts_default();
	xopts.rdwr = 1;
	if (be) {
		xopts.be = be;
	}

	dev.xdev = xnvme_dev_open(uri, &xopts);
	if (!dev.xdev) {
		fprintf(stderr, "xnvme_dev_open(%s): %s\n", uri, strerror(errno));
		return 1;
	}
	dev.geo = xnvme_dev_get_geo(dev.xdev);
	dev.lba_shift = lba_shift_of(dev.geo->lba_nbytes);
	if (dev.lba_shift < 9) {
		fprintf(stderr, "qublk: unsupported LBA size %u\n", dev.geo->lba_nbytes);
		goto err_xdev;
	}

	{
		const struct xnvme_spec_idfy_ctrlr *ctrlr = xnvme_dev_get_ctrlr(dev.xdev);
		dev.has_vwc = ctrlr ? (uint8_t)ctrlr->vwc.present : 1;
	}

	cap_max = dev.geo->mdts_nbytes ? dev.geo->mdts_nbytes : (1u << 20);
	dev.max_io_buf = want_max_io ? want_max_io : (cap_max < (1u << 20) ? cap_max : (1u << 20));
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
		fprintf(stderr, "qublk: kernel lacks UBLK_F_CMD_IOCTL_ENCODE\n");
		goto err_ctrl;
	}

	if (qublk_ctrl_add_dev(&dev) < 0) {
		goto err_ctrl;
	}
	fprintf(stderr,
		"qublk: added ublk dev id=%d nr_queues=%u depth=%u max_io=%u backend=%s uri=%s\n",
		dev.dev_id, dev.nr_queues, dev.depth, dev.max_io_buf, be ? be : "(auto)", uri);

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
	dev.stop = 1;

	qublk_io_thread_join(&dev);
	qublk_ctrl_stop_dev(&dev);
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
	return 1;
}
