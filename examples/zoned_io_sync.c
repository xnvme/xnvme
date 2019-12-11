// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <libxnvme.h>
#include <libxnvmec.h>
#include <libxnvme_util.h>
#include <libznd.h>
#include <time.h>

static int
arbitrary_zone(struct xnvmec *cli, uint64_t *zlba, enum znd_state state)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct znd_report *report = NULL;
	int err = 0;

	xnvmec_pinfo("Retrieving report ...");
	report = znd_report_from_dev(dev, 0x0, 0);
	if (!report) {
		xnvmec_perror("znd_report_from_dev()");
		return -1;
	}

	xnvmec_pinfo("Scan for a seq.-write-req. zone, in state: %s",
		     znd_state_str(state));

	err = znd_report_find_arbitrary(report, state, zlba, cli->args.seed);
	if (err) {
		xnvmec_pinfo("Could not find a zone");
	}

	xnvme_buf_virt_free(report);

	return err;
}

static int
sub_sync_read(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid = cli->args.nsid;

	uint64_t zlba = cli->args.lba;
	uint16_t nlb = cli->args.nlb;
	size_t nsect = nlb + 1;

	const size_t buf_nbytes = geo->nbytes * geo->nsect;
	char *buf = NULL;
	int rcode = 0;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	if (!cli->given[XNVMEC_OPT_LBA]) {
		if (arbitrary_zone(cli, &zlba, ZND_STATE_FULL)) {
			return -1;
		}
	}

	xnvmec_pinfo("Allocating buffer ...");
	buf = xnvme_buf_alloc(dev, buf_nbytes, NULL);
	if (!buf) {
		xnvmec_perror("xnvme_buf_alloc()");
		rcode = 1;
		goto exit;
	}
	xnvmec_pinfo("Clearing it ...");
	xnvmec_buf_clear(buf, buf_nbytes);

	xnvmec_pinfo("Read zlba: 0x%016x, nlb: %u, QD(1)", zlba, nlb);
	xnvmec_pinfo("nsect: %zu, nbytes: %zu", nsect, nsect * geo->nbytes);
	xnvmec_timer_start(cli);

	for (size_t sect = 0; sect < geo->nsect; sect += nsect) {
		const size_t csect = XNVME_MIN(geo->nsect - sect, nsect);
		const uint64_t off = sect * geo->nbytes;
		const uint64_t slba = zlba + sect;

		struct xnvme_req req = { 0 };
		int err = 0;

		err = xnvme_cmd_read(dev, nsid, slba, csect - 1, buf + off,
				     NULL, XNVME_CMD_SYNC, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			xnvmec_perror("xnvme_cmd_read()");
			xnvmec_pinfo("err: 0x%x, sect: %zu, slba: 0x%016x",
				     err, sect, slba);
			xnvme_req_pr(&req, XNVME_PR_DEF);
			rcode = 1;
			goto exit;
		}
	}

	xnvmec_timer_stop(cli);
	xnvmec_timer_bw_pr(cli, "Wall-clock-Read", geo->nsect * geo->nbytes);

	xnvmec_pinfo("Completed successfully");

	if (cli->args.data_output) {
		xnvmec_pinfo("Dumping nbytes: %zu, to: '%s'",
			     buf_nbytes, cli->args.data_output);
		if (xnvmec_buf_to_file(buf, buf_nbytes,
				       cli->args.data_output)) {
			xnvmec_perror("xnvmec_buf_to_file()");
		}
	}

exit:
	xnvme_buf_free(dev, buf);

	return rcode;
}

static int
sub_sync_write(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid = cli->args.nsid;

	uint64_t zlba = cli->args.lba;
	uint16_t nlb = cli->args.nlb;
	size_t nsect = nlb + 1;

	const size_t buf_nbytes = geo->nbytes * geo->nsect;
	char *buf = NULL;
	int rcode = 0;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	if (!cli->given[XNVMEC_OPT_LBA]) {
		if (arbitrary_zone(cli, &zlba, ZND_STATE_EMPTY)) {
			return -1;
		}
	}

	xnvmec_pinfo("Allocating buffer ...");
	buf = xnvme_buf_alloc(dev, buf_nbytes, NULL);
	if (!buf) {
		xnvmec_perror("xnvme_buf_alloc()");
		rcode = 1;
		goto exit;
	}
	if (cli->args.data_input) {
		xnvmec_pinfo("Filling buf with content of '%s'",
			     cli->args.data_input);
		if (xnvmec_buf_from_file(buf, buf_nbytes,
					 cli->args.data_input)) {
			xnvmec_perror("xnvmec_buf_from_file()");
			xnvme_buf_free(dev, buf);
			return -1;
		}
	} else {
		xnvmec_pinfo("Filling buf with random data");
		xnvmec_buf_fill(buf, buf_nbytes);
	}

	xnvmec_pinfo("Writing zlba: 0x%016x, nlb: %u, QD(1)", zlba, nlb);
	xnvmec_pinfo("nsect: %zu, nbytes: %zu", nsect, nsect * geo->nbytes);
	xnvmec_timer_start(cli);

	for (size_t sect = 0; sect < geo->nsect; sect += nsect) {
		const size_t csect = XNVME_MIN(geo->nsect - sect, nsect);
		const uint64_t off = sect * geo->nbytes;
		const uint64_t slba = zlba + sect;

		struct xnvme_req req = { 0 };
		int err = 0;

		err = xnvme_cmd_write(dev, nsid, slba, csect - 1, buf + off,
				      NULL, XNVME_CMD_SYNC, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			xnvmec_perror("xnvme_cmd_write()");
			xnvmec_pinfo("err: 0x%x, sect: %zu, slba: 0x%016x",
				     err, sect, slba);
			xnvme_req_pr(&req, XNVME_PR_DEF);
			rcode = 1;
			goto exit;
		}
	}

	xnvmec_timer_stop(cli);
	xnvmec_timer_bw_pr(cli, "Wall-clock-Write", geo->nsect * geo->nbytes);

	xnvmec_pinfo("Completed successfully");

exit:
	xnvme_buf_free(dev, buf);

	return rcode;
}

