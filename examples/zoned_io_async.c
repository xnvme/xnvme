// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxnvme.h>

#define QDEPTH 8

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
 * This example shows how to fill a Zone using asynchronous Zone Append commands
 *
 * - Retrieve the descriptor of an empty Zone
 * - Allocate buffers for Command Payload
 * - Setup a Command Queue
 *   | Callback functions and callback arguments
 *   | Using asynchronous command-contexts
 * - Submit an append command for each LBA of the Zone
 *   | Re-submission when busy, reap completion, waiting till empty
 * - Reporting on IO-errors
 * - Teardown
 */
static int
zone_append(struct xnvme_dev *dev)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct xnvme_spec_znd_descr zone = {0};

	struct cb_args cb_args = {0};
	struct xnvme_queue *queue = NULL;

	char *buf = NULL, *payload = NULL;
	size_t buf_nbytes;
	struct xnvme_timer timer = {0};
	int err;

	if (geo->type != XNVME_GEO_ZONED) {
		fprintf(stderr, "Not a zoned namespace\n");
		return -EINVAL;
	}

	err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_EMPTY, &zone);
	if (err) {
		fprintf(stderr, "xnvme_znd_descr_from_dev_in_state(): %s\n", strerror(-err));
		goto exit;
	}
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	buf_nbytes = zone.zcap * geo->lba_nbytes;

	printf("Allocating and filling buf_nbytes: %zu\n", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		err = -errno;
		fprintf(stderr, "xnvme_buf_alloc(): %s\n", strerror(-err));
		goto exit;
	}

	err = xnvme_buf_fill(buf, buf_nbytes, "anum");
	if (err) {
		fprintf(stderr, "xnvme_buf_fill(): %s\n", strerror(-err));
		goto exit;
	}

	err = xnvme_queue_init(dev, QDEPTH, 0, &queue);
	if (err) {
		fprintf(stderr, "xnvme_queue_init(): %s\n", strerror(-err));
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_pool, &cb_args);

	xnvme_timer_start(&timer);

	payload = buf;
	for (uint64_t curs = 0; (curs < zone.zcap) && !cb_args.ecount;) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

submit:
		err = xnvme_znd_append(ctx, nsid, zone.zslba, 0, payload, NULL);
		switch (err) {
		case 0:
			cb_args.submitted += 1;
			goto next;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			fprintf(stderr, "xnvme_znd_append(): %s\n", strerror(-err));
			xnvme_queue_put_cmd_ctx(queue, ctx);
			goto exit;
		}

next:
		++curs;
		payload += geo->lba_nbytes;
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

	xnvme_timer_bw_pr(&timer, "Wall-clock", buf_nbytes);

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
	       "Fill the first empty zone of the device at <uri> with asynchronous Zone Append "
	       "commands\n\n"
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

	err = zone_append(dev);

	xnvme_dev_close(dev);

	return err ? 1 : 0;
}
