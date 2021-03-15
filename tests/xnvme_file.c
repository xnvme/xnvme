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

static struct xnvmec_sub g_subs[] = {
	{
		"write-fsync", "Write a file and call fsync between writes",
		"Write a file and call fsync between writes", test_file_fsync, {
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_POSA},
		}
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
