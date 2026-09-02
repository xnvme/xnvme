// SPDX-FileCopyrightText: Simon A. F. Lund <os@safl.dk>
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <libxnvme.h>

/* Controllers one run may drive, matching what a GPU runtime allows. */
#define CTRLRS_MAX 16

/**
 * Write a distinct pattern to every controller and read them all back
 *
 * One process, several controllers, one buffer pool. Everything the process
 * built once has to reach all of them: the memory it allocates, the addresses
 * the controllers consume, and the doorbells it rings. Where any of those is
 * per-controller but held once, this reads back another controller's pattern
 * or nothing at all, which a single-controller test cannot show.
 *
 * The payload buffers may be device memory, which the host can neither read
 * nor write directly, so patterns are staged in host memory and moved with
 * xnvme_buf_memcpy(), which knows where a buffer lives.
 */
static int
sub_io(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev[CTRLRS_MAX] = {cli->args.dev};
	struct xnvme_opts opts = xnvme_opts_default();
	void *buf[CTRLRS_MAX] = {NULL};
	char *host_w = NULL, *host_r = NULL, *uris = NULL;
	int ndev = 1, err = 0;
	size_t nbytes;

	if (!cli->args.alt_uri) {
		xnvme_cli_perr("At least one more controller is required; give --alt-uri",
			       -EINVAL);
		return -EINVAL;
	}

	opts.be = cli->args.be;
	opts.nsid = cli->args.dev_nsid;

	/* A comma-separated list, so one run can drive as many controllers as
	 * the machine has rather than exactly two. */
	uris = strdup(cli->args.alt_uri);
	if (!uris) {
		return -errno;
	}
	for (char *uri = strtok(uris, ","); uri; uri = strtok(NULL, ",")) {
		if (ndev == CTRLRS_MAX) {
			err = -E2BIG;
			xnvme_cli_perr("More controllers than this test drives", err);
			goto exit;
		}

		dev[ndev] = xnvme_dev_open(uri, &opts);
		if (!dev[ndev]) {
			err = -errno;
			xnvme_cli_perr("xnvme_dev_open(alt_uri)", err);
			goto exit;
		}
		++ndev;
	}

	nbytes = xnvme_dev_get_geo(dev[0])->lba_nbytes;
	if (!nbytes) {
		err = -EINVAL;
		xnvme_cli_perr("Unusable lba_nbytes", err);
		goto exit;
	}
	for (int i = 1; i < ndev; ++i) {
		if (xnvme_dev_get_geo(dev[i])->lba_nbytes == nbytes) {
			continue;
		}
		err = -EINVAL;
		xnvme_cli_perr("The controllers disagree on lba_nbytes", err);
		goto exit;
	}

	host_w = xnvme_buf_virt_alloc(0x1000, nbytes);
	host_r = xnvme_buf_virt_alloc(0x1000, nbytes);
	if (!host_w || !host_r) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_virt_alloc()", err);
		goto exit;
	}

	for (int i = 0; i < ndev; ++i) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev[i]);

		buf[i] = xnvme_buf_alloc(dev[i], nbytes);
		if (!buf[i]) {
			err = -errno;
			xnvme_cli_perr("xnvme_buf_alloc()", err);
			goto exit;
		}

		memset(host_w, 0xA0 + i, nbytes);
		err = xnvme_buf_memcpy(buf[i], host_w, nbytes);
		if (err) {
			xnvme_cli_perr("xnvme_buf_memcpy(to device)", err);
			goto exit;
		}

		err = xnvme_nvm_write(&ctx, xnvme_dev_get_nsid(dev[i]), 0, 0, buf[i], NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			err = err ? err : -EIO;
			xnvme_cli_perr("xnvme_nvm_write()", err);
			goto exit;
		}
	}

	/* Every controller written before any is read, so a read reaching the
	 * wrong one returns that controller's pattern rather than its own. */
	for (int i = 0; i < ndev; ++i) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev[i]);

		memset(host_r, 0, nbytes);
		err = xnvme_buf_memcpy(buf[i], host_r, nbytes);
		if (err) {
			xnvme_cli_perr("xnvme_buf_memcpy(clearing)", err);
			goto exit;
		}

		err = xnvme_nvm_read(&ctx, xnvme_dev_get_nsid(dev[i]), 0, 0, buf[i], NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			err = err ? err : -EIO;
			xnvme_cli_perr("xnvme_nvm_read()", err);
			goto exit;
		}

		err = xnvme_buf_memcpy(host_r, buf[i], nbytes);
		if (err) {
			xnvme_cli_perr("xnvme_buf_memcpy(from device)", err);
			goto exit;
		}

		memset(host_w, 0xA0 + i, nbytes);
		if (memcmp(host_w, host_r, nbytes)) {
			err = -EIO;
			xnvme_cli_pinf("controller %d of %d returned another pattern", i, ndev);
			xnvme_cli_perr("The controller did not return what was written", err);
			goto exit;
		}
	}

exit:
	for (int i = 0; i < ndev; ++i) {
		xnvme_buf_free(dev[i], buf[i]);
	}
	xnvme_buf_virt_free(host_w);
	xnvme_buf_virt_free(host_r);
	for (int i = 1; i < ndev; ++i) {
		xnvme_dev_close(dev[i]);
	}
	free(uris);

	return err;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"io",
		"Write and read both controllers from one process",
		"Write and read both controllers from one process",
		sub_io,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_ALT_URI, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_DEV_NSID, XNVME_CLI_LOPT},
			XNVME_CLI_CORE_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Multi-controller verification",
	.descr_short = "Drive two controllers from one process",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