static int
sub_sync_append(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid = cli->args.nsid;

	uint64_t zlba = cli->args.lba;
	uint16_t nlb = cli->args.nlb;
	size_t nsect = nlb + 1;
	int rcode = 0;

	const size_t buf_nbytes = geo->nbytes * geo->nsect;
	char *buf = NULL;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	if (!cli->given[XNVMEC_OPT_LBA]) {
		if (arbitrary_zone(cli, &zlba, ZND_STATE_EMPTY)) {
			return -1;
		}
	}

	xnvmec_pinfo("Allocating buffer ...");
	buf = xnvme_buf_alloc(dev, buf_nbytes, NULL);
	if (!buf) {
		xnvmec_perror("xnvme_buf_alloc()");
		rcode = 1;
		goto exit;
	}
	if (cli->args.data_input) {
		xnvmec_pinfo("Filling buf with content of '%s'",
			     cli->args.data_input);
		if (xnvmec_buf_from_file(buf, buf_nbytes,
					 cli->args.data_input)) {
			xnvmec_perror("xnvmec_buf_from_file()");
			xnvme_buf_free(dev, buf);
			return -1;
		}
	} else {
		xnvmec_pinfo("Filling buf with random data");
		xnvmec_buf_fill(buf, buf_nbytes);
	}

	xnvmec_pinfo("Appending zlba: 0x%016x, nlb: %u, QD(1)", zlba, nlb);
	xnvmec_pinfo("nsect: %zu, nbytes: %zu", nsect, nsect * geo->nbytes);
	xnvmec_timer_start(cli);

	for (size_t sect = 0; sect < geo->nsect; sect += nsect) {
		size_t csect = XNVME_MIN(geo->nsect - sect, nsect);
		const uint64_t off = sect * geo->nbytes;


		struct xnvme_req req = { 0 };
		int err = 0;

		err = znd_cmd_append(dev, nsid, zlba, csect - 1, buf + off,
				     NULL, XNVME_CMD_SYNC, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			xnvmec_perror("xnvme_cmd_zone_append()");
			xnvmec_pinfo("err: 0x%x, sect: %zu, zlba: 0x%016x",
				     err, sect, zlba);
			xnvme_req_pr(&req, XNVME_PR_DEF);
			rcode = 1;
			goto exit;
		}
	}

	xnvmec_timer_stop(cli);
	xnvmec_timer_bw_pr(cli, "Wall-clock-Append", geo->nsect * geo->nbytes);

	xnvmec_pinfo("Completed successfully");

exit:
	xnvme_buf_free(dev, buf);

	return rcode;
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvmec_sub subs[] = {
	{
		"read", "Zone Read of a full zone",
		"Zone Read of a full zone", sub_sync_read, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_LBA, XNVMEC_LOPT},
			{XNVMEC_OPT_NLB, XNVMEC_LOPT},
			{XNVMEC_OPT_SEED, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},

	{
		"write", "Zone Write sync. until full",
		"Zone Write sync. until full", sub_sync_write, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_LBA, XNVMEC_LOPT},
			{XNVMEC_OPT_NLB, XNVMEC_LOPT},
			{XNVMEC_OPT_SEED, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
		}
	},

	{
		"append", "Zone Append sync. until full",
		"Zone sync. until full", sub_sync_append, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_LBA, XNVMEC_LOPT},
			{XNVMEC_OPT_NLB, XNVMEC_LOPT},
			{XNVMEC_OPT_SEED, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
		}
	},
};

static struct xnvmec cli = {
	.title = "Zoned Synchronous IO Example",
	.descr_short =	"Synchronous IO: read / write / append, "
	"using 4k payload at QD1",
	.subs = subs,
	.nsubs = sizeof(subs) / sizeof(subs[0]),
};

int
main(int argc, char **argv)
{
	return xnvmec(&cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
