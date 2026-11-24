// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/mman.h>
#endif

#include <libxnvme.h>
#include <xnvme_util.h>
#include <xnvme_vcs.h>

#include "xnvmeperf_gpu.h"

static int
sub_htod(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = {0};
	uint32_t iosize = cli->args.iosize;
	uint32_t nslots = cli->args.qdepth ? cli->args.qdepth : 1;
	uint32_t seconds = cli->args.runtime ? cli->args.runtime : 5;
	double gbps = 0.0;
	int err;

	xnvme_opts_set_defaults(&opts);

	xnvme_cli_to_opts(cli, &opts);

	if (!iosize || !xnvme_is_pow2(iosize)) {
		err = -EINVAL;
		xnvme_cli_perr("Error: --iosize must be a power of 2", err);
		return err;
	}

	printf("xnvme perf-xfer htod: iosize %u, in-flight %u, runtime %u s, gpu %u\n", iosize,
	       nslots, seconds, opts.gpu_id);

	err = xnvmeperf_htod_roofline(opts.gpu_id, iosize, nslots, seconds, &gbps);
	if (err) {
		xnvme_cli_perr("Failed: xnvmeperf_htod_roofline()", err);
		return err;
	}

	printf("\nhost-to-device copy: %.2f GB/s\n", gbps);
	return 0;
}

#ifdef __linux__
#ifndef MAP_HUGE_SHIFT
#define MAP_HUGE_SHIFT 26
#endif

#define XFER_NSIZES_MAX 64

static double
usecs_since(const struct timespec *t0)
{
	struct timespec t1;

	clock_gettime(CLOCK_MONOTONIC, &t1);

	return (double)(t1.tv_sec - t0->tv_sec) * 1e6 + (double)(t1.tv_nsec - t0->tv_nsec) / 1e3;
}

static int
cmp_double(const void *a, const void *b)
{
	double x = *(const double *)a, y = *(const double *)b;

	return (x > y) - (x < y);
}

static void
pr_usecs(int indent, const char *name, double *usecs, uint32_t count)
{
	double median;

	qsort(usecs, count, sizeof(*usecs), cmp_double);
	median = (count % 2) ? usecs[count / 2] : (usecs[count / 2 - 1] + usecs[count / 2]) / 2;
	printf("%*s%s_usecs: {min: %.1f, median: %.1f, max: %.1f}\n", indent, "", name, usecs[0],
	       median, usecs[count - 1]);
}

/**
 * Print the times of a pair of calls, such as registering and unregistering: those of the first
 * pair, and the minimum, median and maximum of each call over the count pairs after it
 */
static void
pr_times(int indent, const char *names[2], const double first[2], double *usecs[2], uint32_t count)
{
	for (int i = 0; i < 2; ++i) {
		printf("%*sfirst_%s_usecs: %.1f\n", indent, "", names[i], first[i]);
	}
	for (int i = 0; i < 2; ++i) {
		pr_usecs(indent, names[i], usecs[i], count);
	}
}

static int
get_count(struct xnvme_cli *cli, uint32_t *count)
{
	uint64_t given = cli->given[XNVME_CLI_OPT_COUNT] ? cli->args.count : 100;

	if (!given || (given > UINT32_MAX)) {
		xnvme_cli_perr("Error: --count must be above 0 and at most UINT32_MAX", -EINVAL);
		return -EINVAL;
	}
	*count = (uint32_t)given;

	return 0;
}

/**
 * Parse the sizes of --mib into bytes; returns how many there are, or -EINVAL when the list is
 * empty or longer than nbytes_max, or a size is zero, does not fit in a size_t or is not a
 * decimal number
 */
