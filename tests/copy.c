// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

/**
 *  Test COPY command
 *  Copy using contiguously adress space.
 *
 *  a) Allocate buffers
 *  b) Populate with contiguous string af chars
 *  c) Copy payload
 *  d) Compare copied payload with original
 *
 *  Constraints:
 *  Abide to mdts constraint
 *  Abite to COPY format zero max entries
 */
static int
test_copy(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint32_t nsid;
	struct xnvme_cmd_ctx ctx = {0};

	char *wbuf = NULL, *rbuf = NULL;
	struct xnvme_spec_nvm_scopy_source_range *source_range = NULL;
	uint64_t xfer_naddr, xfer_nbytes, sdlba, slba;
	int err;

	switch (geo->type) {
	case XNVME_GEO_CONVENTIONAL:
	case XNVME_GEO_ZONED:
		break;
	default:
		XNVME_DEBUG("FAILED: not nvm / zns, got; %d", geo->type);
		return -EINVAL;
	}

	xnvme_dev_pr(dev, XNVME_PR_DEF);

	xfer_naddr = XNVME_MIN(geo->mdts_nbytes / geo->lba_nbytes, 128);
	xfer_nbytes = xfer_naddr * geo->lba_nbytes;
	slba = 0x0;
	sdlba = slba + xfer_naddr;

	wbuf = xnvme_buf_alloc(dev, xfer_nbytes);
	if (!wbuf) {
		err = errno;
		XNVME_DEBUG("Failed: allocating write buffer");
		goto exit;
	}
	xnvme_buf_fill(wbuf, xfer_nbytes, "anum");

	rbuf = xnvme_buf_alloc(dev, xfer_nbytes);
	if (!rbuf) {
		err = errno;
		XNVME_DEBUG("Failed: allocating read buffer");
		goto exit;
	}
	xnvme_buf_fill(rbuf, xfer_nbytes, "0");

	source_range = xnvme_buf_alloc(dev, sizeof(*source_range));
	if (!source_range) {
		err = errno;
		XNVME_DEBUG("Failed: allocating range buffer");
		goto exit;
	}
	source_range->entry[0].slba = slba;
	source_range->entry[0].nlb = xfer_naddr - 1;

	// Write data to the buffer
	nsid = xnvme_dev_get_nsid(dev);
	ctx = xnvme_cmd_ctx_from_dev(dev);

	err = xnvme_nvm_write(&ctx, nsid, slba, (xfer_naddr - 1), wbuf, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_pinf("xnvme_nvm_write(): "
			       "{err: 0x%x, slba: 0x%016lx}",
			       err, slba);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		goto exit;
	}

	// Copy LBA's
	err = xnvme_nvm_scopy(&ctx, nsid, sdlba, source_range->entry, xfer_naddr - 1,
			      XNVME_NVM_SCOPY_FMT_ZERO);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_pinf("xnvme_nvm_copy(): "
			       "{err: 0x%x, slba: 0x%016lx}",
			       err, slba);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		goto exit;
	}

	// Read back LBA's
	err = xnvme_nvm_read(&ctx, nsid, sdlba, xfer_naddr - 1, rbuf, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_pinf("xnvme_nvm_read(): "
			       "{err: 0x%x, slba: 0x%016lx}",
			       err, slba);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		goto exit;
	}

	// Compare LBA's
	xnvme_cli_pinf("Comparing wbuf and rbuf");
	if (xnvme_buf_diff(wbuf, rbuf, xfer_nbytes)) {
		xnvme_buf_diff_pr(wbuf, rbuf, xfer_nbytes, XNVME_PR_DEF);
		err = -EIO;
		goto exit;
	}

exit:
	xnvme_buf_free(dev, wbuf);
	xnvme_buf_free(dev, rbuf);
	xnvme_buf_free(dev, source_range);
	return err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"copy",
		"Basic test of copy functionality, write and read, to/from buffers",
		"Basic test of copy functionality, write and read, to/from buffers",
		test_copy,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ELBA, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Basic LBLK Test Verification",
	.descr_short = "Basic LBLK Test Verification",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
