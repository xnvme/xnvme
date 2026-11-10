// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Usage Examples
 * --------------
 *
 * ## OS Managed Devices
 * # Linux
 * example_dev /dev/nvme0n1 1
 *
 * # FreeBSD
 * example_dev /dev/nvme0ns1 1
 *
 * # Windows
 * example_dev \\.\PhysicalDevice1 1
 *
 * # User-space driver
 * example_dev 0000:04:00.0 1
 *
 * # NVMe-over-Fabrics
 * example_dev 192.168.1.1:4472 1
 */
#include <stdio.h>
#include <string.h>
#include <libxnvme.h>

int
main(int argc, char *argv[])
{
	struct xnvme_opts opts = {0};
	struct xnvme_dev *dev;
	int err;

	xnvme_opts_set_defaults(&opts);

	if (argc < 2 || !strcmp(argv[1], "--help")) {
		printf("Usage: %s <uri> [<nsid>]\n\nOpen namespace <nsid>, or 1, of the device at "
		       "<uri> and print it\n",
		       argv[0]);
		return argc < 2;
	}

	switch (argc) {
	case 3:
		opts.nsid = atoi(argv[2]);
		break;
	case 2:
		opts.nsid = 1;
		break;
	default:
		return 1;
	}

	err = xnvme_dev_open_uri(argv[1], &opts, &dev);
	if (err) {
		xnvme_cli_perr("xnvme_dev_open_uri()", err);
		return 1;
	}
	xnvme_dev_pr(dev, XNVME_PR_DEF);
	xnvme_dev_close(dev);

	return 0;
}
