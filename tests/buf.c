// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
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

static void
pr_props(const char *name, const struct xnvme_buf_props *props)
{
	xnvme_cli_pinf("%s: {cpu_access: %u, numa_node: %d, page_nbytes: %" PRIu64 "}", name,
		       props->cpu_access, props->numa_node, props->page_nbytes);
}

#ifdef __linux__
static int
props_of_mappings(void)
{
	long pagesize = sysconf(_SC_PAGESIZE);
	size_t nbytes = 4 * (size_t)pagesize;
	struct xnvme_buf_props props;
	unsigned char present[4] = {0};
	// A container's default seccomp profile denies move_pages(); the node is then unknown
	bool numa = !access("/sys/devices/system/node/node0", F_OK) &&
		    !syscall(SYS_move_pages, 0, 0, NULL, NULL, NULL, 0);
	char *map;
	int err;

	map = mmap(NULL, nbytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (map == MAP_FAILED) {
		xnvme_cli_perr("mmap()", -errno);
		return -errno;
	}

	err = xnvme_buf_get_props(map, nbytes, &props);
	pr_props("untouched", &props);
	if (err || (props.cpu_access != XNVME_BUF_PROP_YES) ||
	    (props.page_nbytes < (uint64_t)pagesize) || (props.numa_node != -1)) {
		xnvme_cli_pinf("FAILED: untouched pages are accessible and on no NUMA node");
		err = err ? err : -EINVAL;
		goto exit;
	}
	if (mincore(map, nbytes, present) || present[0] || present[1] || present[2] ||
	    present[3]) {
		xnvme_cli_pinf("FAILED: the inspection made pages present");
		err = -EINVAL;
		goto exit;
	}

	memset(map, 0xAB, nbytes);
	err = xnvme_buf_get_props(map, nbytes, &props);
	pr_props("touched", &props);
	if (err || (props.cpu_access != XNVME_BUF_PROP_YES) || (numa && props.numa_node < 0)) {
		xnvme_cli_pinf("FAILED: touched pages are accessible and on a NUMA node");
		err = err ? err : -EINVAL;
		goto exit;
	}

	// MADV_DONTFORK splits the mapping in three without changing its access
	if (madvise(map + pagesize, pagesize, MADV_DONTFORK)) {
		err = -errno;
		xnvme_cli_perr("madvise()", err);
		goto exit;
	}
	err = xnvme_buf_get_props(map, nbytes, &props);
	pr_props("three mappings", &props);
	if (err || (props.cpu_access != XNVME_BUF_PROP_YES)) {
		xnvme_cli_pinf("FAILED: a range over three accessible mappings is accessible");
		err = err ? err : -EINVAL;
		goto exit;
	}
	if (mprotect(map + pagesize, pagesize, PROT_READ)) {
		err = -errno;
		xnvme_cli_perr("mprotect()", err);
		goto exit;
	}
	err = xnvme_buf_get_props(map, nbytes, &props);
	pr_props("a read-only page in the middle", &props);
	if (err || (props.cpu_access != XNVME_BUF_PROP_NO)) {
		xnvme_cli_pinf("FAILED: a range with a read-only page is not accessible");
		err = err ? err : -EINVAL;
		goto exit;
	}
	if (mprotect(map + pagesize, pagesize, PROT_READ | PROT_WRITE)) {
		err = -errno;
		xnvme_cli_perr("mprotect()", err);
		goto exit;
	}

	if (mprotect(map + 3 * pagesize, pagesize, PROT_NONE)) {
		err = -errno;
		xnvme_cli_perr("mprotect()", err);
		goto exit;
	}
	err = xnvme_buf_get_props(map, nbytes, &props);
	pr_props("partly PROT_NONE", &props);
	if (err || (props.cpu_access != XNVME_BUF_PROP_NO) || (props.numa_node != -1)) {
		xnvme_cli_pinf("FAILED: a range with a PROT_NONE page is not accessible");
		err = err ? err : -EINVAL;
		goto exit;
	}
	err = xnvme_buf_get_props(map, 3 * (size_t)pagesize, &props);
	pr_props("accessible part", &props);
	if (err || (props.cpu_access != XNVME_BUF_PROP_YES)) {
		xnvme_cli_pinf("FAILED: the pages before the PROT_NONE one are accessible");
		err = err ? err : -EINVAL;
		goto exit;
	}

exit:
	munmap(map, nbytes);

	return err;
}

static int
props_of_hugetlb(void)
{
	long pagesize = sysconf(_SC_PAGESIZE);
	size_t nbytes = 2 * 1024 * 1024;
	struct xnvme_buf_props props;
	void *map;
	int err;

	map = mmap(NULL, nbytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB,
		   -1, 0);
	if (map == MAP_FAILED) {
		xnvme_cli_pinf("SKIP: no hugetlb pages reserved; errno: %d", errno);
		return 0;
	}

	err = xnvme_buf_get_props(map, nbytes, &props);
	pr_props("hugetlb", &props);
	munmap(map, nbytes);
	if (err || (props.cpu_access != XNVME_BUF_PROP_YES) ||
	    (props.page_nbytes <= (uint64_t)pagesize)) {
		xnvme_cli_pinf("FAILED: hugetlb pages are accessible and larger than base pages");
		return err ? err : -EINVAL;
	}

	return 0;
}

static int
props_of_unmapped(void)
{
	long pagesize = sysconf(_SC_PAGESIZE);
	struct xnvme_buf_props props;
	void *map;
	int err;

	map = mmap(NULL, (size_t)pagesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,
		   0);
	if (map == MAP_FAILED) {
		xnvme_cli_perr("mmap()", -errno);
		return -errno;
	}
	munmap(map, (size_t)pagesize);

	err = xnvme_buf_get_props(map, (size_t)pagesize, &props);
	pr_props("unmapped", &props);
	if (err || (props.cpu_access != XNVME_BUF_PROP_NO) || props.page_nbytes ||
	    (props.numa_node != -1)) {
		xnvme_cli_pinf("FAILED: an unmapped range is not accessible and has no page size");
		return err ? err : -EINVAL;
	}

	return 0;
}
#endif

static int
test_buf_props(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_buf_props props = {0};
	size_t buf_nbytes = 4096;
	void *buf;
	int err;

	if ((xnvme_buf_get_props(&props, sizeof(props), NULL) != -EINVAL) ||
	    (xnvme_buf_get_props(&props, 0, &props) != -EINVAL) ||
	    (xnvme_buf_get_props((void *)UINTPTR_MAX, 2, &props) != -EINVAL)) {
		xnvme_cli_pinf(
			"FAILED: no -EINVAL for a NULL props, an empty range, or one running "
			"past the address space");
		return -EINVAL;
	}

#ifdef __linux__
	err = props_of_mappings();
	err = err ? err : props_of_unmapped();
	err = err ? err : props_of_hugetlb();
	if (err) {
		return err;
	}
#endif

	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		xnvme_cli_perr("xnvme_buf_alloc()", -errno);
		return -errno;
	}
	err = xnvme_buf_get_props(buf, buf_nbytes, &props);
	pr_props("xnvme_buf_alloc()", &props);
	xnvme_buf_free(dev, buf);
	if (err) {
		xnvme_cli_perr("xnvme_buf_get_props()", err);
		return err;
	}
#ifdef __linux__
	if (props.cpu_access == XNVME_BUF_PROP_UNKNOWN) {
		xnvme_cli_pinf("FAILED: Linux tells whether the CPU accesses memory");
		return -EINVAL;
	}
#else
	if ((props.cpu_access != XNVME_BUF_PROP_UNKNOWN) || (props.numa_node != -1) ||
	    props.page_nbytes) {
		xnvme_cli_pinf("FAILED: only Linux is inspected");
		return -EINVAL;
	}
#endif

	xnvme_cli_pinf("LGTM: xnvme_buf_get_props");

	return 0;
}

