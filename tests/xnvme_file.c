#include <errno.h>
#include <libxnvme_file.h>
#include <libxnvmec.h>

/**
 * Tests that xnvme_file_open handles the XNVME_FILE_OFLG_TRUNC flag correctly.
 *
 * Opens and writes a file of size 1024 and then opens the same file using
 * XNVME_FILE_OFLG_TRUNC and writes 64 bytes, before verifying that the file
 * contains only 64 bytes in the end.
 */
int
test_file_fsync(struct xnvmec *cli)
{
	const char *output_path = cli->args.data_output;
	struct xnvme_dev *fh = NULL;
	struct xnvme_cmd_ctx ctx;
	char *buf = NULL;
	size_t bytes_per_write = 1024;
	size_t num_writes = 8;
	int err;

	fh = xnvme_file_open(output_path,
			     XNVME_FILE_OFLG_CREATE | XNVME_FILE_OFLG_WRONLY);
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

	for (size_t i = 0; i < bytes_per_write; i++) {
		buf[i] = 33 + (i % (125 - 33));
	}

	ctx = xnvme_file_get_cmd_ctx(fh);

	for (size_t i = 0; i < num_writes; i++) {
		xnvmec_pinf("Writing %lu bytes", bytes_per_write);
		err = xnvme_file_pwrite(&ctx, buf, bytes_per_write, 0);
		if (err) {
			xnvmec_perr("xnvme_file_pwrite()", err);
			return err;
		}

		xnvmec_pinf("fsync'ing");
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
int
file_write_ascii(const char *path, size_t size, int flags)
{
	struct xnvme_dev *fh = NULL;
	struct xnvme_cmd_ctx ctx;
	char *buf = NULL;
	int err = 0;

	fh = xnvme_file_open(path, flags);
	buf = xnvme_buf_alloc(fh, size);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", size);
		for (size_t i = 0; i < size; i++) {
			err = xnvme_file_pwrite(&ctx, buf, size, 0);
			if (err) {
				xnvmec_perr("xnvme_file_pwrite()", err);
				return err;
			}
		}
	}
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
	size_t init_write_size = 1024, trunc_write_size = 64;
	const char *output_path = cli->args.data_output;
	int err;

	err = file_write_ascii(output_path, init_write_size,
			       XNVME_FILE_OFLG_CREATE | XNVME_FILE_OFLG_WRONLY);
	if (err) {
		xnvmec_perr("file_open_write_ascii()", err);
		return err;
	}
	xnvmec_pinf("Wrote %d bytes to %s", init_write_size, output_path);

	err = file_write_ascii(output_path, trunc_write_size,
			       XNVME_FILE_OFLG_CREATE | XNVME_FILE_OFLG_WRONLY | XNVME_FILE_OFLG_TRUNC);
	if (err) {
		xnvmec_perr("file_open_write_ascii()", err);
		return err;
	}
	xnvmec_pinf("Truncated and wrote %d bytes to %s", trunc_write_size,
		    output_path);

	// Verify contents of file is size trunc_write_size
	struct xnvme_dev *fh = NULL;
	fh = xnvme_file_open(output_path, XNVME_FILE_OFLG_RDONLY);
	if (fh == NULL) {
		xnvmec_perr("xnvme_file_open()", err);
		return err;
	}

	// TODO: xnvme_dev_get_geo(fh)->tbytes returns 512 * floor(file_size / 512)
	// meaning that trunc_write_size must be a multiple of 512 in order for the
	// following assertion to work as expected.
	uint64_t tbytes = xnvme_dev_get_geo(fh)->tbytes ;
	xnvmec_pinf("tbytes: %lu == trunc_write_size: %lu", tbytes, trunc_write_size);
	assert(tbytes == trunc_write_size);
	xnvme_dev_close(fh);

	return 0;
}

static struct xnvmec_sub g_subs[] = {
	{
		"write-fsync", "Write a file and call fsync between writes",
		"Write a file and call fsync between writes", test_file_fsync, {
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_POSA},
		},
		"file-trunc", "Write a file and then overwrite it using trunc",
		"Write a file and then overwrite it using trunc", test_file_trunc, {
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
