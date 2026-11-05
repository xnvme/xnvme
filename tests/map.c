// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libxnvme.h>

static int
test_mem_map_unmap(struct xnvme_cli *cli)
{
	uint64_t count = cli->args.count;
	size_t align;
	int nerr = 0, err;

	xnvme_cli_pinf("count: %zu", count);

	err = xnvme_buf_get_alignment(cli->args.dev, &align);
	if (err) {
		xnvme_cli_perr("xnvme_buf_get_alignment()", err);
		return err;
	}
	xnvme_cli_pinf("alignment: %zu", align);

	for (uint64_t i = 0; i < count; ++i) {
		size_t buf_nbytes = 1ULL << i;
		void *buf;

		// The memory to register starts and ends on the alignment
		buf_nbytes = (buf_nbytes + align - 1) & ~(align - 1);

		printf("\n");
		xnvme_cli_pinf("[register/unregister] i: %zu, buf_nbytes: %zu", i + 1, buf_nbytes);

		buf = xnvme_buf_virt_alloc(align, buf_nbytes);
		if (!buf) {
			xnvme_cli_perr("xnvme_buf_virt_alloc()", -errno);
			nerr += 1;
			continue;
		}

		err = xnvme_buf_register(cli->args.dev, buf, buf_nbytes);
		if (err) {
			xnvme_cli_perr("xnvme_buf_register()", err);
			nerr += 1;
			xnvme_buf_virt_free(buf);
			continue;
		}

		err = xnvme_buf_unregister(cli->args.dev, buf);
		if (err) {
			xnvme_cli_perr("xnvme_buf_unregister()", err);
			nerr += 1;
		}

		xnvme_buf_virt_free(buf);
	}

	if (nerr) {
		xnvme_cli_pinf("--={[ Got Errors - see details above ]}=--");
		xnvme_cli_pinf("nerr: %d out of count: %zu", nerr, count);
	} else {
		xnvme_cli_pinf("LGTM: xnvme_buf_{register,unregister}");
	}
	printf("\n");

	return nerr ? -ENOMEM : 0;
}

/**
 * Read the first LBA into the given buffer, to tell that it is still usable as payload
 */
static int
read_into(struct xnvme_dev *dev, void *buf)
{
	struct xnvme_cmd_ctx ctx = {0};
	int err;

	xnvme_dev_get_cmd_ctx(dev, &ctx);

	err = xnvme_nvm_read(&ctx, xnvme_dev_get_nsid(dev), 0x0, 0, buf, NULL);
	if (!err && xnvme_cmd_ctx_cpl_status(&ctx)) {
		err = -EIO;
	}

	return err;
}

static int
register_unregister(struct xnvme_dev *dev, void *buf, size_t nbytes, const char *what)
{
	int err;

	err = xnvme_buf_register(dev, buf, nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_register()", err);
		xnvme_cli_pinf("FAILED: registering %s", what);
		return err;
	}

	err = xnvme_buf_unregister(dev, buf);
	if (err) {
		xnvme_cli_perr("xnvme_buf_unregister()", err);
		xnvme_cli_pinf("FAILED: unregistering %s", what);
	}

	return err;
}

/**
 * Registrations are counted: registering memory from xnvme_buf_alloc() leaves it be, and a range
 * within a registered one is counted on it, so unregistering either leaves the memory usable
 */
static int
test_register_counted(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	size_t align, nbytes;
	uint8_t *buf;
	int err;

	err = xnvme_buf_get_alignment(dev, &align);
	if (err) {
		xnvme_cli_perr("xnvme_buf_get_alignment()", err);
		return err;
	}
	nbytes = (geo->lba_nbytes + align - 1) & ~(align - 1);
	xnvme_cli_pinf("alignment: %zu, nbytes: %zu", align, nbytes);

	buf = xnvme_buf_alloc(dev, nbytes);
	if (!buf) {
		xnvme_cli_perr("xnvme_buf_alloc()", -errno);
		return -errno;
	}
	err = register_unregister(dev, buf, nbytes, "memory from xnvme_buf_alloc()");
	if (!err) {
		err = read_into(dev, buf);
		if (err) {
			xnvme_cli_pinf(
				"FAILED: reading into memory from xnvme_buf_alloc(), err: %d",
				err);
		}
	}
	xnvme_buf_free(dev, buf);
	if (err) {
		return err;
	}

	buf = xnvme_buf_virt_alloc(align, 2 * nbytes);
	if (!buf) {
		xnvme_cli_perr("xnvme_buf_virt_alloc()", -errno);
		return -errno;
	}
	err = xnvme_buf_register(dev, buf, 2 * nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_register()", err);
		goto exit;
	}
	err = register_unregister(dev, buf + nbytes, nbytes, "a range within a registered one");
	if (!err) {
		err = read_into(dev, buf);
		if (err) {
			xnvme_cli_pinf("FAILED: reading into the registered range, err: %d", err);
		}
	}
	if (xnvme_buf_unregister(dev, buf)) {
		xnvme_cli_pinf("FAILED: unregistering the registered range");
		err = err ? err : -EIO;
	}

exit:
	xnvme_buf_virt_free(buf);

	if (!err) {
		xnvme_cli_pinf("LGTM: registrations are counted");
	}

	return err;
}

