// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <string.h>
#include <libxnvme.h>
#include <xnvme_dev.h>

static struct xnvme_dev *
ctrlr_open(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = {0};
	struct xnvme_dev *dev;
	int err;

	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return NULL;
	}

	opts.nsid = 0;

	dev = xnvme_dev_open(cli->args.uri, &opts);
	if (!dev) {
		xnvme_cli_perr("xnvme_dev_open()", -errno);
		return NULL;
	}

	return dev;
}

static int
test_ctrlr_open(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev;
	const struct xnvme_ident *ident;

	dev = ctrlr_open(cli);
	if (!dev) {
		return -errno;
	}

	ident = xnvme_dev_get_ident(dev);
	if (ident->dtype != XNVME_DEV_TYPE_NVME_CONTROLLER) {
		xnvme_cli_pinf("FAILED: expected dtype CONTROLLER(%d), got %d",
			       XNVME_DEV_TYPE_NVME_CONTROLLER, ident->dtype);
		xnvme_dev_close(dev);
		return -EINVAL;
	}

	xnvme_cli_pinf("LGTM: controller-only dev_open");
	xnvme_dev_close(dev);
	return 0;
}

static int
test_ctrlr_idfy(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev;
	struct xnvme_spec_idfy *idfy = NULL;
	struct xnvme_cmd_ctx ctx;
	int err;

	dev = ctrlr_open(cli);
	if (!dev) {
		return -errno;
	}

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy));
	if (!idfy) {
		xnvme_cli_perr("xnvme_buf_alloc()", -ENOMEM);
		xnvme_dev_close(dev);
		return -ENOMEM;
	}
	memset(idfy, 0, sizeof(*idfy));

	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ctrlr(&ctx, idfy);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_idfy_ctrlr()", err ? err : -EIO);
		xnvme_buf_free(dev, idfy);
		xnvme_dev_close(dev);
		return err ? err : -EIO;
	}

	if (!idfy->ctrlr.mn[0]) {
		xnvme_cli_pinf("FAILED: model number is empty");
		xnvme_buf_free(dev, idfy);
		xnvme_dev_close(dev);
		return -EINVAL;
	}

	xnvme_cli_pinf("LGTM: Identify Controller via controller-only handle");
	xnvme_buf_free(dev, idfy);
	xnvme_dev_close(dev);
	return 0;
}

static int
test_ctrlr_nslist(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev;
	struct xnvme_spec_idfy *idfy = NULL;
	struct xnvme_cmd_ctx ctx;
	uint32_t *nslist;
	int err;

	dev = ctrlr_open(cli);
	if (!dev) {
		return -errno;
	}

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy));
	if (!idfy) {
		xnvme_cli_perr("xnvme_buf_alloc()", -ENOMEM);
		xnvme_dev_close(dev);
		return -ENOMEM;
	}
	memset(idfy, 0, sizeof(*idfy));

	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy(&ctx, XNVME_SPEC_IDFY_NSLIST, 0, 0, 0, 0, idfy);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_idfy(NSLIST)", err ? err : -EIO);
		xnvme_buf_free(dev, idfy);
		xnvme_dev_close(dev);
		return err ? err : -EIO;
	}

	nslist = (uint32_t *)idfy;
	if (!nslist[0]) {
		xnvme_cli_pinf("FAILED: namespace list is empty");
		xnvme_buf_free(dev, idfy);
		xnvme_dev_close(dev);
		return -ENOENT;
	}

	xnvme_cli_pinf("Active namespaces:");
	for (int i = 0; i < 1024 && nslist[i]; ++i) {
		xnvme_cli_pinf("  nsid: %u", nslist[i]);
	}

	xnvme_cli_pinf("LGTM: Identify Namespace List via controller-only handle");
	xnvme_buf_free(dev, idfy);
	xnvme_dev_close(dev);
	return 0;
}

//
// Command-Line Interface (CLI) definition
//
static int
test_ctrlr_gfeat_nqueues(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = ctrlr_open(cli);
	uint32_t nsq = 0, ncq = 0;
	struct xnvme_spec_feat feat = {.val = 0};
	struct xnvme_cmd_ctx ctx;
	int err;

	if (!dev) {
		return -errno;
	}

	err = xnvme_adm_gfeat_nqueues(NULL, &nsq, &ncq);
	if (err != -EINVAL) {
		xnvme_cli_pinf("FAILED: no ctx should be -EINVAL, got %d", err);
		err = err ? err : -EIO;
		goto exit;
	}

	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_gfeat_nqueues(&ctx, NULL, &ncq);
	if (err != -EINVAL) {
		xnvme_cli_pinf("FAILED: no output should be -EINVAL, got %d", err);
		err = err ? err : -EIO;
		goto exit;
	}

	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_gfeat_nqueues(&ctx, &nsq, &ncq);
	if (err) {
		xnvme_cli_perr("xnvme_adm_gfeat_nqueues()", err);
		goto exit;
	}

	/* The helper exists to add the one the spec leaves out, so the check
	 * that matters is against the raw feature rather than against a range:
	 * a helper returning NSQA unchanged would still look plausible. */
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_gfeat(&ctx, 0x0, XNVME_SPEC_FEAT_NQUEUES, XNVME_SPEC_FEAT_SEL_CURRENT,
			      NULL, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_gfeat()", err);
		err = err ? err : -EIO;
		goto exit;
	}
	feat.val = ctx.cpl.cdw0;

	if ((nsq != (uint32_t)feat.nqueues.nsqa + 1) || (ncq != (uint32_t)feat.nqueues.ncqa + 1)) {
		xnvme_cli_pinf("FAILED: nsq(%u) ncq(%u) vs nsqa(%u) ncqa(%u)", nsq, ncq,
			       feat.nqueues.nsqa, feat.nqueues.ncqa);
		err = -EIO;
		goto exit;
	}

	xnvme_cli_pinf("nsq: %u, ncq: %u", nsq, ncq);
	err = 0;

exit:
	xnvme_dev_close(dev);

	return err;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"gfeat-nqueues",
		"Number of Queues, counted rather than zero-based",
		"Number of Queues, counted rather than zero-based",
		test_ctrlr_gfeat_nqueues,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"open",
		"Open controller with nsid=0, verify dtype",
		"Open controller with nsid=0, verify dtype",
		test_ctrlr_open,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"idfy",
		"Open controller, send Identify Controller",
		"Open controller, send Identify Controller",
		test_ctrlr_idfy,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"nslist",
		"Open controller, send Identify Namespace List",
		"Open controller, send Identify Namespace List",
		test_ctrlr_nslist,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Controller-only dev_open tests",
	.descr_short = "Test opening NVMe controllers with nsid=0",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
