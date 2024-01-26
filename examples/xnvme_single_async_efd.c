// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>
#include <unistd.h>
/**
 * This example shows how send a single asynchronous read command
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
sub_async_read(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct xnvme_cmd_ctx *ctx = NULL;
	struct xnvme_queue *queue;
	size_t buf_nbytes;
	char *buf = NULL;
	int completion_fd;
	uint64_t ptr;
	int err;

	// Initialize a command-queue
	err = xnvme_queue_init(dev, 16, 0, &queue);
	if (err) {
		xnvme_cli_perr("xnvme_queue_init()", err);
		xnvme_dev_close(dev);
		return err;
	}

	buf_nbytes = xnvme_dev_get_geo(dev)->nbytes;

	xnvme_cli_pinf("Allocate a payload-buffer of nbytes: %zu", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	memset(buf, 0, buf_nbytes);

	completion_fd = xnvme_queue_get_completion_fd(queue);
	if (completion_fd <= 0) {
		err = -errno;
		xnvme_cli_perr("error getting eventfd", err);
		goto exit;
	}

	ctx = xnvme_queue_get_cmd_ctx(queue);

	// Submit a read command
	err = xnvme_nvm_read(ctx, nsid, 0x0, 0, buf, NULL);
	if (err) {
		xnvme_cli_perr("error submitting", err);
		goto exit;
	}

	err = read(completion_fd, &ptr, sizeof(ptr));
	if (err < 0) {
		err = -errno;
		xnvme_cli_perr("error reading eventfd", err);
		goto exit;
	}

	// This must process 1 command.
	err = xnvme_queue_poke(queue, 0);
	if (err < 0) {
		xnvme_cli_perr("error Processing completions", err);
		goto exit;
	} else if (err != 1) {
		xnvme_cli_pinf("unexpected number of completions %d", err);
		err = -EIO;
		goto exit;
	}

	xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);

exit:
	if (ctx) {
		xnvme_queue_put_cmd_ctx(queue, ctx);
	}
	xnvme_buf_free(dev, buf);
	xnvme_queue_term(queue);

	return err < 0 ? err : 0;
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvme_cli_sub g_subs[] = {{
	"read",
	"Send a single asynchronous read command and wait for completion using eventfd",
	"Send a single asynchronous read command and wait for completion using eventfd",
	sub_async_read,
	{
		{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
		{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

		{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
		{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LOPT},
		{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},
		{XNVME_CLI_OPT_ELBA, XNVME_CLI_LOPT},
		{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},
		{XNVME_CLI_OPT_SEED, XNVME_CLI_LOPT},

		XNVME_CLI_ASYNC_OPTS,
	},
}};

static struct xnvme_cli g_cli = {
	.title = "xNVMe: Asynchronous IO With Eventfd Example",
	.descr_short = "xNVMe: Asynchronous IO With Eventfd Example",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}