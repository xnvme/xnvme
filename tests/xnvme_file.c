#include <errno.h>
#include <libxnvme_file.h>
#include <libxnvmec.h>

int
test_file_fsync(struct xnvmec *cli)
{
	struct xnvme_opts opts = {.create = 1, .wronly = 1};
	const char *output_path = cli->args.data_output;
	const size_t bytes_per_write = 1024;
	const size_t num_writes = 8;
	struct xnvme_dev *fh;
	char *buf;
	int err;

	fh = xnvme_file_open(output_path, &opts);
	if (fh == NULL) {
		xnvmec_perr("xnvme_file_open()", errno);
		return -errno;
	}

	buf = xnvme_buf_alloc(fh, bytes_per_write);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", errno);
		xnvme_file_close(fh);
		return -errno;
	}
	xnvmec_buf_fill(buf, bytes_per_write, "ascii");

	for (size_t i = 0; i < num_writes; i++) {
		struct xnvme_cmd_ctx ctx = xnvme_file_get_cmd_ctx(fh);

		err = xnvme_file_pwrite(&ctx, buf, bytes_per_write, 0);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvmec_perr("xnvme_file_pwrite()", err);
			return err;
		}

		err = xnvme_file_sync(fh);
		if (err) {
			xnvmec_perr("xnvme_file_sync()", err);
			return err;
		}
	}

	xnvme_buf_free(fh, buf);
	xnvme_file_close(fh);
	return 0;
}

/**
 * Opens a file at the given path with the given flags and writes ASCII
 * [33; 125] to it repeatedly, i.e. "!@#$%^&\()*+," and so on.
 */
static int
file_write_ascii(const char *path, size_t nbytes, struct xnvme_opts *opts)
{
	struct xnvme_dev *fh = NULL;
	struct xnvme_cmd_ctx ctx = {0};
	char *buf = NULL;
	int err;

	fh = xnvme_file_open(path, opts);
	if (fh == NULL) {
		xnvmec_perr("xnvme_file_open()", errno);
		return -errno;
	}

	buf = xnvme_buf_alloc(fh, nbytes);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", errno);
		xnvme_file_close(fh);
		return -errno;
	}
	xnvmec_buf_fill(buf, nbytes, "ascii");

	ctx = xnvme_file_get_cmd_ctx(fh);
	err = xnvme_file_pwrite(&ctx, buf, nbytes, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_file_pwrite()", err);
		return err;
	}

	xnvme_buf_free(fh, buf);
	xnvme_file_close(fh);
	return 0;
}

/**
 * Tests that xnvme_file_open handles the XNVME_FILE_OFLG_TRUNC flag correctly.
 *
 * Opens and writes a file of size 1024 and then opens the same file using
 * XNVME_FILE_OFLG_TRUNC and writes 64 bytes, before verifying that the file
 * contains only 64 bytes in the end.
 */
int
test_file_trunc(struct xnvmec *cli)
{
	const char *output_path = cli->args.data_output;
	size_t init_write_size = 1024, trunc_write_size = 123, tbytes = 0;
	struct xnvme_dev *fh;
	int err;

	err = file_write_ascii(output_path, init_write_size,
			       &(struct xnvme_opts){.create = 1, .wronly = 1, .truncate = 1});
	if (err) {
		xnvmec_perr("file_open_write_ascii()", err);
		return err;
	}

	err = file_write_ascii(output_path, trunc_write_size,
			       &(struct xnvme_opts){.create = 1, .wronly = 1, .truncate = 1});
	if (err) {
		xnvmec_perr("file_open_write_ascii()", err);
		return err;
	}

	fh = xnvme_file_open(output_path, &(struct xnvme_opts){.rdonly = 1});
	if (fh == NULL) {
		xnvmec_perr("xnvme_file_open()", err);
		return err;
	}

	tbytes = xnvme_dev_get_geo(fh)->tbytes;

	xnvme_dev_close(fh);

	if (trunc_write_size == tbytes) {
		return 0;
	}

	XNVME_DEBUG("FAILED: trunc_write_size: %zu, tbytes: %zu", trunc_write_size, tbytes);
	return -EIO;
}

static struct xnvmec_sub g_subs[] = {
	{
		"write-fsync",
		"Write a file and call fsync between writes",
		"Write a file and call fsync between writes",
		test_file_fsync,
		{
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_POSA},
		},
	},
	{
		"file-trunc",
		"Write a file and then overwrite it using trunc",
		"Write a file and then overwrite it using trunc",
		test_file_trunc,
		{
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_POSA},
		},
	},
};

static struct xnvmec g_cli = {
	.title = "xNVMe file - Exercise the xnvme_file API",
	.descr_short = "Exercise the xnvme_file API",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_NONE);
}
