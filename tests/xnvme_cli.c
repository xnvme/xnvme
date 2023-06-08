// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

int
xnvme_cli_copy_io(struct xnvme_cli *cli)
{
	const char *input_path = cli->args.data_input, *output_path = cli->args.data_output;
	struct xnvme_opts opts = {.rdonly = 1};
	struct xnvme_dev *fh;
	void *buf;
	size_t file_size;
	int err = 0;

	fh = xnvme_file_open(input_path, &opts);
	if (fh == NULL) {
		xnvme_cli_perr("xnvme_file_open()", errno);
		return -errno;
	}
	file_size = xnvme_dev_get_geo(fh)->tbytes;

	buf = xnvme_buf_alloc(fh, file_size);
	if (!buf) {
		xnvme_cli_perr("xnvme_buf_alloc()", errno);
		xnvme_file_close(fh);
		return -errno;
	}

	err = xnvme_buf_from_file(buf, file_size, input_path);
	if (err) {
		xnvme_cli_perr("xnvme_buf_from_file()", errno);
		goto exit;
	}

	err = xnvme_buf_to_file(buf, file_size, output_path);
	if (err) {
		xnvme_cli_perr("xnvme_buf_to_file()", errno);
		goto exit;
	}

exit:
	xnvme_buf_free(fh, buf);
	xnvme_file_close(fh);
	return err;
}

int
xnvme_cli_check_opt_attr(struct xnvme_cli *XNVME_UNUSED(cli))
{
	int err = 0;

	for (int opt = 1; opt < XNVME_CLI_OPT_END; ++opt) {
		const struct xnvme_cli_opt_attr *opt_attr;

		opt_attr = xnvme_cli_get_opt_attr(opt);
		if (opt_attr) {
			continue;
		}

		err = ENOSYS;
		xnvme_cli_pinf("ERR: !opt_attr for opt: %d", opt);
	}

	return err;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"copy-xnvme_cli_run",
		"Copy a file using xnvme_buf_{to,from}_file",
		"Copy a file using xnvme_buf_{to,from}_file",
		xnvme_cli_copy_io,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_DATA_INPUT, XNVME_CLI_POSA},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
		},
	},
	{
		"check-opt-attr",
		"Check that all 'xnvme_cli_opt' has attributes defined",
		"Check that all 'xnvme_cli_opt' has attributes defined",
		xnvme_cli_check_opt_attr,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "xNVMe command-line-interface exercises",
	.descr_short = "xNVMe command-line-interface exercises",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
