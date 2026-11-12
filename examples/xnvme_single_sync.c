// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <libxnvme.h>

/**
 * This example shows how send a single synchronous command
 *
 * - Allocate a buffer for the command payload
 * - Submit the asynchronous command
 * - Reporting on IO-errors
 * - Teardown
 */
int
main(int argc, char **argv)
{
	struct xnvme_opts opts = {0};
	struct xnvme_dev *dev = NULL;
	uint32_t nsid;
	char *buf = NULL;
	size_t buf_nbytes;
	int err = 0;

	xnvme_opts_set_defaults(&opts);

	if (argc < 2 || !strcmp(argv[1], "--help")) {
		printf("Usage: %s <uri>\n\nRead the first block of the device at <uri> with one "
		       "synchronous command\n",
		       argv[0]);
		return argc < 2;
	}

	err = xnvme_dev_open_uri(argv[1], &opts, &dev);
	if (err) {
		xnvme_cli_perr("xnvme_dev_open_uri()", err);
		return 1;
	}
	nsid = xnvme_dev_get_nsid(dev);

	buf_nbytes = xnvme_dev_get_geo(dev)->lba_nbytes;

	xnvme_cli_pinf("Allocate a payload-buffer of nbytes: %zu", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		xnvme_cli_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}

	err = xnvme_buf_clear(buf, buf_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_clear()", err);
		goto exit;
	}

	// This block would typically be a loop as more than a single command would be submitted
	{
		struct xnvme_cmd_ctx ctx = {0};

		// Prepare a read of one logical block; NLB is zero-based
		struct xnvme_cmd_prep_nvm_read_args args = {
			.args_nbytes = sizeof(args),
			.nsid = nsid,
			.slba = 0x0,
			.nlb = 0,
		};

		xnvme_dev_get_cmd_ctx(dev, &ctx);
		err = xnvme_cmd_prep_nvm_read(&ctx.cmd, &args);
		if (err) {
			xnvme_cli_perr("xnvme_cmd_prep_nvm_read()", err);
			goto exit;
		}

		// Submit it and wait for its completion
		err = xnvme_cmd_pass(&ctx, buf, buf_nbytes, NULL, 0);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("xnvme_cmd_pass()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
		xnvme_cli_pinf("Submitted and completed command succesfully");
	}

exit:
	xnvme_buf_free(dev, buf);
	xnvme_dev_close(dev);

	return err;
}
