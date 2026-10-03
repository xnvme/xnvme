// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <pthread.h>
#include <libxnvme.h>

#define THREADS 16
#define SLOTS 8

#ifndef XNVME_RAND_R_ENABLED
/**
 * Thread-safe stand-in for rand_r() where libc has none, e.g. Windows; the
 * glibc LCG, returning bits [30:16] of the state
 */
static int
rand_r(unsigned int *seed)
{
	*seed = *seed * 1103515245 + 12345;
	return (int)((*seed >> 16) & 0x7fff);
}
#endif

struct hammer {
	struct xnvme_dev *dev;
	uint64_t rounds;
	unsigned seed;
	int nerr;
};

/**
 * Allocate and free buffers of varying sizes in a random order, one thread of
 * many doing the same on the same device, so the backend's allocator sees
 * concurrent calls. A backend that shares one heap across threads without a
 * lock corrupts its free list here, which glibc reports as a double free or a
 * corrupted chunk.
 */
static void *
hammer_fn(void *arg)
{
	struct hammer *h = arg;
	void *held[SLOTS] = {NULL};

	for (uint64_t round = 0; round < h->rounds; ++round) {
		int slot = rand_r(&h->seed) % SLOTS;

		if (held[slot]) {
			xnvme_buf_free(h->dev, held[slot]);
			held[slot] = NULL;
			continue;
		}
		held[slot] = xnvme_buf_alloc(h->dev, 4096UL << (rand_r(&h->seed) % 6));
		if (!held[slot]) {
			h->nerr += 1;
		}
	}
	for (int slot = 0; slot < SLOTS; ++slot) {
		if (held[slot]) {
			xnvme_buf_free(h->dev, held[slot]);
		}
	}
	return NULL;
}

static int
test_buf_alloc_free_threads(struct xnvme_cli *cli)
{
	struct hammer hammers[THREADS];
	pthread_t tids[THREADS];
	int nerr = 0;

	xnvme_cli_pinf("threads: %d, rounds per thread: %zu", THREADS, cli->args.count);

	for (int i = 0; i < THREADS; ++i) {
		hammers[i].dev = cli->args.dev;
		hammers[i].rounds = cli->args.count;
		hammers[i].seed = 1000 + i;
		hammers[i].nerr = 0;
		if (pthread_create(&tids[i], NULL, hammer_fn, &hammers[i])) {
			xnvme_cli_perr("pthread_create()", -errno);
			return -errno;
		}
	}
	for (int i = 0; i < THREADS; ++i) {
		pthread_join(tids[i], NULL);
		nerr += hammers[i].nerr;
	}

	if (nerr) {
		xnvme_cli_pinf("nerr: %d allocations refused", nerr);
		return -ENOMEM;
	}
	xnvme_cli_pinf("LGMT: xnvme_buf_{alloc,free} from %d threads", THREADS);
	return 0;
}

static int
test_buf_alloc_free(struct xnvme_cli *cli)
{
	uint64_t count = cli->args.count;
	int nerr = 0;

	xnvme_cli_pinf("count: %zu", count);

	for (uint64_t i = 0; i < count; ++i) {
		size_t buf_nbytes = 1 << i;
		void *buf;

		printf("\n");
		xnvme_cli_pinf("[alloc/free] i: %zu, buf_nbytes: %zu", i + 1, buf_nbytes);

		buf = xnvme_buf_alloc(cli->args.dev, buf_nbytes);
		if (!buf) {
			xnvme_cli_perr("xnvme_buf_alloc()", -errno);
			nerr += 1;
			continue;
		}
		xnvme_buf_free(cli->args.dev, buf);

		xnvme_cli_pinf("buf: %p", buf);
	}

	printf("\n");
	if (nerr) {
		xnvme_cli_pinf("--={[ Got Errors - see details above ]}=--");
		xnvme_cli_pinf("nerr: %d out of count: %zu", nerr, count);
	} else {
		xnvme_cli_pinf("LGMT: xnvme_buf_{alloc,free}");
	}
	printf("\n");

	return nerr ? -ENOMEM : 0;
}

static int
test_buf_vtophys(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	size_t buf_nbytes = 4096;
	uint64_t phys = 0;
	void *buf;
	int err;

	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		xnvme_cli_perr("xnvme_buf_alloc()", -errno);
		return -errno;
	}

	err = xnvme_buf_vtophys(dev, buf, &phys);
	if (err == -ENOSYS) {
		xnvme_cli_pinf("SKIP: buf_vtophys not supported by this mem backend");
		xnvme_buf_free(dev, buf);
		return 0;
	}
	if (err) {
		xnvme_cli_perr("xnvme_buf_vtophys()", err);
		xnvme_buf_free(dev, buf);
		return err;
	}

	xnvme_cli_pinf("buf: %p, phys: 0x%" PRIx64, buf, phys);

	if (!phys) {
		xnvme_cli_pinf("FAILED: phys == 0");
		xnvme_buf_free(dev, buf);
		return -EINVAL;
	}

	xnvme_buf_free(dev, buf);

	xnvme_cli_pinf("LGMT: xnvme_buf_vtophys");

	return 0;
}

static int
test_virt_buf_alloc_free(struct xnvme_cli *cli)
{
	uint64_t count = cli->args.count;
	int nerr = 0;

	xnvme_cli_pinf("count: %zu", count);

	for (uint64_t i = 0; i < count; ++i) {
		size_t buf_nbytes = 1 << i;
		void *buf;

		printf("\n");
		xnvme_cli_pinf("[alloc/free] i: %zu, buf_nbytes: %zu", i + 1, buf_nbytes);

		buf = xnvme_buf_virt_alloc(0x1000, buf_nbytes);
		if (!buf) {
			xnvme_cli_perr("xnvme_buf_alloc()", -errno);
			nerr += 1;
			continue;
		}
		xnvme_cli_pinf("buf: %p", buf);

		xnvme_buf_virt_free(buf);
	}

	printf("\n");
	if (nerr) {
		xnvme_cli_pinf("--={[ Got Errors - see details above ]}=--");
		xnvme_cli_pinf("nerr: %d out of count: %zu", nerr, count);
	} else {
		xnvme_cli_pinf("LGMT: xnvme_buf_virt_{alloc,free}");
	}
	printf("\n");

	return nerr ? -ENOMEM : 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"buf_alloc_free",
		"Allocate and free a buffer 'count' times of size [1, 2^count]",
		"Allocate and free a buffer 'count' times of size [1, 2^count]",
		test_buf_alloc_free,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LREQ},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"buf_alloc_free_threads",
		"Allocate and free buffers from 16 threads at once, 'count' rounds each",
		"Allocate and free buffers from 16 threads at once, 'count' rounds each",
		test_buf_alloc_free_threads,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LREQ},
			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"buf_vtophys",
		"Allocate a buffer and resolve its physical address",
		"Allocate a buffer and resolve its physical address",
		test_buf_vtophys,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"buf_virt_alloc_free",
		"Allocate and free a buffer 'count' times of size [1, 2^count]",
		"Allocate and free a buffer 'count' times of size [1, 2^count]",
		test_virt_buf_alloc_free,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LREQ},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Test xNVMe basic buffer alloc/free",
	.descr_short = "Test xNVMe basic buffer alloc/free",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
