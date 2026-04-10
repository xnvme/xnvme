#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxnvme.h>

#include "xnvmeperf.h"

struct xnvmeperf_job {
	struct xnvme_dev *dev;
	struct xnvme_queue *queue;
	void *buf;
	uint32_t nsid;
	int opcode;
	size_t nbytes;
	uint64_t nblocks;
	uint16_t nlb;
	uint64_t offset;
	unsigned int seed;
	uint64_t io_completed;
	uint64_t io_failed;
	uint64_t (*get_slba)(struct xnvmeperf_job *);
	struct xnvmeperf_args *args;
};

struct xnvmeperf_thread {
	int cpu;
	int njobs;
	int job_start;
	struct xnvmeperf_job *jobs;
	struct xnvmeperf_args *args;
	double elapsed;
	struct xnvme_dev **devs;
	int ndevs;
};

#ifndef XNVME_RAND_R_ENABLED
/**
 * Minimal thread-safe PRNG for platforms without rand_r() (e.g. Windows).
 *
 * Uses the glibc LCG parameters: multiplier 1103515245, increment 12345,
 * as specified in the C standard example and used by many libc implementations.
 * Returns bits [30:16] of the updated state to avoid the low-bit cycling
 * typical of LCGs, matching the behaviour of POSIX rand_r().
 *
 * @param seed  Per-caller state; must not be shared across threads
 * @return      Pseudo-random value in [0, RAND_MAX]
 */
static int
rand_r(unsigned int *seed)
{
	*seed = *seed * 1103515245 + 12345;
	return (int)((*seed >> 16) & 0x7fff);
}
#endif

static int
pin_to_cpu(int cpu)
{
#ifdef XNVME_PTHREAD_SETAFFINITY_NP_ENABLED
	cpu_set_t cpuset;

	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	return pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
#else
	(void)cpu;
	fprintf(stderr, "Warning: CPU pinning not supported on this platform\n");
	return 0;
#endif
}

// Returns the integer value of a hexadecimal digit character.
static int
hex_digit_val(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	return c - 'A' + 10;
}

static int
parse_cpumask(const char *hex, int **cpus_out, int *ncpus_out)
{
	int len, count = 0, err;
	int *cpus;
	bool nonzero = false;

	len = strlen(hex);

	if (len > 1 && (!memcmp(hex, "0x", 2) || !memcmp(hex, "0X", 2))) {
		hex += 2;
		len -= 2;
	}

	if (len == 0) {
		err = -EINVAL;
		xnvme_cli_perr("Error: --cpumask must be a non-zero hex value", err);
		return err;
	}

	for (int i = 0; i < len; i++) {
		char c = hex[i];
		if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
		      (c >= 'A' && c <= 'F'))) {
			err = -EINVAL;
			xnvme_cli_perr("Error: --cpumask must be a non-zero hex value", err);
			return err;
		}
		if (c != '0') {
			nonzero = true;
		}
	}
	if (!nonzero) {
		err = -EINVAL;
		xnvme_cli_perr("Error: --cpumask must be a non-zero hex value", err);
		return err;
	}

	// Count set bits across all hex digits
	for (int i = 0; i < len; i++) {
		int value = hex_digit_val(hex[i]);
		for (int b = 0; b < 4; b++) {
			count += (value >> b) & 1;
		}
	}

	cpus = malloc(sizeof(int) * count);
	if (!cpus) {
		return -errno;
	}

	// Extract CPU indices; rightmost digit = lowest bits
	count = 0;
	for (int i = len - 1; i >= 0; i--) {
		int value = hex_digit_val(hex[i]);
		int bit_offset = (len - 1 - i) * 4;
		for (int b = 0; b < 4; b++) {
			if ((value >> b) & 1) {
				cpus[count++] = bit_offset + b;
			}
		}
	}

	*cpus_out = cpus;
	*ncpus_out = count;
	return 0;
}

/**
 * Returns the next sequential starting LBA and advances job->offset by nlb.
 *
 * @param job  Job whose offset is advanced
 * @return     Starting LBA before the advance
 */
static uint64_t
get_slba_seq(struct xnvmeperf_job *job)
{
	uint64_t slba = job->offset;
	job->offset += job->nlb;
	if (job->offset >= job->nblocks * job->nlb) {
		job->offset = 0;
	}
	return slba;
}

/**
 * Returns a random starting LBA aligned to nlb within the device address space.
 *
 * Uses rand_r() for thread-safe random number generation seeded per-job.
 *
 * @param job  Job whose seed is used for random generation
 * @return     Random nlb-aligned starting LBA
 */
static uint64_t
get_slba_rand(struct xnvmeperf_job *job)
{
	uint64_t slba = (rand_r(&job->seed) % job->nblocks) * job->nlb;
	return slba;
}

/**
 * Submits a single async IO using the job's get_slba function and buffer.
 *
 * @param job  Job providing opcode, nsid, nlb, buffer, and slba selector
 * @param ctx  Command context obtained from the job's queue
 * @return     0 on success, negative errno on error
 */
