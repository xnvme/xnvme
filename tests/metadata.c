// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <errno.h>

/**
 * 1) Format to 512 dbuf and 64 mbuf:
 * 2) Write dbuf and mbuf to device
 * 3) Read dbuf and mbuf from device
 * 4) Verify that the content of the read is the same as the write
 */
static int
test_buf(struct xnvme_cli *cli)
{
	int err;
	int lbaf_i = 3; // index of lba format with 512 dbuf / 64 mbuf
	struct xnvme_dev *dev = cli->args.dev;
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	const struct xnvme_spec_idfy_ns *ns = xnvme_dev_get_ns(dev);
	char *w_dbuf, *r_dbuf;
	char *w_mbuf, *r_mbuf;

	xnvme_cli_pinf("Formatting device to 512 dbuf / 64 mbuf");
	err = xnvme_adm_format(&ctx, nsid, lbaf_i, 0, 0, 0, 0, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_format()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
	}

	struct xnvme_spec_lbaf lbaf = ns->lbaf[lbaf_i];
	size_t ds = 1 << lbaf.ds;

	w_dbuf = xnvme_buf_alloc(dev, ds);
	if (!w_dbuf) {
		err = -ENOMEM;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		return err;
	}

	r_dbuf = xnvme_buf_alloc(dev, ds);
	if (!r_dbuf) {
		err = -ENOMEM;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		xnvme_buf_free(dev, w_dbuf);
		return err;
	}

	if (lbaf.ms) {
		w_mbuf = xnvme_buf_alloc(dev, lbaf.ms);
		if (!w_mbuf) {
			err = -ENOMEM;
			xnvme_cli_perr("xnvme_buf_alloc()", err);
			xnvme_buf_free(dev, w_dbuf);
			xnvme_buf_free(dev, r_dbuf);
			return err;
		}

		r_mbuf = xnvme_buf_alloc(dev, lbaf.ms);
		if (!r_mbuf) {
			err = -ENOMEM;
			xnvme_cli_perr("xnvme_buf_alloc()", err);
			xnvme_buf_free(dev, w_dbuf);
			xnvme_buf_free(dev, r_dbuf);
			xnvme_buf_free(dev, w_mbuf);
			return err;
		}

		err = xnvme_buf_fill(w_mbuf, lbaf.ms, "rand-t");
		if (err) {
			xnvme_cli_perr("xnvme_buf_fill()", err);
			goto exit;
		}

		memset(r_mbuf, 0, lbaf.ms);
	}

	err = xnvme_buf_fill(w_dbuf, ds, "rand-t");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill()", err);
		goto exit;
	}

	memset(r_dbuf, 0, ds);

	ctx.cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
	ctx.cmd.common.nsid = nsid;
	ctx.cmd.nvm.slba = 0;
	ctx.cmd.nvm.nlb = 0;
	err = xnvme_cmd_pass(&ctx, w_dbuf, ds, w_mbuf, lbaf.ms);
	if (err) {
		xnvme_cli_perr("xnvme_cmd_pass(WRITE)", err);
		goto exit;
	}

	ctx.cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
	err = xnvme_cmd_pass(&ctx, r_dbuf, ds, r_mbuf, lbaf.ms);
	if (err) {
		xnvme_cli_perr("xnvme_cmd_pass(READ)", err);
		goto exit;
	}

	xnvme_cli_pinf("Comparing WRITE dbuf and READ dbuf");
	if (xnvme_buf_diff(w_dbuf, r_dbuf, ds)) {
		xnvme_buf_diff_pr(w_dbuf, r_dbuf, ds, XNVME_PR_DEF);
		err = -EIO;
		goto exit;
	}
	if (lbaf.ms) {
		xnvme_cli_pinf("Comparing WRITE mbuf and READ mbuf");
		if (xnvme_buf_diff(w_mbuf, r_mbuf, lbaf.ms)) {
			xnvme_buf_diff_pr(w_mbuf, r_mbuf, lbaf.ms, XNVME_PR_DEF);
			err = -EIO;
			goto exit;
		}
	}

exit:
	xnvme_buf_free(dev, w_dbuf);
	xnvme_buf_free(dev, r_dbuf);
	if (lbaf.ms) {
		xnvme_buf_free(dev, w_mbuf);
		xnvme_buf_free(dev, r_mbuf);
	}

	return err;
}

/**
 * 1) Format to 512 dbuf and 64 mbuf:
 * 2) Write dvec and mbuf to device
 * 3) Read dvec and mbuf from device
 * 4) Verify that the content of the read is the same as the write
 */
