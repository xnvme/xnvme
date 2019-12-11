// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libznd.h>
#include <libxnvmec.h>

static int
test_open_zdptr(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct znd_idfy_ns *zns = (void *)xnvme_dev_get_ns(dev);

	uint32_t zde_nbytes = zns->zonef[zns->fzsze].zdes * 64;
	uint8_t *zde = NULL;

	uint64_t zslba = cli->args.lba;
	uint64_t zidx = 0;

	struct znd_report *before = NULL, *after = NULL;
	struct xnvme_req req = { 0 };
	int rcode = 0;
	int err;

	if (!zde_nbytes) {
		xnvmec_pinfo("Invalid device: zde_nbytes: %d", zde_nbytes);
		errno = EINVAL;
		return -1;
	}

	// Allocate and fill Zone Descriptor Extension
	zde = xnvme_buf_alloc(dev, zde_nbytes, NULL);
	if (!zde) {
		xnvmec_pinfo("xnvme_buf_alloc(zde_nbytes)");
		rcode = 1;
		goto exit;
	}
	xnvmec_buf_fill(zde, zde_nbytes);

	xnvmec_pinfo("Retrieving state before EOPEN");

	before = znd_report_from_dev(dev, 0x0, 0);
	if (!before) {
		xnvmec_perror("znd_report_from_dev()");
		rcode = 1;
		goto exit;
	}

	xnvmec_pinfo("Scan for empty and sequential write req. zone");

	for (uint64_t idx = 0; idx < before->nentries; ++idx) {
		struct znd_descr *descr = ZND_REPORT_DESCR(before, idx);

		if (cli->given[XNVMEC_OPT_LBA] && \
		    (cli->args.lba != descr->zslba)) {
			continue;
		}

		if ((descr->zs == ZND_STATE_EMPTY) && \
		    (descr->zt == ZND_TYPE_SEQWR) && \
		    (descr->zcap)) {
			zslba = descr->zslba;
			zidx = idx;
			break;
		}
	}
	if (!zslba) {
		xnvmec_pinfo("Could not find a usable zslba...");
		rcode = 1;
		goto exit;
	}

	xnvmec_pinfo("Using: {zslba: 0x%016x, zidx: %zu}", zslba, zidx);

	xnvmec_pinfo("Before");
	znd_descr_pr(ZND_REPORT_DESCR(before, zidx), XNVME_PR_DEF);

	err = znd_cmd_mgmt_send(dev, nsid, zslba, ZND_SEND_RESET, 0x0, NULL,
				XNVME_CMD_SYNC, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_pinfo("znd_cmd_mgmt_send(RESET)");
		rcode = 1;
		goto exit;
	}

	xnvmec_pinfo("Sending MGMT-EOPEN");

	err = znd_cmd_mgmt_send(dev, nsid, zslba, ZND_SEND_OPEN, 0, zde,
				XNVME_CMD_SYNC, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		xnvmec_pinfo("xnvme_cmd_zone_mgmt(OPEN)");
		rcode = 1;
		goto exit;
	}

	xnvmec_pinfo("After");

	after = znd_report_from_dev(dev, 0x0, 0);
	if (!after) {
		xnvmec_perror("znd_report_from_dev");
		rcode = 1;
		goto exit;
	}

	znd_descr_pr(ZND_REPORT_DESCR(before, zidx), XNVME_PR_DEF);

	{
		// Verification
		struct znd_descr *descr = ZND_REPORT_DESCR(after, zidx);
		uint8_t *zde_after = ZND_REPORT_DEXT(after, zidx);

		if (xnvmec_buf_diff(zde, zde_after, zde_nbytes)) {
			xnvmec_buf_diff_pr(zde, descr, zde_nbytes,
					   XNVME_PR_DEF);
			rcode = 1;
			goto exit;
		}

		if (!descr->za.zdv) {
			xnvmec_pinfo("ERR: !descr->za.zdv");
			rcode = 1;
			goto exit;
		}

		if (ZND_STATE_EOPEN != descr->zs) {
			xnvmec_pinfo("ERR: invalid zc: 0x%x", descr->zs);
			rcode = 1;
			goto exit;
		}
	}

exit:
	xnvme_buf_virt_free(before);
	xnvme_buf_virt_free(after);
	xnvme_buf_free(dev, zde);
	return rcode;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub subs[] = {
	{
		"test_open_zdptr", "jazz", "jazz", test_open_zdptr, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_LBA, XNVMEC_LOPT},
		}
	},
};

static struct xnvmec cli = {
	.title = "Test Zoned Reporting and Management",
	.descr_short = "Test Zoned Reporting and Management",
	.subs = subs,
	.nsubs = sizeof(subs) / sizeof(subs[0]),
};

int
main(int argc, char **argv)
{
	return xnvmec(&cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