static int
submit_io(struct xnvmeperf_job *job, struct xnvme_cmd_ctx *ctx)
{
	uint64_t slba = job->get_slba(job);

	ctx->cmd.common.opcode = job->opcode;
	ctx->cmd.common.nsid = job->nsid;
	ctx->cmd.nvm.nlb = job->nlb - 1;
	ctx->cmd.nvm.slba = slba;

	return xnvme_cmd_pass(ctx, job->buf, (job->nbytes * job->nlb), NULL, 0);
}

/**
 * Async IO completion callback; updates job counters and returns ctx to the queue.
 *
 * @param ctx     Completed command context
 * @param cb_arg  Pointer to the xnvmeperf_job that owns the queue
 */
static void
cb_fn(struct xnvme_cmd_ctx *ctx, void *cb_arg)
{
	struct xnvmeperf_job *job = cb_arg;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		job->io_failed++;
	} else {
		job->io_completed++;
	}

	xnvme_queue_put_cmd_ctx(ctx->async.queue, ctx);
}

/**
 * Initializes a job for the given device and benchmark arguments.
 *
 * Derives geometry fields (nlb, nblocks, nbytes), sets the opcode and slba
 * selector based on the IO pattern, and creates the async queue. The caller
 * is responsible for allocating job->buf and calling xnvme_queue_term() on
 * cleanup.
 *
 * @param job   Job struct to initialize (must be zeroed by caller)
 * @param dev   Open xNVMe device handle
 * @param args  Benchmark arguments
 * @param seed  Initial seed for rand_r() used in random IO patterns
 * @return      0 on success, negative errno on error
 */
static int
setup_job(struct xnvmeperf_job *job, struct xnvme_dev *dev, struct xnvmeperf_args *args,
	  unsigned int seed)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	int err;

	job->args = args;
	job->seed = seed;
	job->dev = dev;

	job->nsid = xnvme_dev_get_nsid(job->dev);
	job->nbytes = geo->lba_nbytes;
	job->nblocks = geo->tbytes / args->iosize;
	job->nlb = args->iosize / job->nbytes;
	if (job->nlb == 0) {
		fprintf(stderr, "Error: iosize (%u) smaller than lba size (%zu)\n", args->iosize,
			job->nbytes);
		return -EINVAL;
	}

	switch (job->args->pattern) {
	case IOPATTERN_READ:
	case IOPATTERN_RANDREAD:
		job->opcode = XNVME_SPEC_NVM_OPC_READ;
		break;
	case IOPATTERN_WRITE:
	case IOPATTERN_RANDWRITE:
		job->opcode = XNVME_SPEC_NVM_OPC_WRITE;
		break;
	case IOPATTERN_VERIFY:
		// skip as opcode is set manually
		break;
	default:
		fprintf(stderr, "Error: Unsupported pattern(%d)", job->args->pattern);
		return -ENOTSUP;
	}

	switch (job->args->pattern) {
	case IOPATTERN_READ:
	case IOPATTERN_WRITE:
	case IOPATTERN_VERIFY:
		job->get_slba = get_slba_seq;
		break;
	case IOPATTERN_RANDREAD:
	case IOPATTERN_RANDWRITE:
		job->get_slba = get_slba_rand;
		break;
	default:
		fprintf(stderr, "Error: Unsupported pattern(%d)", job->args->pattern);
		return -ENOTSUP;
	}

	err = xnvme_queue_init(job->dev, args->qdepth, 0, &job->queue);
	if (err) {
		xnvme_cli_perr("Failed: xnvme_queue_init()", err);
		return err;
	}
	xnvme_queue_set_cb(job->queue, cb_fn, job);

	return 0;
}

static void
thread_term(struct xnvmeperf_thread *thread)
{
	for (int i = 0; i < thread->ndevs; i++) {
		struct xnvmeperf_job *job = &thread->jobs[i];

		if (job->buf) {
			xnvme_buf_free(job->dev, job->buf);
		}
		if (job->queue) {
			xnvme_queue_term(job->queue);
		}
	}
	free(thread->jobs);
}

static int
thread_init(struct xnvmeperf_thread *thread, struct xnvmeperf_args *args)
{
	int err;

	thread->njobs = thread->ndevs;
	thread->jobs = calloc(thread->ndevs, sizeof(struct xnvmeperf_job));
	if (!thread->jobs) {
		err = -errno;
		xnvme_cli_perr("Failed: calloc() for jobs", err);
		return err;
	}

	for (int i = 0; i < thread->ndevs; i++) {
		struct xnvmeperf_job *job = &thread->jobs[i];

		err = setup_job(job, thread->devs[(thread->job_start + i) / (int)args->nqueues],
				args, (unsigned int)(thread->cpu * 1000 + i));
		if (err) {
			xnvme_cli_perr("Failed: setup_job()", err);
			return err;
		}

		job->buf = xnvme_buf_alloc(job->dev, args->iosize);
		if (!job->buf) {
			err = -errno;
			xnvme_cli_perr("Failed: xnvme_buf_alloc()", err);
			return err;
		}
	}

	return err;
}

