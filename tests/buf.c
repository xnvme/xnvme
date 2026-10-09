// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <libxnvme.h>

#define THREADS 16
#define SLOTS 8

#ifndef XNVME_RAND_R_ENABLED
static int
rand_r(unsigned int *seed)
{
	*seed = *seed * 1103515245 + 12345;
	return (int)((*seed >> 16) & 0x7fff);
}
#endif

#define MAX_NBYTES (4096UL << 5)

struct hammer {
	struct xnvme_dev *dev;
	uint64_t rounds;
	unsigned seed;
	int id;
	int nerr;
	int ncorrupt;
	uint64_t *stamp;
	uint64_t *check;
};

static void
stamp_fill(uint64_t *stamp, size_t nbytes, uint64_t tag)
{
	for (size_t i = 0; i < nbytes / sizeof(*stamp); ++i) {
		stamp[i] = tag ^ i;
	}
}

/*
 * Each buffer gets a pattern unique to this thread, slot and allocation, written and read back
 * through xnvme_buf_memcpy() so device-memory buffers work too. Two threads handed overlapping
 * blocks then fail the check instead of passing unnoticed.
 */
static void *
hammer_fn(void *arg)
{
	struct hammer *h = arg;
	void *held[SLOTS] = {NULL};
	size_t nbytes[SLOTS] = {0};
	uint64_t tag[SLOTS] = {0};

	for (uint64_t round = 0; round < h->rounds; ++round) {
		int slot = rand_r(&h->seed) % SLOTS;

		if (held[slot]) {
			stamp_fill(h->stamp, nbytes[slot], tag[slot]);
			if (xnvme_buf_memcpy(h->check, held[slot], nbytes[slot]) ||
			    memcmp(h->check, h->stamp, nbytes[slot])) {
				h->ncorrupt += 1;
			}
			xnvme_buf_free(h->dev, held[slot]);
			held[slot] = NULL;
			continue;
		}
		nbytes[slot] = 4096UL << (rand_r(&h->seed) % 6);
		held[slot] = xnvme_buf_alloc(h->dev, nbytes[slot]);
		if (!held[slot]) {
			h->nerr += 1;
			continue;
		}
		tag[slot] = ((uint64_t)h->id << 56) | ((uint64_t)slot << 48) | round;
		stamp_fill(h->stamp, nbytes[slot], tag[slot]);
		if (xnvme_buf_memcpy(held[slot], h->stamp, nbytes[slot])) {
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
	int ncorrupt = 0;
	int nerr = 0;
	int err;

	xnvme_cli_pinf("threads: %d, rounds per thread: %zu", THREADS, cli->args.count);

	for (int i = 0; i < THREADS; ++i) {
		hammers[i].dev = cli->args.dev;
		hammers[i].rounds = cli->args.count;
		hammers[i].seed = 1000 + i;
		hammers[i].id = i;
		hammers[i].nerr = 0;
		hammers[i].ncorrupt = 0;
		hammers[i].stamp = malloc(MAX_NBYTES);
		hammers[i].check = malloc(MAX_NBYTES);
		if (!hammers[i].stamp || !hammers[i].check) {
			xnvme_cli_perr("malloc()", -ENOMEM);
			err = ENOMEM;
		} else {
			err = pthread_create(&tids[i], NULL, hammer_fn, &hammers[i]);
			if (err) {
				xnvme_cli_perr("pthread_create()", -err);
			}
		}
		if (err) {
			for (int j = 0; j < i; ++j) {
				pthread_join(tids[j], NULL);
			}
			for (int j = 0; j <= i; ++j) {
				free(hammers[j].stamp);
				free(hammers[j].check);
			}
			return -err;
		}
	}
	for (int i = 0; i < THREADS; ++i) {
		pthread_join(tids[i], NULL);
		nerr += hammers[i].nerr;
		ncorrupt += hammers[i].ncorrupt;
		free(hammers[i].stamp);
		free(hammers[i].check);
	}

	if (ncorrupt) {
		xnvme_cli_pinf("ncorrupt: %d buffers changed while held by one thread", ncorrupt);
		return -EIO;
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
