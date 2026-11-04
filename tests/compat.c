// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <string.h>
#include <libxnvme.h>

static int
sub_deprecated(struct xnvme_cli *XNVME_UNUSED(cli))
{
	struct xnvme_opts expected = {0};
	int err = 0;

	xnvme_opts_set_defaults(&expected);

	for (int i = 0; i < 2; ++i) {
		struct xnvme_opts opts = xnvme_opts_default();

		if (memcmp(&opts, &expected, sizeof(opts))) {
			xnvme_cli_pinf(
				"FAILED: xnvme_opts_default() != xnvme_opts_set_defaults()");
			err = -EINVAL;
		}
	}

	return err;
}

static int
sub_null_args(struct xnvme_cli *XNVME_UNUSED(cli))
{
	int err = 0;

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