/**
 * Per-CPU benchmark thread; runs IO against the devices assigned to this thread.
 *
 * Sets up one job per device, fills queues to depth, then drives a time-bounded
 * IO loop. Elapsed time and per-job completion counters are written back into
 * the thread struct for aggregation by the main thread.
 *
 * @param arg  Pointer to xnvmeperf_thread describing this thread's assignment
 * @return     Always NULL
 */
static void *
thread_fn(void *arg)
{
	struct xnvmeperf_thread *thread = arg;
	struct xnvmeperf_args *args = thread->args;
	struct xnvme_timer timer = {0};
	uint64_t runtime_ns = (uint64_t)args->time * 1000000000ULL;
	int err;

	err = pin_to_cpu(thread->cpu);
	if (err) {
		fprintf(stderr, "Warning: failed to pin thread to CPU %d\n", thread->cpu);
	}

	for (int i = 0; i < thread->ndevs; i++) {
		struct xnvmeperf_job *job = &thread->jobs[i];

		err = xnvme_buf_fill(job->buf, args->iosize, "anum");
		if (err) {
			xnvme_cli_perr("Failed: xnvme_buf_fill()", err);
			return NULL;
		}
	}

	xnvme_timer_start(&timer);

	// Fill all queues to depth
	for (int i = 0; i < thread->ndevs; i++) {
		struct xnvmeperf_job *job = &thread->jobs[i];

		for (uint32_t d = 0; d < args->qdepth; d++) {
			struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(job->queue);
			if (!ctx) {
				break;
			}

			err = submit_io(job, ctx);
			if (err) {
				xnvme_queue_put_cmd_ctx(job->queue, ctx);
				break;
			}
		}
	}

	{
		unsigned int poke_count = 0;

		while (1) {
			for (int i = 0; i < thread->ndevs; i++) {
				struct xnvmeperf_job *job = &thread->jobs[i];
				struct xnvme_cmd_ctx *ctx;

				xnvme_queue_poke(job->queue, 0);

				while ((ctx = xnvme_queue_get_cmd_ctx(job->queue))) {
					if (submit_io(job, ctx)) {
						xnvme_queue_put_cmd_ctx(job->queue, ctx);
						break;
					}
				}
			}

			if ((++poke_count & 63) == 0) {
				xnvme_timer_stop(&timer);
				if (xnvme_timer_elapsed_nsecs(&timer) >= runtime_ns) {
					break;
				}
			}
		}
	}

	for (int i = 0; i < thread->ndevs; i++) {
		xnvme_queue_drain(thread->jobs[i].queue);
	}

	xnvme_timer_stop(&timer);
	thread->elapsed = xnvme_timer_elapsed_secs(&timer);

	return NULL;
}

static void *
dev_close_fn(void *arg)
{
	xnvme_dev_close(arg);
	return NULL;
}

static void
print_run_args(struct xnvmeperf_args *args, const char *pattern)
{
	printf("Running xnvmeperf with arguments:\n");
	printf("- Devices: [");
	for (int i = 0; i < args->ndevs; i++) {
		if (i) {
			printf(", ");
		}
		printf("%s", args->dev_uris[i]);
	}
	printf("] (total: %d)\n", args->ndevs);
	printf("- io pattern: %s\n", pattern);
	printf("- queues per device: %u\n", args->nqueues);
	printf("- queue depth: %u\n", args->qdepth);
	printf("- io size: %u\n", args->iosize);
	printf("- runtime: %u\n", args->time);

	if (args->ncpus) {
		printf("- CPUs: [");
		for (int i = 0; i < args->ncpus; i++) {
			if (i) {
				printf(", ");
			}
			printf("%d", args->cpus[i]);
		}
		printf("] (total: %d)\n", args->ncpus);
	}
}

/**
 * Print a performance results table.
 *
 * @param title    Header string (e.g. "xnvmeperf" or "xnvmeperf cuda-run")
 * @param elapsed  Elapsed time in seconds
 * @param uris     Device URI strings
 * @param ndevs    Number of devices
 * @param iops     Per-device IOPS
 * @param mibps    Per-device throughput in MiB/s
 * @param failed   Per-device failed I/O count
 * @param cpus     Per-device CPU strings, or NULL to omit the CPUs column
 */
