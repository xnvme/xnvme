#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxnvme.h>

enum iopattern {
	IOPATTERN_READ = 1,
	IOPATTERN_WRITE = 2,
	IOPATTERN_RANDREAD = 3,
	IOPATTERN_RANDWRITE = 4,
	IOPATTERN_VERIFY = 5, ///< Used for verify subcommand
};

struct xnvmeperf_args {
	int ndevs;
	const char **dev_uris;
	int ncpus;
	int *cpus;
	uint32_t qdepth;
	uint32_t iosize;
	uint32_t time;
	uint32_t count;
	enum iopattern pattern;
	struct xnvme_opts opts;
};

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
	struct xnvmeperf_job *jobs;
	struct xnvmeperf_args *args;
	double elapsed;
	struct xnvme_dev **devs;
	const char **dev_uris;
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

static int
parse_cpumask(const char *hex, int **cpus_out, int *ncpus_out)
{
	char *endptr;
	unsigned long mask;
	unsigned long tmp;
	int count = 0, err;
	int *cpus;

	mask = strtoul(hex, &endptr, 16);
	if (*endptr != '\0' || endptr == hex || mask == 0) {
		err = -EINVAL;
		xnvme_cli_perr("Error: --cpumask must be a non-zero hex value", err);
		return err;
	}

	for (tmp = mask; tmp; tmp >>= 1) {
		count += tmp & 1;
	}

	cpus = malloc(sizeof(int) * count);
	if (!cpus) {
		return -errno;
	}

	count = 0;
	for (int i = 0; mask; i++, mask >>= 1) {
		if (mask & 1) {
			cpus[count++] = i;
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

	thread->njobs = thread->ndevs;
	thread->jobs = calloc(thread->ndevs, sizeof(struct xnvmeperf_job));
	if (!thread->jobs) {
		err = -errno;
		xnvme_cli_perr("Failed: calloc() for jobs", err);
		return NULL;
	}

	for (int i = 0; i < thread->ndevs; i++) {
		struct xnvmeperf_job *job = &thread->jobs[i];

		err = setup_job(job, thread->devs[i], args,
				(unsigned int)(thread->cpu * 1000 + i));
		if (err) {
			xnvme_cli_perr("Failed: setup_job()", err);
			goto exit;
		}

		job->buf = xnvme_buf_alloc(job->dev, args->iosize);
		if (!job->buf) {
			err = -errno;
			xnvme_cli_perr("Failed: xnvme_buf_alloc()", err);
			goto exit;
		}
		xnvme_buf_fill(job->buf, args->iosize, "anum");
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

exit:
	for (int i = 0; i < thread->ndevs; i++) {
		struct xnvmeperf_job *job = &thread->jobs[i];

		if (job->buf) {
			xnvme_buf_free(job->dev, job->buf);
		}
		if (job->queue) {
			xnvme_queue_term(job->queue);
		}
	}

	return NULL;
}

static void *
dev_close_fn(void *arg)
{
	xnvme_dev_close(arg);
	return NULL;
}

static const char *
iopattern_name(enum iopattern p)
{
	switch (p) {
	case IOPATTERN_READ:
		return "read";
	case IOPATTERN_WRITE:
		return "write";
	case IOPATTERN_RANDREAD:
		return "randread";
	case IOPATTERN_RANDWRITE:
		return "randwrite";
	default:
		return "unknown";
	}
}

static void
print_results(struct xnvmeperf_thread *threads, struct xnvmeperf_args *args)
{
	uint64_t total_completed = 0, total_failed = 0;
	double elapsed = 0;

	for (int t = 0; t < args->ncpus; t++) {
		if (threads[t].elapsed > elapsed) {
			elapsed = threads[t].elapsed;
		}
	}

	printf("\n");
	printf("====================================================================\n");
	printf(" xnvmeperf: pattern=%s, iosize=%u, qdepth=%u, time=%.2fs\n",
	       iopattern_name(args->pattern), args->iosize, args->qdepth, elapsed);
	printf("====================================================================\n");
	printf(" %-20s  %6s  %12s %10s %8s\n", "Device", "CPUs", "IOPS", "MiB/s", "Failed");

	for (int d = 0; d < args->ndevs; d++) {
		uint64_t completed = 0, failed = 0;
		char cpus[256] = {0};
		int cpus_len = 0;

		for (int t = 0; t < args->ncpus; t++) {
			struct xnvmeperf_thread *thread = &threads[t];

			for (int j = 0; j < thread->njobs; j++) {
				if (strcmp(thread->dev_uris[j], args->dev_uris[d]) != 0) {
					continue;
				}

				completed += thread->jobs[j].io_completed;
				failed += thread->jobs[j].io_failed;

				if (cpus_len > 0) {
					cpus_len += snprintf(cpus + cpus_len,
							     sizeof(cpus) - cpus_len, ",");
				}
				cpus_len += snprintf(cpus + cpus_len, sizeof(cpus) - cpus_len,
						     "%d", thread->cpu);
			}
		}

		double iops = (double)completed / elapsed;
		double mibps =
			((double)completed * (double)args->iosize) / (elapsed * 1024.0 * 1024.0);

		printf(" %-20s  %6s  %12.2f %10.2f %8lu\n", args->dev_uris[d], cpus, iops, mibps,
		       (unsigned long)failed);

		total_completed += completed;
		total_failed += failed;
	}

	printf("------------------------------------------------------------\n");
	printf(" %-29s %12.2f %10.2f %8lu\n", "Total:", (double)total_completed / elapsed,
	       ((double)total_completed * (double)args->iosize) / (elapsed * 1024.0 * 1024.0),
	       (unsigned long)total_failed);
	printf("====================================================================\n");
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
	pthread_t *tids;
	pthread_t *close_tids;
	int err = 0;

	// Pre-open all devices, as they can only be opened once.
	devs = calloc(args->ndevs, sizeof(*devs));
	if (!devs) {
		err = -errno;
		xnvme_cli_perr("Failed: calloc() for devs", err);
		return err;
	}

	for (int i = 0; i < args->ndevs; i++) {
		devs[i] = xnvme_dev_open(args->dev_uris[i], &args->opts);
		if (!devs[i]) {
			err = -errno;
			fprintf(stderr, "Failed: xnvme_dev_open(%s): err(%d)\n", args->dev_uris[i],
				err);
			for (int j = 0; j < i; j++) {
				xnvme_dev_close(devs[j]);
			}
			free(devs);
			return err;
		}
		err = xnvme_dev_derive_geo(devs[i]);
		if (err) {
			xnvme_cli_perr("Failed: xnvme_dev_derive_geo()", err);
			for (int j = 0; j <= i; j++) {
				xnvme_dev_close(devs[j]);
			}
			free(devs);
			return err;
		}
	}

	threads = calloc(args->ncpus, sizeof(*threads));
	tids = calloc(args->ncpus, sizeof(*tids));
	if (!threads || !tids) {
		err = -ENOMEM;
		xnvme_cli_perr("Failed: calloc()", err);
		goto close_devs;
	}

	for (int i = 0; i < args->ncpus; i++) {
		int start, count;

		if (args->ndevs >= args->ncpus) {
			int base = args->ndevs / args->ncpus;
			int extra = args->ndevs % args->ncpus;

			start = i * base + (i < extra ? i : extra);
			count = base + (i < extra ? 1 : 0);
		} else {
			start = i % args->ndevs;
			count = 1;
		}

		threads[i].cpu = args->cpus[i];
		threads[i].args = args;
		threads[i].ndevs = count;
		threads[i].devs = &devs[start];
		threads[i].dev_uris = &args->dev_uris[start];

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
		free(threads[i].jobs);
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
static void
fill_pattern(void *buf, size_t nbytes, uint64_t slba, uint16_t nlb)
{
	size_t lba_size = nbytes / nlb;

	for (uint16_t i = 0; i < nlb; i++) {
		uint8_t *p = (uint8_t *)buf + i * lba_size;
		uint64_t lba = slba + i;

		xnvme_buf_fill(p, lba_size, "anum");
		memcpy(p, &lba, sizeof(lba));
	}
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
	int nios = (int)args->count;
	int err = 0;

	printf("\nxnvmeperf verify: iosize=%u, nios=%d\n", args->iosize, nios);
	printf("====================================================================\n");

	for (int d = 0; d < args->ndevs; d++) {
		struct xnvme_dev *dev;
		struct xnvmeperf_job job = {0};
		void *write_buf = NULL, *read_buf = NULL, *expect_buf = NULL;
		uint64_t mismatches = 0;

		dev = xnvme_dev_open(args->dev_uris[d], &args->opts);
		if (!dev) {
			err = -errno;
			fprintf(stderr, "Failed: xnvme_dev_open(%s): err(%d)\n", args->dev_uris[d],
				err);
			continue;
		}

		err = xnvme_dev_derive_geo(dev);
		if (err) {
			xnvme_cli_perr("Failed: xnvme_dev_derive_geo()", err);
			goto next_dev;
		}

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

			fill_pattern(write_buf, args->iosize, slba, job.nlb);

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

			xnvme_buf_clear(read_buf, args->iosize);

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

			fill_pattern(expect_buf, args->iosize, slba, job.nlb);
			if (xnvme_buf_diff(expect_buf, read_buf, args->iosize) != 0) {
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
		xnvme_dev_close(dev);
	}

	printf("====================================================================\n");
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

static int
sub_run(struct xnvme_cli *cli)
{
	struct xnvmeperf_args args = {0};
	int err = 0;

	args.ndevs = cli->args.posn_count;
	if (args.ndevs <= 0) {
		fprintf(stderr, "Error: at least one device URI is required\n");
		return -EINVAL;
	}
	args.dev_uris = cli->args.posn;

	err = parse_cpumask(cli->args.cpumask, &args.cpus, &args.ncpus);
	if (err) {
		xnvme_cli_perr("Failed: parse_cpumask()", err);
		return err;
	}

	args.pattern = str_to_iopattern(cli->args.iopattern);
	if (!args.pattern) {
		fprintf(stderr, "Error: unknown iopattern '%s'\n", cli->args.iopattern);
		err = -EINVAL;
		goto free_cpus;
	}

	args.qdepth = cli->args.qdepth;
	if (!args.qdepth || !xnvme_is_pow2(args.qdepth)) {
		fprintf(stderr, "Error: --qdepth must be a power of 2\n");
		err = -EINVAL;
		goto free_cpus;
	}

	args.iosize = cli->args.iosize;
	if (!args.iosize || !xnvme_is_pow2(args.iosize)) {
		fprintf(stderr, "Error: --iosize must be a power of 2\n");
		err = -EINVAL;
		goto free_cpus;
	}

	args.time = cli->args.runtime;
	if (!args.time) {
		fprintf(stderr, "Error: --time must be a positive integer\n");
		err = -EINVAL;
		goto free_cpus;
	}

	args.opts = xnvme_opts_default();
	xnvme_cli_to_opts(cli, &args.opts);

	err = xnvmeperf_run(&args);

free_cpus:
	free(args.cpus);
	return err;
}

static int
sub_verify(struct xnvme_cli *cli)
{
	struct xnvmeperf_args args = {0};
	int err;

	args.ndevs = cli->args.posn_count;
	if (args.ndevs <= 0) {
		err = -EINVAL;
		xnvme_cli_perr("Error: at least one device URI is required", err);
		return err;
	}
	args.dev_uris = cli->args.posn;

	args.iosize = cli->args.iosize;
	if (!args.iosize || !xnvme_is_pow2(args.iosize)) {
		err = -EINVAL;
		xnvme_cli_perr("Error: --iosize must be a power of 2", err);
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
	args.opts = xnvme_opts_default();
	xnvme_cli_to_opts(cli, &args.opts);

	return xnvmeperf_verify(&args);
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
