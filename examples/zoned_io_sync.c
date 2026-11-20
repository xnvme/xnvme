// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxnvme.h>

/**
 * This example shows how to fill a Zone using synchronous write commands
 *
 * - Retrieve the descriptor of an empty Zone
 * - Allocate buffers for Command Payload
 * - Write the Zone one LBA at a time, from its start to its capacity
 * - Reporting on IO-errors
 * - Teardown
 */
static int
zone_write(struct xnvme_dev *dev)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct xnvme_spec_znd_descr zone = {0};

	size_t buf_nbytes;
	char *buf = NULL;
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

	xnvme_timer_start(&timer);

	for (uint64_t sect = 0; sect < zone.zcap; ++sect) {
		struct xnvme_cmd_ctx ctx;
		void *payload = buf + sect * geo->lba_nbytes;

		xnvme_dev_get_cmd_ctx(dev, &ctx);
		err = xnvme_nvm_write(&ctx, nsid, zone.zslba + sect, 0, payload, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			err = err ? err : -EIO;
			fprintf(stderr, "xnvme_nvm_write(): %s\n", strerror(-err));
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			goto exit;
		}
	}

	xnvme_timer_stop(&timer);
	xnvme_timer_bw_pr(&timer, "Wall-clock", buf_nbytes);

exit:
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
	       "Fill the first empty zone of the device at <uri> with synchronous write "
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

	err = zone_write(dev);

	xnvme_dev_close(dev);

	return err ? 1 : 0;
}