static void
print_perf_results(const char *title, double elapsed, const char **uris, int ndevs,
		   const double *iops, const double *mibps, const uint64_t *failed,
		   const char **cpus)
{
	double total_iops = 0, total_mibps = 0;
	uint64_t total_failed = 0;

	printf("\n");
	printf("====================================================================\n");
	printf(" %s (elapsed: %.2fs)\n", title, elapsed);
	printf("====================================================================\n");
	if (cpus) {
		printf(" %-20s  %6s  %12s %10s %8s\n", "Device", "CPUs", "IOPS", "MiB/s",
		       "Failed");
	} else {
		printf(" %-20s  %12s %10s %8s\n", "Device", "IOPS", "MiB/s", "Failed");
	}

	for (int d = 0; d < ndevs; d++) {
		if (cpus) {
			printf(" %-20s  %6s  %12.2f %10.2f %8lu\n", uris[d], cpus[d], iops[d],
			       mibps[d], (unsigned long)failed[d]);
		} else {
			printf(" %-20s  %12.2f %10.2f %8lu\n", uris[d], iops[d], mibps[d],
			       (unsigned long)failed[d]);
		}

		total_iops += iops[d];
		total_mibps += mibps[d];
		total_failed += failed[d];
	}

	printf("--------------------------------------------------------------------\n");
	if (cpus) {
		printf(" %-29s %12.2f %10.2f %8lu\n", "Total:", total_iops, total_mibps,
		       (unsigned long)total_failed);
	} else {
		printf(" %-21s %12.2f %10.2f %8lu\n", "Total:", total_iops, total_mibps,
		       (unsigned long)total_failed);
	}
	printf("====================================================================\n");
}

static void
print_results(struct xnvmeperf_thread *threads, struct xnvmeperf_args *args)
{
	double *iops, *mibps, elapsed = 0;
	uint64_t *failed;
	char **cpus_bufs;
	const char **cpus;

	for (int t = 0; t < args->ncpus; t++) {
		if (threads[t].elapsed > elapsed) {
			elapsed = threads[t].elapsed;
		}
	}

	iops = calloc(args->ndevs, sizeof(*iops));
	mibps = calloc(args->ndevs, sizeof(*mibps));
	failed = calloc(args->ndevs, sizeof(*failed));
	cpus_bufs = calloc(args->ndevs, sizeof(*cpus_bufs));
	cpus = calloc(args->ndevs, sizeof(*cpus));
	if (!iops || !mibps || !failed || !cpus_bufs || !cpus) {
		goto done;
	}

	for (int d = 0; d < args->ndevs; d++) {
		uint64_t completed = 0;
		int cpus_len = 0;

		cpus_bufs[d] = calloc(256, 1);
		if (!cpus_bufs[d]) {
			goto done;
		}

		for (int t = 0; t < args->ncpus; t++) {
			struct xnvmeperf_thread *thread = &threads[t];

			for (int j = 0; j < thread->njobs; j++) {
				if (strcmp(args->dev_uris[(thread->job_start + j) /
							  (int)args->nqueues],
					   args->dev_uris[d]) != 0) {
					continue;
				}

				completed += thread->jobs[j].io_completed;
				failed[d] += thread->jobs[j].io_failed;

				if (cpus_len > 0) {
					cpus_len += snprintf(cpus_bufs[d] + cpus_len,
							     256 - cpus_len, ",");
				}
				cpus_len += snprintf(cpus_bufs[d] + cpus_len, 256 - cpus_len, "%d",
						     thread->cpu);
			}
		}

		iops[d] = (double)completed / elapsed;
		mibps[d] =
			((double)completed * (double)args->iosize) / (elapsed * 1024.0 * 1024.0);
		cpus[d] = cpus_bufs[d];
	}

	print_perf_results("xnvmeperf", elapsed, args->dev_uris, args->ndevs, iops, mibps, failed,
			   cpus);

done:
	if (cpus_bufs) {
		for (int i = 0; i < args->ndevs; i++) {
			free(cpus_bufs[i]);
		}
	}
	free(cpus_bufs);
	free(cpus);
	free(failed);
	free(mibps);
	free(iops);
}

/**
 * Open all devices in args and derive their geometry.
 *
 * devs must be a calloc'd array of args->ndevs null-initialised pointers.
 * On failure all successfully opened devices are closed and their entries
 * are set to NULL.
 *
 * @param args  Benchmark arguments with dev_uris, ndevs, and opts
 * @param devs  Caller-allocated array of ndevs device pointers (zero-initialised)
 * @return 0 on success, negative errno on error
 */
static int
xnvmeperf_open_devs(struct xnvmeperf_args *args, struct xnvme_dev **devs)
{
	int err;

	for (int i = 0; i < args->ndevs; i++) {
		devs[i] = xnvme_dev_open(args->dev_uris[i], &args->opts);
		if (!devs[i]) {
			err = -errno;
			fprintf(stderr, "Failed: xnvme_dev_open(%s): err(%d)\n", args->dev_uris[i],
				err);
			goto close;
		}
		err = xnvme_dev_derive_geo(devs[i]);
		if (err) {
			xnvme_cli_perr("Failed: xnvme_dev_derive_geo()", err);
			goto close;
		}
	}
	return 0;

close:
	for (int i = 0; i < args->ndevs; i++) {
		if (devs[i]) {
			xnvme_dev_close(devs[i]);
			devs[i] = NULL;
		}
	}
	return err;
}

