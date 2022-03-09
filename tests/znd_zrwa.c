// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvmec.h>
#include <libxnvme_spec.h>
#include <libxnvme_nvm.h>
#include <libxnvme_znd.h>

static int
test_support(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_spec_znd_idfy_ctrlr *ctrlr;
	struct xnvme_spec_znd_idfy_ns *ns;
	int err = 0;

	ctrlr = (void *)xnvme_dev_get_ctrlr_css(dev);
	if (!ctrlr) {
		err = -errno;
		xnvmec_perr("xnvme_dev_get_ctrlr_css()", -err);
		return err;
	}
	ns = (void *)xnvme_dev_get_ns_css(dev);
	if (!ns) {
		err = -errno;
		xnvmec_perr("xnvme_dev_get_ns_css()", -err);
		return err;
	}
	xnvme_spec_znd_idfy_ctrlr_pr(ctrlr, XNVME_PR_DEF);
	xnvme_spec_znd_idfy_ns_pr(ns, XNVME_PR_DEF);

	if (!ns->ozcs.bits.zrwasup) {
		err = -ENOSYS;
		xnvmec_perr("!ns->oczs.bits.zrwasup", -err);
	}

	xnvmec_pinf(err ? "ZRWA: is NOT supported" : "ZRWA: LGTM");

	return err;
}

static int
test_idfy(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_spec_znd_idfy_ctrlr *ctrlr;
	struct xnvme_spec_znd_idfy_ns *ns;
	int err = 0;

	ctrlr = (void *)xnvme_dev_get_ctrlr_css(dev);
	if (!ctrlr) {
		err = -errno;
		xnvmec_perr("xnvme_dev_get_ctrlr_css()", -err);
		return err;
	}
	ns = (void *)xnvme_dev_get_ns_css(dev);
	if (!ns) {
		err = -errno;
		xnvmec_perr("xnvme_dev_get_ns_css()", -err);
		return err;
	}
	xnvme_spec_znd_idfy_ctrlr_pr(ctrlr, XNVME_PR_DEF);
	xnvme_spec_znd_idfy_ns_pr(ns, XNVME_PR_DEF);

	xnvmec_pinf("ZRWA: LGTM");

	return err;
}

/**
 * - Retrieve a Zone-Descriptor for an empty zone
 * - EOPEN the Zone with ZRWA-allocation according to 'zrwaa'
 * - Retrieve the same Zone-Descriptor
 * - Verify that the state is EOPEN and that the ZRWAA attribute matches 'zrwaa'
 */
static int
_eopen_with_zrwa(struct xnvmec *cli, bool zrwaa)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct xnvme_spec_znd_descr zone = {0}, after = {0};
	int err;

	if (cli->given[XNVMEC_OPT_SLBA]) {
		err = xnvme_znd_descr_from_dev(dev, cli->args.slba, &zone);
		if (err) {
			xnvmec_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	} else {
		err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_EMPTY, &zone);
		if (err) {
			xnvmec_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	}
	xnvmec_pinf("Using the following zone:");
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	err = xnvme_znd_mgmt_send(&ctx, nsid, zone.zslba, false, XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN,
				  zrwaa, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_pinf("xnvme_cmd_zone_mgmt(EOPEN)");
		err = err ? err : -EIO;
		goto exit;
	}

	err = xnvme_znd_descr_from_dev(dev, zone.zslba, &after);
	if (err) {
		xnvmec_perr("xnvme_znd_descr_from_dev()", -err);
		goto exit;
	}

	xnvmec_pinf("Zone after EOPEN:");
	xnvme_spec_znd_descr_pr(&after, XNVME_PR_DEF);
	if (after.za.zrwav != zrwaa) {
		err = -EIO;
		xnvmec_pinf("INVALID: after->za.zrwav: %d != zrwav: %d", after.za.zrwav, zrwaa);
	}
	if (after.zs != XNVME_SPEC_ZND_STATE_EOPEN) {
		err = -EIO;
		xnvmec_pinf("INVALID: after->zs: %d", after.zs);
	}

	if (!err) {
		xnvmec_pinf("LGMT");
	}

exit:
	return err;
}