static int
test_buf_helpers(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	size_t nbytes = 4096 * 4, diff = 0;
	char *buf, *expected = NULL, *copy = NULL;
	int err;

	buf = xnvme_buf_alloc(dev, nbytes);
	if (!buf) {
		xnvme_cli_perr("xnvme_buf_alloc()", -errno);
		return -errno;
	}
	expected = xnvme_buf_virt_alloc(4096, nbytes);
	copy = xnvme_buf_virt_alloc(4096, nbytes);
	if (!expected || !copy) {
		err = -ENOMEM;
		goto exit;
	}

	err = xnvme_buf_fill(expected, nbytes, "anum");
	err = err ? err : xnvme_buf_fill(buf, nbytes, "anum");
	diff = 0; // xnvme_buf_diff() adds to it
	err = err ? err : xnvme_buf_diff(expected, buf, nbytes, &diff);
	if (err || diff) {
		xnvme_cli_pinf("FAILED: xnvme_buf_fill(); err: %d, diff: %zu", err, diff);
		err = err ? err : -EIO;
		goto exit;
	}

	err = xnvme_buf_memcpy(copy, buf, nbytes);
	if (err || memcmp(copy, expected, nbytes)) {
		xnvme_cli_pinf("FAILED: xnvme_buf_memcpy() into host memory; err: %d", err);
		err = err ? err : -EIO;
		goto exit;
	}

	// The "anum" pattern has no zero byte, so a cleared buffer differs in every byte
	err = xnvme_buf_clear(buf, nbytes);
	diff = 0; // xnvme_buf_diff() adds to it
	err = err ? err : xnvme_buf_diff(expected, buf, nbytes, &diff);
	if (err || (diff != nbytes)) {
		xnvme_cli_pinf("FAILED: xnvme_buf_clear(); err: %d, diff: %zu", err, diff);
		err = err ? err : -EIO;
		goto exit;
	}

	err = xnvme_buf_memcpy(buf, expected, nbytes);
	diff = 0; // xnvme_buf_diff() adds to it
	err = err ? err : xnvme_buf_diff(expected, buf, nbytes, &diff);
	if (err || diff) {
		xnvme_cli_pinf("FAILED: xnvme_buf_memcpy() from host memory; err: %d", err);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_cli_pinf("LGTM: the buffer helpers on memory from xnvme_buf_alloc()");

exit:
	xnvme_buf_virt_free(copy);
	xnvme_buf_virt_free(expected);
	xnvme_buf_free(dev, buf);

	return err;
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
			{XNVME_CLI_OPT_DEVICE_HEAP_SIZE, XNVME_CLI_LOPT},
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
		"buf_props",
		"Inspect the properties of memory with xnvme_buf_get_props()",
		"Check what xnvme_buf_get_props() reports for a buffer from xnvme_buf_alloc() "
		"and, on Linux, for untouched, touched, read-only, PROT_NONE, hugetlb and "
		"unmapped ranges",
		test_buf_props,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"buf_helpers",
		"Fill, copy, compare and clear memory from xnvme_buf_alloc()",
		"Fill, copy, compare and clear memory from xnvme_buf_alloc() with the buffer "
		"helpers, which call the CUDA or HIP runtime for memory of the heap of a GPU "
		"backend and use the CPU for other memory",
		test_buf_helpers,
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