/**
 * Write one LBA carrying the given metadata buffer
 *
 * @return 0 where the controller accepted it, negative errno where the write
 * was refused, whether by the backend or by the controller.
 */
static int
mptr_write(struct xnvme_dev *dev, void *mbuf)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	struct xnvme_cmd_ctx ctx = {0};
	void *dbuf;
	int err;

	xnvme_dev_get_cmd_ctx(dev, &ctx);

	dbuf = xnvme_buf_alloc(dev, geo->lba_nbytes);
	if (!dbuf) {
		xnvme_cli_perr("xnvme_buf_alloc()", -errno);
		return -errno;
	}
	memset(dbuf, 0, geo->lba_nbytes);

	err = xnvme_nvm_write(&ctx, xnvme_dev_get_nsid(dev), 0x0, 0, dbuf, mbuf);
	if (!err && xnvme_cmd_ctx_cpl_status(&ctx)) {
		err = -EIO;
	}

	xnvme_buf_free(dev, dbuf);

	return err;
}

/**
 * A metadata buffer the runtime was never told about must be refused
 *
 * Memory outside the registry translates to zero, so submitting anyway points
 * the controller at physical address zero, which without an IOMMU is host
 * memory it may write.
 *
 * Writes rather than reads on purpose: without the check a write has the
 * controller read address zero where a read has it write there.
 *
 * The registered buffer goes first as a control. A refusal on its own says
 * nothing, since a namespace carrying no metadata refuses such a write whatever
 * buffer it is given, on any backend.
 */
static int
test_mptr_registration(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	size_t mbuf_nbytes = geo->nbytes_oob ? geo->nbytes_oob : 4096;
	void *mbuf;
	int err;

	mbuf = xnvme_buf_alloc(dev, mbuf_nbytes);
	if (!mbuf) {
		xnvme_cli_perr("xnvme_buf_alloc(mbuf)", -errno);
		return -errno;
	}

	err = mptr_write(dev, mbuf);
	xnvme_buf_free(dev, mbuf);

	if (err) {
		xnvme_cli_pinf("INCONCLUSIVE: refused with a registered mbuf too, err(%d)", err);
		xnvme_cli_pinf("the namespace reports nbytes_oob(%u); with none, a write "
			       "carrying an mptr is refused whatever the buffer is",
			       geo->nbytes_oob);
		return -ENOTSUP;
	}

	/* Deliberately not xnvme_buf_alloc(): the whole point is memory the
	 * runtime has no record of */
	mbuf = malloc(mbuf_nbytes);
	if (!mbuf) {
		xnvme_cli_perr("malloc()", -errno);
		return -errno;
	}

	err = mptr_write(dev, mbuf);
	free(mbuf);

	if (!err) {
		xnvme_cli_pinf("FAILED: the command was accepted with an unregistered mbuf");
		return -EIO;
	}

	xnvme_cli_pinf("LGTM: accepted with a registered mbuf, refused with err(%d)", err);

	return 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"mem_map_unmap",
		"Register and unregister a buffer 'count' times of size [1, 2^count]",
		"Register and unregister a buffer 'count' times of size [1, 2^count]",
		test_mem_map_unmap,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LREQ},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"register_counted",
		"Register memory from xnvme_buf_alloc() and a range within a registered one",
		"Register memory from xnvme_buf_alloc() and a range within a registered one",
		test_register_counted,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"mptr_registration",
		"Refuse a metadata buffer the registry has no record of",
		"Refuse a metadata buffer the registry has no record of",
		test_mptr_registration,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ASYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Test xNVMe registering memory",
	.descr_short = "Test xNVMe registering memory",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