static int
parse_mib(const char *arg, size_t *nbytes, int nbytes_max)
{
	const char *pos = arg;
	int n = 0;

	if (!arg) {
		return -EINVAL;
	}
	for (;;) {
		unsigned long long mib;
		char *end;

		if ((n == nbytes_max) || (*pos < '0') || (*pos > '9')) {
			return -EINVAL;
		}
		errno = 0;
		mib = strtoull(pos, &end, 10);
		if (errno || !mib || (mib > (SIZE_MAX >> 20))) {
			return -EINVAL;
		}
		nbytes[n++] = (size_t)mib << 20;
		if (!*end) {
			return n;
		}
		if (*end != ',') {
			return -EINVAL;
		}
		pos = end + 1;
	}
}

static int
get_sizes(struct xnvme_cli *cli, size_t *nbytes)
{
	int nsizes = parse_mib(cli->args.mib, nbytes, XFER_NSIZES_MAX);

	if (nsizes < 0) {
		xnvme_cli_perr(
			"Error: --mib takes up to 64 sizes in MiB, each above 0, separated by "
			"commas",
			nsizes);
	}

	return nsizes;
}

/**
 * Register memory with dev and unregister it, timing each call: the dma-buf fd named by vaddr,
 * or, when fd is negative, the nbytes of host memory at vaddr
 */
static int
register_once(struct xnvme_dev *dev, int fd, void *vaddr, size_t nbytes, double *reg_usecs,
	      double *unreg_usecs)
{
	struct timespec t0;
	int err;

	clock_gettime(CLOCK_MONOTONIC, &t0);
	err = (fd < 0) ? xnvme_buf_register(dev, vaddr, nbytes)
		       : xnvme_buf_register_dmabuf(dev, fd, vaddr);
	*reg_usecs = usecs_since(&t0);
	if (err) {
		xnvme_cli_perr((fd < 0) ? "xnvme_buf_register()" : "xnvme_buf_register_dmabuf()",
			       err);
		return err;
	}

	clock_gettime(CLOCK_MONOTONIC, &t0);
	err = xnvme_buf_unregister(dev, vaddr);
	*unreg_usecs = usecs_since(&t0);
	if (err) {
		xnvme_cli_perr("xnvme_buf_unregister()", err);
	}

	return err;
}

static int
register_dmabuf(struct xnvme_cli *cli, uint32_t count)
{
	static const char *names[2] = {"register", "unregister"};
	struct xnvme_opts opts = {0};
	struct xnvme_dev *dev = NULL;
	double *usecs[2] = {NULL, NULL};
	double first[2];
	void *vaddr = MAP_FAILED;
	off_t nbytes;
	int err = 0;

	nbytes = lseek(cli->args.dmabuf_fd, 0, SEEK_END);
	if (nbytes <= 0) {
		err = nbytes < 0 ? -errno : -EINVAL;
		xnvme_cli_perr("Error: --dmabuf-fd is no dma-buf this process holds", err);
		return err;
	}
	// addresses for the dma-buf to be named by, which nothing reads or writes
	vaddr = mmap(NULL, (size_t)nbytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
		     -1, 0);
	usecs[0] = calloc(count, sizeof(double));
	usecs[1] = calloc(count, sizeof(double));
	if ((vaddr == MAP_FAILED) || !usecs[0] || !usecs[1]) {
		err = -ENOMEM;
		xnvme_cli_perr("Error: no addresses for the dma-buf, or no memory", err);
		goto exit;
	}

	xnvme_opts_set_defaults(&opts);
	xnvme_cli_to_opts(cli, &opts);
	dev = xnvme_dev_open(cli->args.uri, &opts);
	if (!dev) {
		err = -errno;
		xnvme_cli_perr("xnvme_dev_open()", err);
		goto exit;
	}

	err = register_once(dev, cli->args.dmabuf_fd, vaddr, 0, &first[0], &first[1]);
	if (err) {
		goto exit;
	}
	for (uint32_t i = 0; i < count; ++i) {
		err = register_once(dev, cli->args.dmabuf_fd, vaddr, 0, &usecs[0][i],
				    &usecs[1][i]);
		if (err) {
			goto exit;
		}
	}

	printf("xnvme_perf_xfer_register:\n");
	printf("  uri: '%s'\n", cli->args.uri);
	printf("  be: '%s'\n", cli->args.be ? cli->args.be : "");
	printf("  homi_id: %u\n", opts.homi_id);
	printf("  dmabuf_nbytes: %jd\n", (intmax_t)nbytes);
	printf("  count: %u\n", count);
	pr_times(2, names, first, usecs, count);

exit:
	xnvme_dev_close(dev);
	if (vaddr != MAP_FAILED) {
		munmap(vaddr, (size_t)nbytes);
	}
	free(usecs[1]);
	free(usecs[0]);

	return err;
}