static int
test_eopen_with_zrwa(struct xnvmec *cli)
{
	return _eopen_with_zrwa(cli, true);
}

static int
test_eopen_without_zrwa(struct xnvmec *cli)
{
	return _eopen_with_zrwa(cli, false);
}

static int
test_flush(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	struct xnvme_spec_znd_idfy_ns *zns = (void *)xnvme_dev_get_ns_css(dev);
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.dev);
	struct xnvme_spec_znd_descr zone = {0};

	// For the write-payload
	void *dbuf = NULL;
	size_t dbuf_nbytes = geo->lba_nbytes;

	uint64_t zrwa_naddr = zns->zrwafg;
	int err;

	if (cli->given[XNVMEC_OPT_SLBA]) {
		err = xnvme_znd_descr_from_dev(dev, cli->args.slba, &zone);
		if (err) {
			xnvmec_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	} else {
		err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_EMPTY, &zone);
		if (err) {
			xnvmec_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	}
	xnvmec_pinf("Using the following zone:");
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvmec_buf_fill(dbuf, dbuf_nbytes, "anum");
	if (err) {
		xnvmec_perr("xnvmec_buf_fill()", err);
		goto exit;
	}

	// Open Zone with Random Write Area
	{
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);

		err = xnvme_znd_mgmt_send(&ctx, nsid, zone.zslba, false,
					  XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN,
					  XNVME_SPEC_ZND_MGMT_OPEN_WITH_ZRWA, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvmec_perr("xnvme_znd_mgmt_send()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
	}

	// Fill a zone using write, multiple times to the same LBAs,
	// dis-regarding the wp, and flushting at boundary
	for (size_t ai = 0; ai < zone.zcap; ai += zrwa_naddr) {

		uint64_t bound = XNVME_MIN(zone.zcap - ai, zrwa_naddr);

		for (uint64_t count = 0; count < bound; ++count) {
			struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
			uint64_t slba = zone.zslba + ai + count;

			err = xnvme_nvm_write(&ctx, nsid, slba, 0, dbuf, NULL);
			if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
				xnvmec_perr("xnvme_nvm_write()", err);
				xnvmec_pinf("slba: 0x%016lx", slba);
				xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
				err = err ? err : -EIO;
				goto exit;
			}
		}

		// Flush the ZRWA
		{
			struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
			uint64_t lba = zone.zslba + ai + bound - 1;

			err = xnvme_znd_zrwa_flush(&ctx, nsid, lba);
			if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
				xnvmec_perr("xnvme_znd_zrwa_flush()", err);
				xnvmec_pinf("ai: %zu, lba: 0x%016lx", ai, lba);
				xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
				err = err ? err : -EIO;
				goto exit;
			}
		}
	}

exit:
	xnvmec_pinf("ZRWA-Flush: %s", err ? "FAILED" : "LGTM");

	xnvme_buf_free(dev, dbuf);

	return err;
}

