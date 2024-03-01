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
 *  Abide to COPY format zero max entries
 */
static int
test_copy(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	const struct xnvme_spec_idfy_ns *ns = xnvme_dev_get_ns(dev);
	uint32_t nsid;
	struct xnvme_cmd_ctx ctx = {0};

	char *wbuf = NULL, *rbuf = NULL;
	struct xnvme_spec_nvm_scopy_fmt_zero *source_range = NULL;
	uint64_t nlb, nbytes, sdlba, slba;
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

	if (cli->given[XNVME_CLI_OPT_NLB]) {
		nlb = cli->args.nlb + 1; // NOTE nlb is zero-based
		if (nlb > ns->mssrl) {
			xnvme_cli_pinf("Failed: nlb > ns->mssrl");
			return -EINVAL;
		}
	} else {
		nlb = ns->mssrl;
	}

	nbytes = nlb * geo->lba_nbytes;
	if (nbytes > geo->mdts_nbytes) {
		xnvme_cli_pinf("nbytes > mdts_nbytes -> limiting nlb to mdts_nbytes / lba_nbytes");
		nlb = geo->mdts_nbytes / geo->lba_nbytes;
		nbytes = geo->mdts_nbytes;
	}

	slba = cli->given[XNVME_CLI_OPT_SLBA] ? cli->args.slba : 0x0;

	if (cli->given[XNVME_CLI_OPT_SDLBA]) {
		sdlba = cli->args.sdlba;
		// Make sure there's no overlap between src and dest
		// Because nlb is incremented by 1 above, slba + nlb == sdlba is legal
		if (slba < sdlba && slba + nlb > sdlba) {
			xnvme_cli_pinf("Overlap between src and dest: slba < sdlba && slba + nlb "
				       "> sdlba");
			return -EINVAL;
		} else if (slba > sdlba && sdlba + nlb < slba) {
			xnvme_cli_pinf("Overlap between src and dest: slba > sdlba && sdlba + nlb "
				       "< slba");
			return -EINVAL;
		} else if (slba == sdlba) {
			xnvme_cli_pinf("Overlap between src and dest: slba == sdlba");
			return -EINVAL;
		}
	} else {
		sdlba = slba + nlb;
	}

	wbuf = xnvme_buf_alloc(dev, nbytes);
	if (!wbuf) {
		err = errno;
		XNVME_DEBUG("Failed: allocating write buffer");
		goto exit;
	}
	xnvme_buf_fill(wbuf, nbytes, "anum");

	rbuf = xnvme_buf_alloc(dev, nbytes);
	if (!rbuf) {
		err = errno;
		XNVME_DEBUG("Failed: allocating read buffer");
		goto exit;
	}
	xnvme_buf_fill(rbuf, nbytes, "0");

	source_range = xnvme_buf_alloc(dev, sizeof(*source_range));
	if (!source_range) {
		err = errno;
		XNVME_DEBUG("Failed: allocating range buffer");
		goto exit;
	}
	source_range->slba = slba;
	source_range->nlb = nlb - 1;

	// Write data to the buffer
	nsid = xnvme_dev_get_nsid(dev);
	ctx = xnvme_cmd_ctx_from_dev(dev);

	err = xnvme_nvm_write(&ctx, nsid, slba, (nlb - 1), wbuf, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_pinf("xnvme_nvm_write(): "
			       "{err: 0x%x, slba: 0x%016lx}",
			       err, slba);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		goto exit;
	}

	// Copy LBA's
	err = xnvme_nvm_scopy(&ctx, nsid, sdlba, source_range, 0, XNVME_NVM_SCOPY_FMT_ZERO);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_pinf("xnvme_nvm_copy(): "
			       "{err: 0x%x, slba: 0x%016lx}",
			       err, slba);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		goto exit;
	}

	// Read back LBA's
	err = xnvme_nvm_read(&ctx, nsid, sdlba, nlb - 1, rbuf, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_pinf("xnvme_nvm_read(): "
			       "{err: 0x%x, slba: 0x%016lx}",
			       err, slba);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		goto exit;
	}

	// Compare LBA's
	xnvme_cli_pinf("Comparing wbuf and rbuf");
	if (xnvme_buf_diff(wbuf, rbuf, nbytes)) {
		xnvme_buf_diff_pr(wbuf, rbuf, nbytes, XNVME_PR_DEF);
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
			{XNVME_CLI_OPT_NLB, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_SDLBA, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Basic Copy Test Verification",
	.descr_short = "Basic Copy Test Verification",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