/**
 * Map nbytes of host memory that nothing has touched yet, of hugetlb pages of 1 << huge_shift
 * bytes, or of base pages when huge_shift is 0. Base pages are mapped 2 MiB aligned, as SPDK
 * registers no other memory, and without transparent huge pages, which would replace them.
 */
static void *
host_map(size_t nbytes, int huge_shift)
{
	const size_t align = 2 * 1024 * 1024;
	uint8_t *raw, *vaddr;
	size_t head;

	if (huge_shift) {
		vaddr = mmap(NULL, nbytes, PROT_READ | PROT_WRITE,
			     MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB |
				     (huge_shift << MAP_HUGE_SHIFT),
			     -1, 0);
		return (vaddr == MAP_FAILED) ? NULL : vaddr;
	}

	raw = mmap(NULL, nbytes + align, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,
		   0);
	if (raw == MAP_FAILED) {
		return NULL;
	}
	head = (align - ((uintptr_t)raw % align)) % align;
	vaddr = raw + head;
	if (head) {
		munmap(raw, head);
	}
	munmap(vaddr + nbytes, align - head);

	// a kernel without transparent huge pages fails this, and needs nothing of it
	(void)madvise(vaddr, nbytes, MADV_NOHUGEPAGE);

	return vaddr;
}

/**
 * Time registering nbytes of host memory and unregistering it, once and then count times. When
 * untouched is false, the memory is written before the first registration, so its pages are
 * faulted in; otherwise it is mapped afresh for each registration and never written.
 */
static int
time_host(struct xnvme_dev *dev, size_t nbytes, int huge_shift, bool untouched, uint32_t count,
	  double first[2], double *usecs[2])
{
	void *vaddr = NULL;
	int err = 0;

	for (uint32_t i = 0; i <= count; ++i) {
		double *reg = i ? &usecs[0][i - 1] : &first[0];
		double *unreg = i ? &usecs[1][i - 1] : &first[1];

		if (!vaddr) {
			vaddr = host_map(nbytes, huge_shift);
			if (!vaddr) {
				err = -errno;
				xnvme_cli_perr(
					"Error: no host memory of the pages --huge asks for; "
					"reserve hugetlb pages",
					err);
				return err;
			}
			if (!untouched) {
				memset(vaddr, 0x5a, nbytes);
			}
		}

		err = register_once(dev, -1, vaddr, nbytes, reg, unreg);
		if (err) {
			break;
		}

		if (untouched) {
			munmap(vaddr, nbytes);
			vaddr = NULL;
		}
	}
	if (vaddr) {
		munmap(vaddr, nbytes);
	}

	return err;
}

