// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libxnvme.h>

/**
 * This example shows how send a single asynchronous read command
 * and wait for completion using eventfd.
 *
 * - Allocate a buffer for the command payload
 * - Setup a Command Queue
 *   | Using command-contexts
 * - Submit the asynchronous command
 * - Reporting on IO-errors
 * - Teardown
 */
static int
read_first_block(struct xnvme_dev *dev)
{
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct xnvme_cmd_ctx *ctx = NULL;
	struct xnvme_queue *queue;
	size_t buf_nbytes;
	char *buf = NULL;
	int completion_fd;
	uint64_t ptr;
	int err;

	// Initialize a command-queue
	err = xnvme_queue_init(dev, 16, 0, &queue);
	if (err) {
		fprintf(stderr, "xnvme_queue_init(): %s\n", strerror(-err));
		return err;
	}

	buf_nbytes = xnvme_dev_get_geo(dev)->nbytes;

	printf("Allocate a payload-buffer of nbytes: %zu\n", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		err = -errno;
		fprintf(stderr, "xnvme_buf_alloc(): %s\n", strerror(-err));
		goto exit;
	}

	err = xnvme_buf_clear(buf, buf_nbytes);
	if (err) {
		fprintf(stderr, "xnvme_buf_clear(): %s\n", strerror(-err));
		goto exit;
	}

	completion_fd = xnvme_queue_get_completion_fd(queue);
	if (completion_fd < 0) {
		err = completion_fd;
		fprintf(stderr, "xnvme_queue_get_completion_fd(): %s\n", strerror(-err));
		goto exit;
	}

	ctx = xnvme_queue_get_cmd_ctx(queue);

	// Submit a read command
	err = xnvme_nvm_read(ctx, nsid, 0x0, 0, buf, NULL);
	if (err) {
		fprintf(stderr, "error submitting: %s\n", strerror(-err));
		goto exit;
	}

	err = read(completion_fd, &ptr, sizeof(ptr));
	if (err < 0) {
		err = -errno;
		fprintf(stderr, "error reading eventfd: %s\n", strerror(-err));
		goto exit;
	}

	// This must process 1 command.
	err = xnvme_queue_poke(queue, 0);
	if (err < 0) {
		fprintf(stderr, "error Processing completions: %s\n", strerror(-err));
		goto exit;
	} else if (err != 1) {
		printf("unexpected number of completions %d\n", err);
		err = -EIO;
		goto exit;
	}

	xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);

exit:
	if (ctx) {
		xnvme_queue_put_cmd_ctx(queue, ctx);
	}
	xnvme_buf_free(dev, buf);
	xnvme_queue_term(queue);

	return err < 0 ? err : 0;
}

static const struct option g_lopts[] = {
	// Given to xnvme_dev_open() in struct xnvme_opts
	{"be", required_argument, NULL, 'b'},     {"admin", required_argument, NULL, 'A'},
	{"sync", required_argument, NULL, 's'},   {"async", required_argument, NULL, 'a'},
	{"mem", required_argument, NULL, 'm'},    {"dev-nsid", required_argument, NULL, 'N'},
	{"direct", required_argument, NULL, 'D'}, {"homi-id", required_argument, NULL, 'H'},
	{"help", no_argument, NULL, 'h'},         {NULL, 0, NULL, 0},
};

static void
usage(const char *name)
{
	printf("Usage: %s <uri> [options]\n\n"
	       "Read the first block of the device at <uri> with one asynchronous command, "
	       "waiting "
	       "for its completion on an eventfd\n\n"
	       "Options:\n"
	       "  --be STRING     Backend\n"
	       "  --admin STRING  Admin command-interface\n"
	       "  --sync STRING   Synchronous command-interface\n"
	       "  --async STRING  Asynchronous command-interface\n"
	       "  --mem STRING    Memory allocator for buffers\n"
	       "  --dev-nsid NUM  Namespace to open the device with\n"
	       "  --direct NUM    Bypass OS-managed caching\n"
	       "  --homi-id NUM   Identifier of the controller host\n",
	       name);
}

int
main(int argc, char **argv)
{
	struct xnvme_opts opts = {0};
	struct xnvme_dev *dev;
	int opt, err;

	xnvme_opts_set_defaults(&opts);

	if (argc < 2 || !strcmp(argv[1], "--help")) {
		usage(argv[0]);
		return argc < 2;
	}

	optind = 2;
	while ((opt = getopt_long(argc, argv, "", g_lopts, NULL)) != -1) {
		switch (opt) {
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

	dev = xnvme_dev_open(argv[1], &opts);
	if (!dev) {
		fprintf(stderr, "xnvme_dev_open(): %s\n", strerror(errno));
		return 1;
	}

	err = read_first_block(dev);

	xnvme_dev_close(dev);

	return err ? 1 : 0;
}