/**
 * Runs a multi-threaded benchmark against all configured devices.
 *
 * Opens all devices, distributes them across CPU threads, and runs a
 * time-bounded async IO loop on each thread. Prints aggregated results
 * on completion.
 *
 * @param args  Benchmark arguments including devices, CPU mask, pattern, and timing
 * @return      0 on success, negative errno on error
 */
static int
xnvmeperf_run(struct xnvmeperf_args *args)
{
	struct xnvmeperf_thread *threads;
	struct xnvme_dev **devs;
	pthread_t *tids, *close_tids;
	int total_jobs, err;

	// Pre-open all devices, as they can only be opened once.
	devs = calloc(args->ndevs, sizeof(*devs));
	if (!devs) {
		err = -errno;
		xnvme_cli_perr("Failed: calloc() for devs", err);
		return err;
	}

	err = xnvmeperf_open_devs(args, devs);
	if (err) {
		free(devs);
		return err;
	}

	total_jobs = args->ndevs * (int)args->nqueues;

	// Underprovisioning: more threads than queues means threads would share a
	// queue, which is not safe. Require at least one queue per thread.
	if (args->ncpus > total_jobs) {
		fprintf(stderr,
			"Error: --cpumask specifies %d threads but only %d queue(s) "
			"(%d device(s) x %d queue(s) each); "
			"increase --nqueues so every thread has its own queue\n",
			args->ncpus, total_jobs, args->ndevs, args->nqueues);
		err = -EINVAL;
		goto close_devs;
	}

	threads = calloc(args->ncpus, sizeof(*threads));
	tids = calloc(args->ncpus, sizeof(*tids));
	if (!threads || !tids) {
		err = -ENOMEM;
		xnvme_cli_perr("Failed: calloc()", err);
		goto close_devs;
	}

	// Distribute job slots evenly across threads; overflow goes to the first
	// few threads one slot at a time.
	for (int i = 0; i < args->ncpus; i++) {
		int base = total_jobs / args->ncpus;
		int extra = total_jobs % args->ncpus;
		int start = i * base + (i < extra ? i : extra);
		int count = base + (i < extra ? 1 : 0);

		threads[i].cpu = args->cpus[i];
		threads[i].args = args;
		threads[i].ndevs = count;
		threads[i].job_start = start;
		threads[i].devs = devs;

		err = thread_init(&threads[i], args);
		if (err) {
			fprintf(stderr, "Failed: thread_init() for thread: %d, err: %d\n", i, err);
			goto close_devs;
		}
	}

	for (int i = 0; i < args->ncpus; i++) {
		err = pthread_create(&tids[i], NULL, thread_fn, &threads[i]);
		if (err) {
			xnvme_cli_perr("Failed: pthread_create()", err);
			args->ncpus = i;
			break;
		}
	}

	for (int i = 0; i < args->ncpus; i++) {
		pthread_join(tids[i], NULL);
	}

	print_results(threads, args);

	for (int i = 0; i < args->ncpus; i++) {
		thread_term(&threads[i]);
	}

close_devs:
	close_tids = calloc(args->ndevs, sizeof(*close_tids));

	// We use threads to close the devices, because it took a long time
	// to close each, so this is to speed up the process.
	if (close_tids) {
		for (int i = 0; i < args->ndevs; i++) {
			err = pthread_create(&close_tids[i], NULL, dev_close_fn, devs[i]);
			if (err) {
				// Failed creating thread, wait for all existing threads to finish
				// and close manually
				xnvme_cli_perr("Failed: pthread_create()", err);
				for (int j = 0; j < i; j++) {
					pthread_join(close_tids[j], NULL);
				}
				free(close_tids);
				goto failed_pthread_close;
			}
		}
		for (int i = 0; i < args->ndevs; i++) {
			pthread_join(close_tids[i], NULL);
		}
		free(close_tids);
	} else {
failed_pthread_close:
		for (int i = 0; i < args->ndevs; i++) {
			xnvme_dev_close(devs[i]);
		}
	}
	free(devs);
	free(threads);
	free(tids);

	return err;
}

/**
 * Fill `buf` with a verifiable per-LBA pattern.
 *
 * Each sector is filled with an "anum" background and then stamped with its
 * absolute LBA number in the first 8 bytes. This allows readback verification
 * to confirm both data integrity and that the correct sector was returned.
 *
 * @param buf     Buffer to fill; must be at least `nbytes` bytes
 * @param nbytes  Total buffer size in bytes (must equal nlb * lba_size)
 * @param slba    Starting LBA of the first sector in the buffer
 * @param nlb     Number of logical blocks in the buffer
 */
