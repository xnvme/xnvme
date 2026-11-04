// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <string.h>
#include <libxnvme.h>

static int
sub_deprecated(struct xnvme_cli *XNVME_UNUSED(cli))
{
	struct xnvme_dev *dev = (struct xnvme_dev *)&dev; // Any handle; it is only stored
	struct xnvme_opts expected = {0};
	struct xnvme_cmd_ctx want = {0};
	int err = 0;

	xnvme_opts_set_defaults(&expected);
	xnvme_dev_get_cmd_ctx(dev, &want);

	for (int i = 0; i < 2; ++i) {
		struct xnvme_opts opts = xnvme_opts_default();
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		struct xnvme_cmd_ctx fctx = xnvme_file_get_cmd_ctx(dev);

		if (memcmp(&opts, &expected, sizeof(opts))) {
			xnvme_cli_pinf(
				"FAILED: xnvme_opts_default() != xnvme_opts_set_defaults()");
			err = -EINVAL;
		}
		if (ctx.dev != want.dev || ctx.opts != want.opts) {
			xnvme_cli_pinf(
				"FAILED: xnvme_cmd_ctx_from_dev() != xnvme_dev_get_cmd_ctx()");
			err = -EINVAL;
		}
		if (fctx.dev != want.dev || fctx.opts != want.opts) {
			xnvme_cli_pinf(
				"FAILED: xnvme_file_get_cmd_ctx() != xnvme_dev_get_cmd_ctx()");
			err = -EINVAL;
		}
	}

	return err;
}

static int
sub_null_args(struct xnvme_cli *XNVME_UNUSED(cli))
{
	struct xnvme_dev *dev = (struct xnvme_dev *)&dev; // Any handle; it is only stored
	struct xnvme_cmd_ctx ctx = {0};
	int err = 0;

	if (xnvme_dev_get_cmd_ctx(NULL, &ctx) != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_dev_get_cmd_ctx(NULL, ctx) != -EINVAL");
		err = -EINVAL;
	}
	if (xnvme_dev_get_cmd_ctx(dev, NULL) != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_dev_get_cmd_ctx(dev, NULL) != -EINVAL");
		err = -EINVAL;
	}
	if (xnvme_dev_get_cmd_ctx(dev, &ctx) || ctx.dev != dev) {
		xnvme_cli_pinf("FAILED: xnvme_dev_get_cmd_ctx(dev, ctx)");
		err = -EINVAL;
	}
	if (xnvme_opts_set_defaults(NULL) != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_opts_set_defaults(NULL) != -EINVAL");
		err = -EINVAL;
	}

	return err;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"deprecated",
		"Call the deprecated functions",
		"Call the deprecated functions",
		sub_deprecated,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
	{
		"null-args",
		"Pass NULL to the functions filling an output argument",
		"Pass NULL to the functions filling an output argument",
		sub_null_args,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Exercise the deprecated functions of libxnvme_compat.h",
	.descr_short = "Exercise the deprecated functions of libxnvme_compat.h",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
