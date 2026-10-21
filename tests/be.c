// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

/**
 * Device-open selects a backend config by matching the URI's classification
 * against the config's caps, so a config without caps is never selected for a
 * classified URI
 */
static int
test_caps(struct xnvme_cli *XNVME_UNUSED(cli))
{
	int nerr = 0;
	int i;

	for (i = 0; xnvme_be_attr_get(i); ++i) {
		const struct xnvme_be_attr *attr = xnvme_be_attr_get(i);

		if (!attr->caps) {
			xnvme_cli_pinf("FAILED: backend config '%s' ('%s') has no caps",
				       attr->name, attr->descr ? attr->descr : "");
			nerr += 1;
		}
	}
	if (!i) {
		xnvme_cli_pinf("FAILED: no backend configs");
		return -ENOENT;
	}
	if (nerr) {
		return -EINVAL;
	}

	xnvme_cli_pinf("LGTM: %d backend configs, all with caps", i);
	return 0;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"caps",
		"Verify that every backend config declares its caps",
		"Verify that every backend config declares its caps",
		test_caps,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Test xNVMe backend configs",
	.descr_short = "Test xNVMe backend configs",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
