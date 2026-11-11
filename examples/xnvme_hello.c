// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <libxnvme.h>

int
main(int argc, char **argv)
{
	struct xnvme_opts opts = {0};
	struct xnvme_dev *dev;

	xnvme_opts_set_defaults(&opts);

	if (argc < 2 || !strcmp(argv[1], "--help")) {
		printf("Usage: %s <uri>\n\nOpen the device at <uri> and print it\n", argv[0]);
		return argc < 2;
	}

	dev = xnvme_dev_open(argv[1], &opts);
	if (!dev) {
		xnvme_cli_perr("xnvme_dev_open()", errno);
		return 1;
	}

	if (xnvme_dev_derive_geo(dev)) {
		printf("Failed deriving geometry, check your permissions.");
	}
	xnvme_dev_pr(dev, XNVME_PR_DEF);
	xnvme_dev_close(dev);

	return 0;
}
