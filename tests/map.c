// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libxnvme.h>
#include <sys/mman.h>
#include <unistd.h>

static int
test_mem_map_unmap(struct xnvme_cli *cli)
{
	uint64_t count = cli->args.count;
	int nerr = 0;

	xnvme_cli_pinf("count: %zu", count);

	for (uint64_t i = 0; i < count; ++i) {
		size_t buf_nbytes = 1 << i;
		void *buf;
		int err;
		long page_size;

		printf("\n");
		xnvme_cli_pinf("[map/unmap] i: %zu, buf_nbytes: %zu", i + 1, buf_nbytes);

		page_size = sysconf(_SC_PAGESIZE);
		buf_nbytes = (buf_nbytes + page_size - 1) & ~(page_size - 1);

		buf = mmap(NULL, buf_nbytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
			   0, 0);
		if (buf == MAP_FAILED) {
			xnvme_cli_perr("mmap()", -errno);
			continue;
		}

		err = xnvme_mem_map(cli->args.dev, buf, buf_nbytes);
		if (err) {
			xnvme_cli_perr("xnvme_mem_map()", -errno);
			nerr += 1;
			munmap(buf, buf_nbytes);
			continue;
		}

		err = xnvme_mem_unmap(cli->args.dev, buf);
		if (err) {
			xnvme_cli_perr("xnvme_mem_unmap()", -errno);
			nerr += 1;
			munmap(buf, buf_nbytes);
			continue;
		}

		if (munmap(buf, buf_nbytes)) {
			xnvme_cli_perr("munmap()", -errno);
			nerr += 1;
			continue;
		}
	}

	if (nerr) {
		xnvme_cli_pinf("--={[ Got Errors - see details above ]}=--");
		xnvme_cli_pinf("nerr: %d out of count: %zu", nerr, count);
	} else {
		xnvme_cli_pinf("LGTM: xnvme_mem_{map,unmap}");
	}
	printf("\n");

	return nerr ? -ENOMEM : 0;
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
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	void *dbuf;
	int err;

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
		"Map and unmap a buffer 'count' times of size [1, 2^count]",
		"Map and unmap a buffer 'count' times of size [1, 2^count]",
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
	.title = "Test xNVMe basic memory map/unmap",
	.descr_short = "Test xNVMe basic memory map/unmap",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
