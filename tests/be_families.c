// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stdlib.h>

#include <libxnvme.h>

static int
open_alt(struct xnvme_cli *cli, struct xnvme_dev **alt_dev)
{
	struct xnvme_opts opts = xnvme_opts_default();
	const char *uri = cli->args.uri;
	int err;

	opts.be = cli->args.alt_be;
	opts.nsid = cli->args.dev_nsid;

	*alt_dev = xnvme_dev_open(uri, &opts);
	if (!(*alt_dev)) {
		err = -errno;
		xnvme_cli_perr("xnvme_dev_open(alt_dev)", err);
		return err;
	}

	return 0;
}

static int
sub_io_cmp(struct xnvme_cli *cli)
{
	struct xnvme_dev *alt_dev, *dev = cli->args.dev;
	const struct xnvme_geo *geo;
	struct xnvme_cmd_ctx ctx;
	void *write_buf = NULL, *read_buf = NULL;
	uint32_t iosize, nsid;
	uint16_t nlb;
	size_t diff = 0;
	int err;

	nsid = cli->args.dev_nsid;

	err = open_alt(cli, &alt_dev);
	if (err) {
		return err;
	}

	geo = xnvme_dev_get_geo(dev);
	if (!geo->lba_nbytes) {
		err = -EINVAL;
		xnvme_cli_perr("Error: LBA size == 0", err);
		goto exit;
	}

	iosize = geo->lba_nbytes;
	nlb = 0;

	write_buf = xnvme_buf_alloc(dev, iosize);
	if (!write_buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc(write)", err);
		goto exit;
	}

	err = xnvme_buf_fill(write_buf, iosize, "anum");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill(write)", err);
		goto exit;
	}

	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_nvm_write(&ctx, nsid, 0, nlb, write_buf, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_nvm_write()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	read_buf = xnvme_buf_alloc(alt_dev, iosize);
	if (!read_buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc(read)", err);
		goto exit;
	}

	err = xnvme_buf_fill(read_buf, iosize, "zero");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill(read)", err);
		goto exit;
	}

	ctx = xnvme_cmd_ctx_from_dev(alt_dev);
	err = xnvme_nvm_read(&ctx, nsid, 0, nlb, read_buf, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_nvm_read()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	err = xnvme_buf_diff(write_buf, read_buf, iosize, &diff);
	if (err) {
		xnvme_cli_perr("xnvme_buf_diff()", err);
		goto exit;
	}

	if (diff) {
		fprintf(stderr, "FAIL: %zu/%u bytes differ\n", diff, iosize);
		err = -EIO;
	} else {
		printf("PASS: %u bytes match\n", iosize);
	}

exit:
	xnvme_buf_free(dev, write_buf);
	xnvme_buf_free(alt_dev, read_buf);
	xnvme_dev_close(alt_dev);

	return err;
}

static int
sub_idfy_cmp(struct xnvme_cli *cli)
{
	struct xnvme_dev *alt_dev, *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx;
	struct xnvme_spec_idfy *idfy = NULL, *alt_idfy = NULL;
	uint32_t nsid;
	size_t diff = 0;
	int err;

	nsid = cli->args.dev_nsid;
	err = open_alt(cli, &alt_dev);
	if (err) {
		return err;
	}

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy));
	if (!idfy) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc(idfy)", err);
		goto exit;
	}
	xnvme_buf_clear(idfy, sizeof(*idfy));

	alt_idfy = xnvme_buf_alloc(alt_dev, sizeof(*alt_idfy));
	if (!alt_idfy) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc(alt_idfy)", err);
		goto exit;
	}
	xnvme_buf_clear(alt_idfy, sizeof(*alt_idfy));

	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ns(&ctx, nsid, idfy);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_idfy_ns(--be)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	ctx = xnvme_cmd_ctx_from_dev(alt_dev);
	err = xnvme_adm_idfy_ns(&ctx, nsid, alt_idfy);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_idfy_ns(--alt-be)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	err = xnvme_buf_diff(idfy, alt_idfy, sizeof(*idfy), &diff);
	if (err) {
		xnvme_cli_perr("xnvme_buf_diff()", err);
		goto exit;
	}

	if (diff) {
		fprintf(stderr, "FAIL: identify responses differ by %zu bytes\n", diff);
		err = -EIO;
	} else {
		printf("PASS: identify controller responses match (%zu bytes)\n", sizeof(*idfy));
	}

exit:
	xnvme_buf_free(dev, idfy);
	xnvme_buf_free(alt_dev, alt_idfy);
	xnvme_dev_close(alt_dev);

	return err;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"io-cmp",
		"Write via --be, read back via --alt-be, compare",
		"Write a known pattern to LBA 0 through the --be backend, read it back\n"
		"through the --alt-be backend, and verify the data matches.\n"
		"Both backends share the same NVMe controller via the compat-family path.\n"
		"NOTE: overwrites LBA 0; only use on a device where that is safe.",
		sub_io_cmp,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_BE, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_ALT_BE, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_DEV_NSID, XNVME_CLI_LREQ},
		},
	},
	{
		"idfy-cmp",
		"Send identify controller via --be and --alt-be, compare responses",
		"Issue an Identify Controller command through each of the two backends\n"
		"sharing the same NVMe controller, then byte-compare the responses.\n"
		"A mismatch indicates the compat-family open path returned inconsistent data.",
		sub_idfy_cmp,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_BE, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_ALT_BE, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_DEV_NSID, XNVME_CLI_LREQ},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "be-families - Backend family coexistence tests",
	.descr_short = "Test that compatible backends can share the same NVMe controller",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
