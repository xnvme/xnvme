// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libznd.h>
#include <libxnvmec.h>

static int
arbitrary_zone(struct xnvmec *cli, enum znd_state state, uint64_t *lba)
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

	err = znd_report_find_arbitrary(report, state, lba, cli->args.seed);
	if (err) {
		xnvmec_pinfo("Could not find a zone");
	}

	xnvme_buf_virt_free(report);

	return err;
}

static int
test_commit(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	struct znd_idfy_ns *zns = (void *)xnvme_dev_get_ns(dev);
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.dev);
	uint64_t zslba = cli->args.lba;

	// As reported
	uint32_t zrwa_mopen = zns->mzrwar;
	uint32_t zrwa_nbytes = zns->zrwas;
	uint32_t zrwa_cgran_nbytes = zns->zrwacg;

	// Converted from bytes to number of lbas, using the `naddr` name since
	// `nlb`/`nlbas` and such are zero-based values, these aren't
	uint32_t zrwa_naddr = zrwa_nbytes / geo->nbytes;
	uint32_t zrwa_cgran_naddr = zrwa_cgran_nbytes / geo->nbytes;

	// For the write-payload
	void *dbuf = NULL;
	size_t dbuf_nbytes = geo->nbytes;

	// For the commit-payload
	struct znd_commit_range *crange = NULL;
	int rcode = 0;

	if (!cli->given[XNVMEC_OPT_LBA]) {
		if (arbitrary_zone(cli, ZND_STATE_EMPTY, &zslba)) {
			return -1;
		}
	}

	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes, NULL);
	if (!dbuf) {
		xnvmec_perror("xnvme_buf_alloc()");
		return -1;
	}
	xnvmec_buf_fill(dbuf, dbuf_nbytes);

	crange = xnvme_buf_alloc(dev, sizeof(*crange), NULL);
	if (!dbuf) {
		xnvmec_perror("xnvme_buf_alloc()");
		xnvme_buf_free(dev, dbuf);
		return -1;
	}
	memset(crange, 0, sizeof(*crange));

	xnvmec_pinfo("using: {nsid: 0x%lx, zslba: 0x%016x}", nsid, zslba);
	xnvmec_pinfo("zrwa: {mopen: %u, nbytes: %u, cgran: %u}",
		     zrwa_mopen, zrwa_nbytes, zrwa_cgran_nbytes);
	xnvmec_pinfo("zrwa: {mopen: %u, naddr: %u, cgran_naddr: %u}",
		     zrwa_mopen, zrwa_naddr, zrwa_cgran_naddr);

	// Open Zone with Random Write Area
	{
		struct xnvme_req req = { 0 };
		int err = 0;

		err = znd_cmd_mgmt_send(dev, nsid, zslba, ZND_SEND_OPEN,
					ZND_SEND_SF_ZRWA, NULL, XNVME_CMD_SYNC,
					&req);
		if (err || xnvme_req_cpl_status(&req)) {
			xnvmec_perror("znd_cmd_mgmt_send()");
			xnvmec_pinfo("err: 0x%x, zslba: 0x%016x", err, zslba);
			xnvme_req_pr(&req, XNVME_PR_DEF);
			rcode = 1;
			goto exit;
		}
	}

	// Fill a zone using write, multiple times to the same LBAs,
	// dis-regarding the wp, and committing at boundary
	for (size_t ai = 0; ai < geo->nsect; ai += zrwa_naddr) {

		for (uint64_t count = 0; count < (zrwa_naddr * 4); ++count) {
			uint64_t rel = (count + 2) % zrwa_naddr;
			uint64_t slba = zslba + ai + rel;
			struct xnvme_req req = { 0 };
			int err = 0;

			err = xnvme_cmd_write(dev, nsid, slba, 0, dbuf, NULL,
					      XNVME_CMD_SYNC, &req);
			if (err || xnvme_req_cpl_status(&req)) {
				xnvmec_perror("xnvme_cmd_write()");
				xnvmec_pinfo("err: 0x%x, slba: 0x%016x",
					     err, slba);
				xnvme_req_pr(&req, XNVME_PR_DEF);
				rcode = 1;
				goto exit;
			}
		}

		// Commit the writes
		{
			uint64_t elba = zslba + ai + zrwa_naddr;
			struct xnvme_req req = { 0 };
			int err = 0;

			crange->elba[0] = elba;

			err = znd_cmd_zrwa_commit(dev, nsid, crange, 1,
						  XNVME_CMD_SYNC, &req);
			if (err || xnvme_req_cpl_status(&req)) {
				xnvmec_perror("znd_cmd_zrwa_commit()");
				xnvmec_pinfo("err: 0x%x, elba: 0x%016x",
					     err, elba);
				xnvme_req_pr(&req, XNVME_PR_DEF);
				rcode = 1;
				goto exit;
			}

			// TODO: verify write-pointer
		}
	}

exit:
	xnvme_buf_free(dev, dbuf);
	xnvme_buf_free(dev, crange);

	return rcode;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub subs[] = {
	{
		"commit", "jazz", "jazz", test_commit, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_LBA, XNVMEC_LOPT},
		}
	},
};

static struct xnvmec cli = {
	.title = "Tests for Zoned Random Write Area",
	.descr_short = "Tests for Zoned Random Write Area",
	.subs = subs,
	.nsubs = sizeof(subs) / sizeof(subs[0]),
};

int
main(int argc, char **argv)
{
	return xnvmec(&cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