static int
register_host(struct xnvme_cli *cli, uint32_t count)
{
	static const char *names[2] = {"register", "unregister"};
	static const char *cases[2] = {"faulted", "untouched"};
	struct xnvme_opts opts = {0};
	struct xnvme_dev *dev = NULL;
	size_t sizes[XFER_NSIZES_MAX];
	double *usecs[2] = {NULL, NULL};
	size_t page_nbytes;
	int huge_shift = 0, nsizes, err = 0;

	nsizes = get_sizes(cli, sizes);
	if (nsizes < 0) {
		return nsizes;
	}
	if (cli->args.huge) {
		huge_shift = !strcmp(cli->args.huge, "2M")   ? 21
			     : !strcmp(cli->args.huge, "1G") ? 30
							     : 0;
		if (!huge_shift) {
			xnvme_cli_perr("Error: --huge takes 2M or 1G", -EINVAL);
			return -EINVAL;
		}
	}
	page_nbytes = huge_shift ? (size_t)1 << huge_shift : (size_t)sysconf(_SC_PAGESIZE);
	for (int i = 0; i < nsizes; ++i) {
		if (sizes[i] % page_nbytes) {
			xnvme_cli_perr("Error: each size of --mib must be whole pages of --huge",
				       -EINVAL);
			return -EINVAL;
		}
	}

	usecs[0] = calloc(count, sizeof(double));
	usecs[1] = calloc(count, sizeof(double));
	if (!usecs[0] || !usecs[1]) {
		err = -ENOMEM;
		xnvme_cli_perr("Error: no memory for the times", err);
		goto exit;
	}

	xnvme_opts_set_defaults(&opts);
	xnvme_cli_to_opts(cli, &opts);
	dev = xnvme_dev_open(cli->args.uri, &opts);
	if (!dev) {
		err = -errno;
		xnvme_cli_perr("xnvme_dev_open()", err);
		goto exit;
	}

	printf("xnvme_perf_xfer_register:\n");
	printf("  uri: '%s'\n", cli->args.uri);
	printf("  be: '%s'\n", cli->args.be ? cli->args.be : "");
	printf("  homi_id: %u\n", opts.homi_id);
	printf("  page_nbytes: %zu\n", page_nbytes);
	printf("  count: %u\n", count);
	printf("  sizes:\n");
	for (int i = 0; i < nsizes; ++i) {
		printf("  - nbytes: %zu\n", sizes[i]);
		for (int untouched = 0; untouched < 2; ++untouched) {
			double first[2];

			err = time_host(dev, sizes[i], huge_shift, untouched, count, first, usecs);
			if (err) {
				goto exit;
			}
			printf("    %s:\n", cases[untouched]);
			pr_times(6, names, first, usecs, count);
		}
		fflush(stdout);
	}

exit:
	xnvme_dev_close(dev);
	free(usecs[1]);
	free(usecs[0]);

	return err;
}
#endif

static int
sub_register(struct xnvme_cli *cli)
{
	uint32_t count;
	int err;

	err = get_count(cli, &count);
	if (err) {
		return err;
	}
#ifdef __linux__
	if (!cli->given[XNVME_CLI_OPT_DMABUF_FD] == !cli->given[XNVME_CLI_OPT_MIB]) {
		xnvme_cli_perr("Error: give either --dmabuf-fd or --mib", -EINVAL);
		return -EINVAL;
	}
	if (cli->given[XNVME_CLI_OPT_HUGE] && !cli->given[XNVME_CLI_OPT_MIB]) {
		xnvme_cli_perr("Error: --huge applies to the host memory of --mib", -EINVAL);
		return -EINVAL;
	}

	return cli->given[XNVME_CLI_OPT_DMABUF_FD] ? register_dmabuf(cli, count)
						   : register_host(cli, count);
#else
	err = -ENOTSUP;
	xnvme_cli_perr("Error: register maps memory with the interfaces of Linux", err);
	return err;
#endif
}

