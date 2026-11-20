// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxnvme.h>

#define DEFAULT_QD 8

struct cb_args {
	uint32_t ecount;
	uint32_t completed;
	uint32_t submitted;
};

static void
cb_pool(struct xnvme_cmd_ctx *ctx, void *cb_arg)
{
	struct cb_args *cb_args = cb_arg;

	cb_args->completed += 1;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
		cb_args->ecount += 1;
	}

	xnvme_queue_put_cmd_ctx(ctx->async.queue, ctx);
}

/**
 * This example shows how to do asynchronous reads
 *
 * - Allocate buffers for Command Payload
 * - Setup a Command Queue
 *   | Callback functions and callback arguments
 *   | Using asynchronous command-contexts
 * - Submit read commands to read a range of LBAs continously
 *   | Re-submission when busy, reap completion, waiting till empty
 * - Reporting on IO-errors
 * - Teardown
 */
static int
read_range(struct xnvme_dev *dev, uint64_t slba, uint64_t elba, uint32_t qd)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	uint64_t naddrs = elba - slba + 1;
	size_t nbytes = naddrs * geo->nbytes;

	struct cb_args cb_args = {0};
	struct xnvme_queue *queue = NULL;

	char *buf = NULL, *payload = NULL;
	struct xnvme_timer timer = {0};
	int err;

	printf("Allocating buf of nbytes: %zu\n", nbytes);
	buf = xnvme_buf_alloc(dev, nbytes);
	if (!buf) {
		err = -errno;
		fprintf(stderr, "xnvme_buf_alloc(): %s\n", strerror(-err));
		goto exit;
	}

	err = xnvme_queue_init(dev, qd, 0, &queue);
	if (err) {
		fprintf(stderr, "xnvme_queue_init(): %s\n", strerror(-err));
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_pool, &cb_args);

	printf("Range: {slba: 0x%" PRIx64 ", naddrs: %" PRIu64 ", nbytes: %zu}, qd: %" PRIu32 "\n",
	       slba, naddrs, nbytes, qd);

	xnvme_timer_start(&timer);

	payload = buf;
	for (uint64_t curs = 0; (curs < naddrs) && !cb_args.ecount;) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

submit:
		err = xnvme_nvm_read(ctx, nsid, slba + curs, 0, payload, NULL);
		switch (err) {
		case 0:
			cb_args.submitted += 1;
			goto next;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			fprintf(stderr, "xnvme_nvm_read(): %s\n", strerror(-err));
			xnvme_queue_put_cmd_ctx(queue, ctx);
			goto exit;
		}

next:
		++curs;
		payload += geo->nbytes;
	}

	err = xnvme_queue_drain(queue);
	if (err < 0) {
		fprintf(stderr, "xnvme_queue_drain(): %s\n", strerror(-err));
		goto exit;
	}

	xnvme_timer_stop(&timer);

	if (cb_args.ecount) {
		err = -EIO;
		fprintf(stderr, "got completion errors: %s\n", strerror(-err));
		goto exit;
	}

	xnvme_timer_bw_pr(&timer, "wall-clock", nbytes);

exit:
	printf("cb_args: {submitted: %u, completed: %u, ecount: %u}\n", cb_args.submitted,
	       cb_args.completed, cb_args.ecount);

	if (queue) {
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			fprintf(stderr, "xnvme_queue_term(): %s\n", strerror(-err_exit));
		}
	}
	xnvme_buf_free(dev, buf);

	return err < 0 ? err : 0;
}

static const struct option g_lopts[] = {
	// Of the read
	{"slba", required_argument, NULL, 'S'},
	{"elba", required_argument, NULL, 'E'},
	{"qdepth", required_argument, NULL, 'q'},
	// Given to xnvme_dev_open() in struct xnvme_opts
	{"be", required_argument, NULL, 'b'},
	{"admin", required_argument, NULL, 'A'},
	{"sync", required_argument, NULL, 's'},
	{"async", required_argument, NULL, 'a'},
	{"mem", required_argument, NULL, 'm'},
	{"dev-nsid", required_argument, NULL, 'N'},
	{"direct", required_argument, NULL, 'D'},
	{"homi-id", required_argument, NULL, 'H'},
	{"help", no_argument, NULL, 'h'},
	{NULL, 0, NULL, 0},
};

static void
usage(const char *name)
{
	printf("Usage: %s <uri> [options]\n\n"
	       "Read the LBAs [SLBA,ELBA] of the device at <uri> with asynchronous commands\n\n"
	       "Options:\n"
	       "  --slba 0xNUM    Start Logical Block Address, default 0x0\n"
	       "  --elba 0xNUM    End Logical Block Address, default SLBA\n"
	       "  --qdepth NUM    Queue depth, default %d\n"
	       "  --be STRING     Backend\n"
	       "  --admin STRING  Admin command-interface\n"
	       "  --sync STRING   Synchronous command-interface\n"
	       "  --async STRING  Asynchronous command-interface\n"
	       "  --mem STRING    Memory allocator for buffers\n"
	       "  --dev-nsid NUM  Namespace to open the device with\n"
	       "  --direct NUM    Bypass OS-managed caching\n"
	       "  --homi-id NUM   Identifier of the controller host\n",
	       name, DEFAULT_QD);
}

int
main(int argc, char **argv)
{
	struct xnvme_opts opts = {0};
	uint64_t slba = 0, elba = 0;
	uint32_t qd = DEFAULT_QD;
	struct xnvme_dev *dev;
	int elba_given = 0;
	int opt, err;

	xnvme_opts_set_defaults(&opts);

	if (argc < 2 || !strcmp(argv[1], "--help")) {
		usage(argv[0]);
		return argc < 2;
	}

	optind = 2;
	while ((opt = getopt_long(argc, argv, "", g_lopts, NULL)) != -1) {
		switch (opt) {
		case 'S':
			slba = strtoull(optarg, NULL, 0);
			break;
		case 'E':
			elba = strtoull(optarg, NULL, 0);
			elba_given = 1;
			break;
		case 'q':
			qd = strtoul(optarg, NULL, 0);
			break;
		case 'b':
			opts.be = optarg;
			break;
		case 'A':
			opts.admin = optarg;
			break;
		case 's':
			opts.sync = optarg;
			break;
		case 'a':
			opts.async = optarg;
			break;
		case 'm':
			opts.mem = optarg;
			break;
		case 'N':
			opts.nsid = strtoul(optarg, NULL, 0);
			break;
		case 'D':
			opts.direct = strtoul(optarg, NULL, 0);
			break;
		case 'H':
			opts.homi_id = strtoul(optarg, NULL, 0);
			opts.shm_id = opts.homi_id;
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		default:
			return 1;
		}
	}
	if (optind < argc) {
		fprintf(stderr, "Unexpected argument: '%s'\n", argv[optind]);
		return 1;
	}
	if (!elba_given) {
		elba = slba;
	}
	if (slba > elba) {
		fprintf(stderr, "Invalid range: slba > elba\n");
		return 1;
	}

	dev = xnvme_dev_open(argv[1], &opts);
	if (!dev) {
		fprintf(stderr, "xnvme_dev_open(): %s\n", strerror(errno));
		return 1;
	}

	err = read_range(dev, slba, elba, qd);

	xnvme_dev_close(dev);

	return err ? 1 : 0;
}
