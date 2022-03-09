#include <errno.h>
#include <libxnvme_file.h>
#include <libxnvmec.h>

int
xnvmec_copy_io(struct xnvmec *cli)
{
	const char *input_path = cli->args.data_input, *output_path = cli->args.data_output;
	struct xnvme_opts opts = {.rdonly = 1};
	struct xnvme_dev *fh;
	void *buf;
	size_t file_size;
	int err = 0;

	fh = xnvme_file_open(input_path, &opts);
	if (fh == NULL) {
		xnvmec_perr("xnvme_file_open()", errno);
		return -errno;
	}
	file_size = xnvme_dev_get_geo(fh)->tbytes;

	buf = xnvme_buf_alloc(fh, file_size);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", errno);
		xnvme_file_close(fh);
		return -errno;
	}

	err = xnvmec_buf_from_file(buf, file_size, input_path);
	if (err) {
		xnvmec_perr("xnvme_buf_from_file()", errno);
		goto exit;
	}

	err = xnvmec_buf_to_file(buf, file_size, output_path);
	if (err) {
		xnvmec_perr("xnvmec_buf_to_file()", errno);
		goto exit;
	}

exit:
	xnvme_buf_free(fh, buf);
	xnvme_file_close(fh);
	return err;
}

int
xnvmec_check_opt_attr(struct xnvmec *XNVME_UNUSED(cli))
{
	int err = 0;

	for (int opt = 1; opt < XNVMEC_OPT_END; ++opt) {
		const struct xnvmec_opt_attr *opt_attr;

		opt_attr = xnvmec_get_opt_attr(opt);
		if (opt_attr) {
			continue;
		}

		err = ENOSYS;
		xnvmec_pinf("ERR: !opt_attr for opt: %d", opt);
	}

	return err;
}

static struct xnvmec_sub g_subs[] = {
	{
		"copy-xnvmec",
		"Copy a file using xnvmec_buf_{to,from}_file",
		"Copy a file using xnvmec_buf_{to,from}_file",
		xnvmec_copy_io,
		{
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_POSA},
		},
	},
	{
		"check-opt-attr",
		"Check that all 'xnvmec_opt' has attributes defined",
		"Check that all 'xnvmec_opt' has attributes defined",
		xnvmec_check_opt_attr,
		{0},
	},
};

static struct xnvmec g_cli = {
	.title = "xNVMe command-line-interface exercises",
	.descr_short = "xNVMe command-line-interface exercises",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_NONE);
}