int
fill_pattern(void *buf, size_t nbytes, uint64_t slba, uint16_t nlb)
{
	size_t lba_size = nbytes / nlb;
	int err;

	for (uint16_t i = 0; i < nlb; i++) {
		uint8_t *p = (uint8_t *)buf + i * lba_size;
		uint64_t lba = slba + i;

		err = xnvme_buf_fill(p, lba_size, "anum");
		if (err) {
			xnvme_cli_perr("xnvme_buf_fill()", err);
			return err;
		}

		err = xnvme_buf_memcpy(p, &lba, sizeof(lba));
		if (err) {
			xnvme_cli_perr("xnvme_buf_memcpy()", err);
			return err;
		}
	}
	return 0;
}

/**
 * Verifies data integrity by writing and reading back a known pattern on each device.
 *
 * For each device: writes args->count sequential IOs with a per-LBA pattern,
 * then reads them back and compares against the expected pattern. Both phases
 * run with effective queue depth 1 to ensure buffer ordering correctness.
 *
 * @param args  Benchmark arguments
 * @return      0 on success, negative errno on the first device error
 */
static int
xnvmeperf_verify(struct xnvmeperf_args *args)
{
	struct xnvme_dev **devs;
	int nios = (int)args->count;
	int err;

	printf("\nxnvmeperf verify: iosize=%u, nios=%d\n", args->iosize, nios);
	printf("====================================================================\n");

	devs = calloc(args->ndevs, sizeof(*devs));
	if (!devs) {
		return -ENOMEM;
	}

	err = xnvmeperf_open_devs(args, devs);
	if (err) {
		free(devs);
		return err;
	}

	err = 0;
	for (int d = 0; d < args->ndevs; d++) {
		struct xnvme_dev *dev = devs[d];
		struct xnvmeperf_job job = {0};
		void *write_buf = NULL, *read_buf = NULL, *expect_buf = NULL;
		uint64_t mismatches = 0;

		err = setup_job(&job, dev, args, 1);
		if (err) {
			xnvme_cli_perr("Failed: setup_job()", err);
			goto next_dev;
		}

		write_buf = xnvme_buf_alloc(dev, args->iosize);
		read_buf = xnvme_buf_alloc(dev, args->iosize);
		expect_buf = malloc(args->iosize);
		if (!write_buf || !read_buf || !expect_buf) {
			err = -errno;
			xnvme_cli_perr("Failed: buffer allocation", err);
			goto next_dev;
		}

		// Phase 1: Write known patterns
		job.opcode = XNVME_SPEC_NVM_OPC_WRITE;
		job.buf = write_buf;
		job.offset = 0;
		job.io_completed = 0;
		job.io_failed = 0;

		for (int i = 0; i < nios; i++) {
			uint64_t slba = job.offset;

			err = fill_pattern(write_buf, args->iosize, slba, job.nlb);
			if (err) {
				fprintf(stderr, "Failed: fill_pattern() at IO %d, err: %d\n", i,
					err);
				break;
			}

			struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(job.queue);
			while (!ctx) {
				xnvme_queue_poke(job.queue, 0);
				ctx = xnvme_queue_get_cmd_ctx(job.queue);
			}

			err = submit_io(&job, ctx);
			if (err) {
				xnvme_queue_put_cmd_ctx(job.queue, ctx);
				fprintf(stderr, "Failed: write submit at IO %d\n", i);
				break;
			}

			// Wait for this write to complete before overwriting write_buf
			// with the next LBA's pattern; without this, all in-flight DMA
			// transfers read from whatever write_buf contains at completion time
			while (job.io_completed + job.io_failed < (uint64_t)(i + 1)) {
				xnvme_queue_poke(job.queue, 0);
			}
		}
		xnvme_queue_drain(job.queue);

		if (job.io_failed > 0) {
			fprintf(stderr, " %-20s  FAIL: %lu write errors\n", args->dev_uris[d],
				(unsigned long)job.io_failed);
			goto next_dev;
		}

		// Phase 2: Read back and verify
		job.opcode = XNVME_SPEC_NVM_OPC_READ;
		job.buf = read_buf;
		job.offset = 0;
		job.io_completed = 0;
		job.io_failed = 0;

		for (int i = 0; i < nios; i++) {
			uint64_t slba;
			size_t diff = 0;

			err = xnvme_buf_clear(read_buf, args->iosize);
			if (err) {
				fprintf(stderr, "Failed: xnvme_buf_clear() at IO %d, err: %d\n", i,
					err);
				break;
			}

			struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(job.queue);
			while (!ctx) {
				xnvme_queue_poke(job.queue, 0);
				ctx = xnvme_queue_get_cmd_ctx(job.queue);
			}

			err = submit_io(&job, ctx);
			if (err) {
				xnvme_queue_put_cmd_ctx(job.queue, ctx);
				fprintf(stderr, "Failed: read submit at IO %d\n", i);
				break;
			}

			// get_slba_seq advances job.offset by nlb on each call, so recover
			// the slba that was just submitted rather than capturing it before the
			// call
			slba = job.offset - job.nlb;

			while (job.io_completed + job.io_failed < (uint64_t)(i + 1)) {
				xnvme_queue_poke(job.queue, 0);
			}

			err = fill_pattern(expect_buf, args->iosize, slba, job.nlb);
			if (err) {
				fprintf(stderr, "Failed: fill_pattern() at IO %d, err: %d\n", i,
					err);
				break;
			}

			err = xnvme_buf_diff(expect_buf, read_buf, args->iosize, &diff);
			if (err) {
				fprintf(stderr, "Failed: xnvme_buf_diff() at IO %d, err: %d\n", i,
					err);
				break;
			}
			if (diff) {
				fprintf(stderr, "  MISMATCH at slba=%lu (IO %d)\n",
					(unsigned long)slba, i);
				mismatches++;
			}
		}
		xnvme_queue_drain(job.queue);

		printf(" %-20s  verified %d IOs, %lu mismatches, %lu failed\n", args->dev_uris[d],
		       nios, (unsigned long)mismatches, (unsigned long)job.io_failed);

next_dev:
		if (write_buf) {
			xnvme_buf_free(dev, write_buf);
		}
		if (read_buf) {
			xnvme_buf_free(dev, read_buf);
		}
		free(expect_buf);
		if (job.queue) {
			xnvme_queue_term(job.queue);
		}
	}

	printf("====================================================================\n");

	for (int i = 0; i < args->ndevs; i++) {
		xnvme_dev_close(devs[i]);
	}
	free(devs);
	return err;
}