static int
test_iovec(struct xnvme_cli *cli)
{
	int err;
	int lbaf_i = 3; // index of lba format with 512 dbuf / 64 mbuf
	struct xnvme_dev *dev = cli->args.dev;
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	const struct xnvme_spec_idfy_ns *ns = xnvme_dev_get_ns(dev);
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	struct iovec *w_dvec, *r_dvec;
	char *w_mbuf, *r_mbuf;

	xnvme_cli_pinf("Formatting device to 512 dbuf / 64 mbuf");
	err = xnvme_adm_format(&ctx, nsid, lbaf_i, 0, 0, 0, 0, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_format()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
	}

	struct xnvme_spec_lbaf lbaf = ns->lbaf[lbaf_i];
	size_t ds = 1 << lbaf.ds;

	w_dvec = xnvme_buf_alloc(dev, sizeof(*w_dvec));
	if (!w_dvec) {
		err = -ENOMEM;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		return err;
	}

	w_dvec->iov_base = xnvme_buf_alloc(dev, ds);
	if (!w_dvec->iov_base) {
		err = -ENOMEM;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		xnvme_buf_free(dev, w_dvec);
		return err;
	}

	r_dvec = xnvme_buf_alloc(dev, sizeof(*r_dvec));
	if (!r_dvec) {
		err = -ENOMEM;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		xnvme_buf_free(dev, w_dvec->iov_base);
		xnvme_buf_free(dev, w_dvec);
		return err;
	}

	r_dvec->iov_base = xnvme_buf_alloc(dev, ds);
	if (!r_dvec->iov_base) {
		err = -ENOMEM;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		xnvme_buf_free(dev, w_dvec->iov_base);
		xnvme_buf_free(dev, w_dvec);
		xnvme_buf_free(dev, r_dvec);
		return err;
	}
	if (lbaf.ms) {
		w_mbuf = xnvme_buf_alloc(dev, lbaf.ms);
		if (!w_mbuf) {
			err = -ENOMEM;
			xnvme_cli_perr("xnvme_buf_alloc()", err);
			xnvme_buf_free(dev, w_dvec->iov_base);
			xnvme_buf_free(dev, w_dvec);
			xnvme_buf_free(dev, r_dvec->iov_base);
			xnvme_buf_free(dev, r_dvec);
			return err;
		}

		r_mbuf = xnvme_buf_alloc(dev, lbaf.ms);
		if (!r_mbuf) {
			err = -ENOMEM;
			xnvme_cli_perr("xnvme_buf_alloc()", err);
			xnvme_buf_free(dev, w_dvec->iov_base);
			xnvme_buf_free(dev, w_dvec);
			xnvme_buf_free(dev, r_dvec->iov_base);
			xnvme_buf_free(dev, r_dvec);
			xnvme_buf_free(dev, w_mbuf);
			return err;
		}

		err = xnvme_buf_fill(w_mbuf, lbaf.ms, "rand-t");
		if (err) {
			xnvme_cli_perr("xnvme_buf_fill()", err);
			goto exit;
		}

		memset(r_mbuf, 0, lbaf.ms);
	}

	err = xnvme_buf_fill(w_dvec->iov_base, ds, "rand-t");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill()", err);
		goto exit;
	}
	w_dvec->iov_len = ds;

	memset(r_dvec->iov_base, 0, ds);
	r_dvec->iov_len = ds;

	ctx.cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
	ctx.cmd.common.nsid = nsid;
	ctx.cmd.nvm.slba = 0;
	ctx.cmd.nvm.nlb = 0;

	err = xnvme_cmd_pass_iov(&ctx, w_dvec, 1, ds, w_mbuf, lbaf.ms);
	if (err) {
		xnvme_cli_perr("xnvme_cmd_pass_iov(WRITE)", err);
		goto exit;
	}

	ctx.cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;

	err = xnvme_cmd_pass_iov(&ctx, r_dvec, 1, ds, r_mbuf, lbaf.ms);
	if (err) {
		xnvme_cli_perr("xnvme_cmd_pass_iov(READ)", err);
		goto exit;
	}

	xnvme_cli_pinf("Comparing WRITE dbuf and READ dbuf");
	if (xnvme_buf_diff(w_dvec->iov_base, r_dvec->iov_base, ds)) {
		xnvme_buf_diff_pr(w_dvec->iov_base, r_dvec->iov_base, ds, XNVME_PR_DEF);
		err = -EIO;
		goto exit;
	}
	if (lbaf.ms) {
		xnvme_cli_pinf("Comparing WRITE mbuf and READ mbuf");
		if (xnvme_buf_diff(w_mbuf, r_mbuf, lbaf.ms)) {
			xnvme_buf_diff_pr(w_mbuf, r_mbuf, lbaf.ms, XNVME_PR_DEF);
			err = -EIO;
			goto exit;
		}
	}

exit:
	xnvme_buf_free(dev, w_dvec->iov_base);
	xnvme_buf_free(dev, w_dvec);
	xnvme_buf_free(dev, r_dvec->iov_base);
	xnvme_buf_free(dev, r_dvec);
	if (lbaf.ms) {
		xnvme_buf_free(dev, w_mbuf);
		xnvme_buf_free(dev, r_mbuf);
	}

	return err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"buf",
		"Verification of metadata using the regular IO path",
		"Verification of metadata using the regular IO path",
		test_buf,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"iovec",
		"Verification of metadata using the vectored IO path",
		"Verification of metadata using the vectored IO path",
		test_iovec,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},

			XNVME_CLI_SYNC_OPTS,
		},

	}};

static struct xnvme_cli g_cli = {
	.title = "Verification of metadata",
	.descr_short = "Verification of metadata",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}