static int
sub_heap(struct xnvme_cli *cli)
{
#ifdef __linux__
	static const char *names[2] = {"open", "close"};
	const char *vfio_mode = getenv("XNVME_UPCIE_VFIO_MODE");
	struct xnvme_opts opts = {0};
	size_t sizes[XFER_NSIZES_MAX];
	double *usecs[2] = {NULL, NULL};
	uint32_t count;
	int nsizes, err;

	err = get_count(cli, &count);
	if (err) {
		return err;
	}
	nsizes = get_sizes(cli, sizes);
	if (nsizes < 0) {
		return nsizes;
	}

	usecs[0] = calloc(count, sizeof(double));
	usecs[1] = calloc(count, sizeof(double));
	if (!usecs[0] || !usecs[1]) {
		err = -ENOMEM;
		xnvme_cli_perr("Error: no memory for the times", err);
		goto exit;
	}

	xnvme_opts_set_defaults(&opts);
	xnvme_cli_to_opts(cli, &opts);

	printf("xnvme_perf_xfer_heap:\n");
	printf("  uri: '%s'\n", cli->args.uri);
	printf("  be: '%s'\n", cli->args.be ? cli->args.be : "");
	printf("  homi_id: %u\n", opts.homi_id);
	printf("  upcie_vfio_mode: '%s'\n", vfio_mode ? vfio_mode : "");
	printf("  count: %u\n", count);
	printf("  sizes:\n");
	for (int i = 0; i < nsizes; ++i) {
		double first[2];

		opts.host_heap_size = sizes[i];
		for (uint32_t j = 0; j <= count; ++j) {
			double *open_usecs = j ? &usecs[0][j - 1] : &first[0];
			double *close_usecs = j ? &usecs[1][j - 1] : &first[1];
			struct xnvme_dev *dev;
			struct timespec t0;

			clock_gettime(CLOCK_MONOTONIC, &t0);
			dev = xnvme_dev_open(cli->args.uri, &opts);
			*open_usecs = usecs_since(&t0);
			if (!dev) {
				err = -errno;
				xnvme_cli_perr("xnvme_dev_open()", err);
				goto exit;
			}

			clock_gettime(CLOCK_MONOTONIC, &t0);
			xnvme_dev_close(dev);
			*close_usecs = usecs_since(&t0);
		}

		printf("  - host_heap_nbytes: %zu\n", sizes[i]);
		pr_times(4, names, first, usecs, count);
		fflush(stdout);
	}

exit:
	free(usecs[1]);
	free(usecs[0]);

	return err;
#else
	xnvme_cli_perr("Error: heap is implemented for Linux only", -ENOTSUP);
	return -ENOTSUP;
#endif
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"htod",
		"Host-to-device copy bandwidth ceiling (requires a GPU backend build)",
		"Keep --qdepth pinned copies of --iosize in flight to one GPU for --runtime "
		"seconds and report the delivered host-to-device GB/s. Takes no NVMe devices.",
		sub_htod,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_IOSIZE, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_RUNTIME, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_GPU_ID, XNVME_CLI_LOPT},
		},
	},
	{
		"register",
		"Time registering memory with a device, and unregistering it",
		"Register memory with the device and unregister it, once and then --count times, "
		"100 by default. Report the microseconds of the first registration and "
		"unregistering apart, and the minimum, median and maximum of the --count after "
		"them, so that a cost only the first call pays does not count in those. The "
		"memory is the dma-buf the process inherited as --dmabuf-fd, named by addresses "
		"reserved with PROT_NONE, which nothing reads or writes; its size is the "
		"exporter's. Or it is host memory of each size of --mib, of base pages or of the "
		"hugetlb pages of --huge, timed twice: written before the first registration, so "
		"that its pages are faulted in, and untouched, mapped afresh for each "
		"registration and never written. With --homi-id the registrations go through the "
		"server.",
		sub_register,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_DMABUF_FD, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_MIB, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_HUGE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_HOMI_ID, XNVME_CLI_LOPT},
		},
	},
	{
		"heap",
		"Time opening a device with a host heap of each size, and closing it",
		"Open the device with a host heap of each size of --mib and close it, once and "
		"then --count times, 100 by default. Report the microseconds of the first open "
		"and close apart, and the minimum, median and maximum of the --count after them. "
		"The upcie backends allocate the heap of hugetlb pages, lock it and map it for "
		"the device as they open it; XNVME_UPCIE_VFIO_MODE, iommufd or type1, chooses how "
		"they map it, and is printed. Other backends have no host heap.",
		sub_heap,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_MIB, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_HOMI_ID, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "xnvme perf-xfer - data transfer benchmarks between memories",
	.vcs = XNVME_VCS_TAG,
	.descr_short = "Run data transfer benchmarks between memories",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
