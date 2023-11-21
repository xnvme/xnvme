// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>
#include <unistd.h>
/**
 * This example shows how send a single asynchronous command
 * and wait for completion using eventfd.
 *
 * - Allocate a buffer for the command payload
 * - Setup a Command Queue
 *   | Using command-contexts
 * - Submit the asynchronous command
 * - Reporting on IO-errors
 * - Teardown
 */
int
main(int argc, char **argv)
{
	struct xnvme_opts opts = xnvme_opts_default();
	struct xnvme_dev *dev = NULL;
	uint32_t nsid;
	char *buf = NULL;
	size_t buf_nbytes;
	struct xnvme_queue *queue = NULL;
	int ret = 0, err = 0;
	int completion_fd;
	uint64_t v;

	if (argc < 2) {
		xnvme_cli_perr("Usage: %s <ident>", EINVAL);
		return 1;
	}

	dev = xnvme_dev_open(argv[1], &opts);
	if (!dev) {
		xnvme_cli_perr("xnvme_dev_open()", errno);
		return 1;
	}
	nsid = xnvme_dev_get_nsid(dev);

	// Initialize a command-queue
	ret = xnvme_queue_init(dev, 16, 0, &queue);
	if (ret) {
		xnvme_cli_perr("xnvme_queue_init()", ret);
		xnvme_dev_close(dev);
		return 1;
	}

	buf_nbytes = xnvme_dev_get_geo(dev)->nbytes;

	xnvme_cli_pinf("Allocate a payload-buffer of nbytes: %zu", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		xnvme_cli_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}
	memset(buf, 0, buf_nbytes);

	completion_fd = xnvme_queue_get_completion_fd(queue);
	if (completion_fd <= 0) {
		xnvme_cli_perr("error getting eventfd", errno);
		goto exit;
	}

	struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

	// Submit a read command
	err = xnvme_nvm_read(ctx, nsid, 0x0, 0, buf, NULL);
	if (err) {
		xnvme_cli_perr("error submitting", -err);
		goto exit;
	}

	ret = read(completion_fd, &v, sizeof(v));
	if (ret < 0) {
		xnvme_cli_perr("error reading eventfd", errno);
		goto exit;
	}

	// This must process 1 command.
	err = xnvme_queue_poke(queue, 0);
	if (err < 0) {
		xnvme_cli_perr("error Processing completions", -err);
		goto exit;
	} else if (err != 1) {
		xnvme_cli_pinf("unexpected number of completions %d", err);
		goto exit;
	}

	xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
	xnvme_queue_put_cmd_ctx(queue, ctx);

exit:
	xnvme_buf_free(dev, buf);
	xnvme_queue_term(queue);
	xnvme_dev_close(dev);

	return ret || err;
}