static int
xnvmeperf_cuda_run(struct xnvmeperf_args *args)
{
	struct xnvme_dev **devs;
	uint64_t *rounds_per_dev, *failed_per_dev;
	double elapsed_s;
	float elapsed_ms = 0;
	int err;

	devs = calloc(args->ndevs, sizeof(*devs));
	if (!devs) {
		err = -errno;
		xnvme_cli_perr("Failed: calloc() for devs", err);
		return err;
	}

	err = xnvmeperf_open_devs(args, devs);
	if (err) {
		free(devs);
		return err;
	}

	rounds_per_dev = calloc(args->ndevs, sizeof(*rounds_per_dev));
	failed_per_dev = calloc(args->ndevs, sizeof(*failed_per_dev));

	if (!rounds_per_dev || !failed_per_dev) {
		err = -ENOMEM;
		xnvme_cli_perr("Failed: calloc()", err);
		free(rounds_per_dev);
		free(failed_per_dev);
		goto close_devs;
	}

	err = xnvmeperf_cuda_run_io(devs, args, rounds_per_dev, failed_per_dev, &elapsed_ms);
	if (err) {
		xnvme_cli_perr("Failed: xnvmeperf_cuda_run_io()", err);
		free(rounds_per_dev);
		free(failed_per_dev);
		goto close_devs;
	}

	elapsed_s = elapsed_ms / 1000.0;
	{
		double iops[args->ndevs], mibps[args->ndevs];

		for (int i = 0; i < args->ndevs; i++) {
			double total_ios = (double)rounds_per_dev[i] * args->qdepth;
			iops[i] = total_ios / elapsed_s;
			mibps[i] = (total_ios * args->iosize) / (elapsed_s * 1024.0 * 1024.0);
		}
		print_perf_results("xnvmeperf cuda-run", elapsed_s, args->dev_uris, args->ndevs,
				   iops, mibps, failed_per_dev, NULL);
	}

	free(rounds_per_dev);
	free(failed_per_dev);

close_devs:
	for (int i = 0; i < args->ndevs; i++) {
		xnvme_dev_close(devs[i]);
	}
	free(devs);
	return err;
}

static enum iopattern
str_to_iopattern(const char *name)
{
	static const struct {
		const char *name;
		enum iopattern pat;
	} patterns[] = {
		{"read", IOPATTERN_READ},
		{"write", IOPATTERN_WRITE},
		{"randread", IOPATTERN_RANDREAD},
		{"randwrite", IOPATTERN_RANDWRITE},
	};

	for (size_t i = 0; i < sizeof(patterns) / sizeof(*patterns); i++) {
		if (strcmp(name, patterns[i].name) == 0) {
			return patterns[i].pat;
		}
	}
	return 0;
}

/**
 * Parse the subset of CLI arguments common to all sub-commands: devices, iosize, and opts.
 *
 * @return 0 on success, negative errno on validation failure
 */
static int
parse_common_args(struct xnvme_cli *cli, struct xnvmeperf_args *args)
{
	int err = 0;

	args->ndevs = cli->args.posn_count;
	if (args->ndevs <= 0) {
		err = -EINVAL;
		xnvme_cli_perr("Error: at least one device URI is required", err);
		return err;
	}
	args->dev_uris = cli->args.posn;

	args->iosize = cli->args.iosize;
	if (!args->iosize || !xnvme_is_pow2(args->iosize)) {
		err = -EINVAL;
		xnvme_cli_perr("Error: --iosize must be a power of 2", err);
		return err;
	}

	args->opts = xnvme_opts_default();
	xnvme_cli_to_opts(cli, &args->opts);
	return err;
}