static int
test_flush_implicit(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	struct xnvme_spec_znd_idfy_ns *zns = (void *)xnvme_dev_get_ns_css(dev);
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.dev);
	struct xnvme_spec_znd_descr zone = {0};

	// For the write-payload
	void *dbuf = NULL;
	size_t dbuf_nbytes = geo->lba_nbytes;

	// uint64_t zrwa_naddr = zns->zrwas;
	uint64_t zrwa_naddr = zns->zrwafg + 1;
	int err;

	if (cli->given[XNVMEC_OPT_SLBA]) {
		err = xnvme_znd_descr_from_dev(dev, cli->args.slba, &zone);
		if (err) {
			xnvmec_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	} else {
		err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_EMPTY, &zone);
		if (err) {
			xnvmec_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	}
	xnvmec_pinf("Using the following zone:");
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvmec_buf_fill(dbuf, dbuf_nbytes, "anum");
	if (err) {
		xnvmec_perr("xnvmec_buf_fill()", err);
		goto exit;
	}

	// Open Zone with Random Write Area
	{
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);

		err = xnvme_znd_mgmt_send(&ctx, nsid, zone.zslba, false,
					  XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN,
					  XNVME_SPEC_ZND_MGMT_OPEN_WITH_ZRWA, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvmec_perr("xnvme_znd_mgmt_send()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
	}

	// Fill a zone using write, multiple times to the same LBAs,
	// dis-regarding the wp, and flushting at boundary
	for (size_t ai = 0; ai < zone.zcap; ai += zrwa_naddr) {

		uint64_t bound = XNVME_MIN(zone.zcap - ai, zrwa_naddr);

		for (uint64_t count = 0; count < bound; ++count) {
			struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
			uint64_t slba = zone.zslba + ai + count;

			err = xnvme_nvm_write(&ctx, nsid, slba, 0, dbuf, NULL);
			if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
				xnvmec_perr("xnvme_nvm_write()", err);
				xnvmec_pinf("slba: 0x%016lx", slba);
				xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
				err = err ? err : -EIO;
				goto exit;
			}
		}
	}

exit:
	xnvmec_pinf("ZRWA-Flush-Implicit: %s", err ? "FAILED" : "LGTM");

	xnvme_buf_free(dev, dbuf);

	return err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub subs[] = {
	{"support",
	 "Print ZRWA related idfy-fields and check for command support",
	 "Print ZRWA related idfy-fields and check for command support",
	 test_support,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
		 {XNVMEC_OPT_ASYNC, XNVMEC_LOPT},
	 }},

	{"idfy",
	 "Print ZRWA related idfy-fields",
	 "Print ZRWA related idfy-fields",
	 test_idfy,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
		 {XNVMEC_OPT_ASYNC, XNVMEC_LOPT},
	 }},

	{"open-with-zrwa",
	 "Verify that EOPEN with ZRWAA allocates ZRWA",
	 "Verify that EOPEN with ZRWAA allocates ZRWA",
	 test_eopen_with_zrwa,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},
		 {XNVMEC_OPT_SLBA, XNVMEC_LOPT},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
		 {XNVMEC_OPT_ASYNC, XNVMEC_LOPT},
	 }},

	{"open-without-zrwa",
	 "Verify that EOPEN without ZRWAA does not allocate ZRWA",
	 "Verify that EOPEN without ZRWAA does not allocate ZRWA",
	 test_eopen_without_zrwa,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},
		 {XNVMEC_OPT_SLBA, XNVMEC_LOPT},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
		 {XNVMEC_OPT_ASYNC, XNVMEC_LOPT},
	 }},

	{"flush",
	 "Verify operation of flush",
	 "Verify operation of flush",
	 test_flush,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},
		 {XNVMEC_OPT_SLBA, XNVMEC_LOPT},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
		 {XNVMEC_OPT_ASYNC, XNVMEC_LOPT},
	 }},

	{"flush-explicit",
	 "Verify operation of explicit-flush",
	 "Verify operation of explicit-flush",
	 test_flush,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},
		 {XNVMEC_OPT_SLBA, XNVMEC_LOPT},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
		 {XNVMEC_OPT_ASYNC, XNVMEC_LOPT},
	 }},

	{"flush-implicit",
	 "Verify operation of implicit-flush",
	 "Verify operation of implicit-flush",
	 test_flush_implicit,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},
		 {XNVMEC_OPT_SLBA, XNVMEC_LOPT},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
		 {XNVMEC_OPT_ASYNC, XNVMEC_LOPT},
	 }},
};

static struct xnvmec cli = {
	.title = "Zone-Random-Write-Area (ZRWA) Verification",
	.descr_short = "Zone-Random-Write-Area (ZRWA) Verification",
	.subs = subs,
	.nsubs = sizeof subs / sizeof(*subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
