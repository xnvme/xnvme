// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <string.h>
#include <libxnvme.h>
#include <xnvme_dev.h>
#include <liburing/xnvme_be_liburing.h>

#define RING_FLAGS_BIG (IORING_SETUP_SQE128 | IORING_SETUP_CQE32)

/**
 * io_uring_cmd alone sets up 128-byte SQEs and 32-byte CQEs, whatever the queue options
 *
 * The flags are not visible through the API, so this reads them from the io_uring of the
 * queue. XNVME_QUEUE_IOPOLL and XNVME_QUEUE_SQPOLL are left out: the kernel refuses them
 * depending on the device and on privileges.
 */
static int
test_ring_flags(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const char *id = dev->be.async.id;
	const int opts[] = {
		0x0,
		XNVME_QUEUE_P2P_CQ_MIRROR,
		XNVME_QUEUE_SQ_HOSTMEM,
		XNVME_QUEUE_P2P_CQ_MIRROR | XNVME_QUEUE_SQ_HOSTMEM,
	};
	unsigned int expected;
	int nerr = 0;

	if (!strcmp(id, "io_uring_cmd")) {
		expected = RING_FLAGS_BIG;
	} else if (!strcmp(id, "io_uring")) {
		expected = 0;
	} else {
		xnvme_cli_pinf("FAILED: async interface '%s' has no io_uring", id);
		return -EINVAL;
	}

	for (size_t i = 0; i < sizeof opts / sizeof(*opts); ++i) {
		struct xnvme_queue *queue = NULL;
		unsigned int flags;
		int err;

		err = xnvme_queue_init(dev, 8, opts[i], &queue);
		if (err) {
			xnvme_cli_perr("xnvme_queue_init()", err);
			return err;
		}

		flags = ((struct xnvme_queue_liburing *)queue)->ring.flags & RING_FLAGS_BIG;
		if (flags != expected) {
			xnvme_cli_pinf("FAILED: opts(0x%x): flags(0x%x) != expected(0x%x)",
				       opts[i], flags, expected);
			nerr += 1;
		}

		xnvme_queue_term(queue);
	}
	if (nerr) {
		return -EINVAL;
	}

	xnvme_cli_pinf("LGTM: '%s' flags(0x%x)", id, expected);

	return 0;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"ring-flags",
		"Verify the SQE and CQE size of the io_uring a queue sets up",
		"Verify the SQE and CQE size of the io_uring a queue sets up",
		test_ring_flags,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			XNVME_CLI_ASYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Test the io_uring of the liburing backend",
	.descr_short = "Test the io_uring of the liburing backend",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