/**
 * Parse the full set of run sub-command arguments into @p args.
 * Calls parse_common_args() then adds iopattern, qdepth, and runtime.
 * Does not parse cpumask or nqueues; those are the caller's responsibility.
 *
 * @return 0 on success, negative errno on validation failure
 */
static int
parse_run_args(struct xnvme_cli *cli, struct xnvmeperf_args *args)
{
	int err = 0;

	err = parse_common_args(cli, args);
	if (err) {
		return err;
	}

	if (!cli->args.iopattern) {
		err = -EINVAL;
		xnvme_cli_perr("Error: --iopattern is required", err);
		return err;
	}
	args->pattern = str_to_iopattern(cli->args.iopattern);
	if (!args->pattern) {
		err = -EINVAL;
		fprintf(stderr, "Error: unknown iopattern '%s': err(%d)\n", cli->args.iopattern,
			err);
		return err;
	}

	args->qdepth = cli->args.qdepth;
	if (!args->qdepth || !xnvme_is_pow2(args->qdepth)) {
		err = -EINVAL;
		xnvme_cli_perr("Error: --qdepth must be a power of 2", err);
		return err;
	}

	args->time = cli->args.runtime;
	if (!args->time) {
		err = -EINVAL;
		xnvme_cli_perr("Error: --time must be a positive integer", err);
		return err;
	}

	args->nqueues = cli->args.nqueues ? cli->args.nqueues : 1;

	return err;
}

static int
sub_run(struct xnvme_cli *cli)
{
	struct xnvmeperf_args args = {0};
	int err;

	err = parse_run_args(cli, &args);
	if (err) {
		return err;
	}

	err = parse_cpumask(cli->args.cpumask, &args.cpus, &args.ncpus);
	if (err) {
		xnvme_cli_perr("Failed: parse_cpumask()", err);
		return err;
	}

	print_run_args(&args, cli->args.iopattern);

	err = xnvmeperf_run(&args);
	free(args.cpus);
	return err;
}

static int
sub_verify(struct xnvme_cli *cli)
{
	struct xnvmeperf_args args = {0};
	int err;

	err = parse_common_args(cli, &args);
	if (err) {
		return err;
	}

	if (!cli->args.count) {
		err = -EINVAL;
		xnvme_cli_perr("Error: --count must be a positive integer", err);
		return err;
	}
	args.count = (uint32_t)cli->args.count;

	args.qdepth = 1;
	args.pattern = IOPATTERN_VERIFY;

	return xnvmeperf_verify(&args);
}

static int
sub_cuda_run(struct xnvme_cli *cli)
{
	struct xnvmeperf_args args = {0};
	int err;

	err = parse_run_args(cli, &args);
	if (err) {
		return err;
	}

	if (!args.opts.be) {
		args.opts.be = "upcie-cuda";
	} else if (strcmp(args.opts.be, "upcie-cuda") != 0) {
		err = -EINVAL;
		fprintf(stderr, "Error: cuda-run requires --be upcie-cuda, got '%s': err(%d)\n",
			args.opts.be, err);
		return err;
	}

	print_run_args(&args, cli->args.iopattern);

	return xnvmeperf_cuda_run(&args);
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"run",
		"Run a benchmark against the given devices",
		"Run a time-bounded async IO benchmark against one or more NVMe devices.\n"
		"Devices are distributed across CPU threads pinned by --cpumask.",
		sub_run,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSN},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_IOPATTERN, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NQUEUES, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_IOSIZE, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_RUNTIME, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_CPUMASK, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_ORCH_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DIRECT, XNVME_CLI_LFLG},
			{XNVME_CLI_OPT_POLL_IO, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_POLL_SQ, XNVME_CLI_LOPT},
		},
	},
	{
		"verify",
		"Verify data integrity by writing and reading back a known pattern",
		"For each device: writes --count sequential IOs with a per-LBA pattern,\n"
		"then reads them back and compares against the expected data.",
		sub_verify,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSN},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_IOSIZE, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_ORCH_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DIRECT, XNVME_CLI_LFLG},
			{XNVME_CLI_OPT_POLL_IO, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_POLL_SQ, XNVME_CLI_LOPT},
		},
	},
	{
		"cuda-run",
		"Run a GPU benchmark against the given devices (requires upcie-cuda backend)",
		"Run a time-bounded GPU NVMe I/O benchmark against one or more devices.\n"
		"All devices run in parallel: one CUDA block per queue, all queues launched\n"
		"in a single kernel. All devices must use the same LBA size.",
		sub_cuda_run,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSN},
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_IOPATTERN, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NQUEUES, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_IOSIZE, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_RUNTIME, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_ORCH_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "xnvmeperf - NVMe async IO benchmark",
	.descr_short = "Run async IO benchmarks against NVMe devices",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
