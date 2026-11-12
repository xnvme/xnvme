// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

static int
test_support(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_spec_znd_idfy_ctrlr *ctrlr;
	struct xnvme_spec_znd_idfy_ns *ns;
	int err = 0;

	ctrlr = (void *)xnvme_dev_get_ctrlr_css(dev);
	if (!ctrlr) {
		err = -errno;
		xnvme_cli_perr("xnvme_dev_get_ctrlr_css()", -err);
		return err;
	}
	ns = (void *)xnvme_dev_get_ns_css(dev);
	if (!ns) {
		err = -errno;
		xnvme_cli_perr("xnvme_dev_get_ns_css()", -err);
		return err;
	}
	xnvme_spec_znd_idfy_ctrlr_pr(ctrlr, XNVME_PR_DEF);
	xnvme_spec_znd_idfy_ns_pr(ns, XNVME_PR_DEF);

	if (!xnvme_bits_get(ns->ozcs, XNVME_SPEC_ZND_IDFY_NS_OZCS_ZRWASUP)) {
		err = -ENOSYS;
		xnvme_cli_perr("!ZRWASUP of ns->ozcs", -err);
	}

	xnvme_cli_pinf(err ? "ZRWA: is NOT supported" : "ZRWA: LGTM");

	return err;
}

static int
test_idfy(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_spec_znd_idfy_ctrlr *ctrlr;
	struct xnvme_spec_znd_idfy_ns *ns;
	int err = 0;

	ctrlr = (void *)xnvme_dev_get_ctrlr_css(dev);
	if (!ctrlr) {
		err = -errno;
		xnvme_cli_perr("xnvme_dev_get_ctrlr_css()", -err);
		return err;
	}
	ns = (void *)xnvme_dev_get_ns_css(dev);
	if (!ns) {
		err = -errno;
		xnvme_cli_perr("xnvme_dev_get_ns_css()", -err);
		return err;
	}
	xnvme_spec_znd_idfy_ctrlr_pr(ctrlr, XNVME_PR_DEF);
	xnvme_spec_znd_idfy_ns_pr(ns, XNVME_PR_DEF);

	xnvme_cli_pinf("ZRWA: LGTM");

	return err;
}

/**
 * - Retrieve a Zone-Descriptor for an empty zone
 * - EOPEN the Zone with ZRWA-allocation according to 'zrwaa'
 * - Retrieve the same Zone-Descriptor
 * - Verify that the state is EOPEN and that the ZRWAA attribute matches 'zrwaa'
 */
static int
_eopen_with_zrwa(struct xnvme_cli *cli, bool zrwaa)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = {0};
	uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct xnvme_spec_znd_descr zone = {0}, after = {0};
	int err;

	xnvme_dev_get_cmd_ctx(dev, &ctx);

	if (cli->given[XNVME_CLI_OPT_SLBA]) {
		err = xnvme_znd_descr_from_dev(dev, cli->args.slba, &zone);
		if (err) {
			xnvme_cli_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	} else {
		err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_EMPTY, &zone);
		if (err) {
			xnvme_cli_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	}
	xnvme_cli_pinf("Using the following zone:");
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	struct xnvme_cmd_prep_zns_zone_management_send_args zms = {
		.args_nbytes = sizeof(zms),
		.nsid = nsid,
		.slba = zone.zslba,
		.zsa = XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN,
		.zsaso = zrwaa,
	};

	err = xnvme_cmd_prep_zns_zone_management_send(&ctx.cmd, &zms);
	if (!err) {
		err = xnvme_cmd_pass(&ctx, NULL, 0, NULL, 0);
	}
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_pinf("xnvme_cmd_zone_mgmt(EOPEN)");
		err = err ? err : -EIO;
		goto exit;
	}

	err = xnvme_znd_descr_from_dev(dev, zone.zslba, &after);
	if (err) {
		xnvme_cli_perr("xnvme_znd_descr_from_dev()", -err);
		goto exit;
	}

	xnvme_cli_pinf("Zone after EOPEN:");
	xnvme_spec_znd_descr_pr(&after, XNVME_PR_DEF);
	if (xnvme_bits_get(after.za, XNVME_SPEC_ZND_DESCR_ZA_ZRWAV) != zrwaa) {
		err = -EIO;
		xnvme_cli_pinf("INVALID: ZRWAV of after->za: %" PRIu64 " != zrwav: %d",
			       xnvme_bits_get(after.za, XNVME_SPEC_ZND_DESCR_ZA_ZRWAV), zrwaa);
	}
	if (xnvme_bits_get(after.zsattr, XNVME_SPEC_ZND_DESCR_ZSATTR_ZS) !=
	    XNVME_SPEC_ZND_STATE_EOPEN) {
		err = -EIO;
		xnvme_cli_pinf("INVALID: after->zsattr: %" PRIu64,
			       xnvme_bits_get(after.zsattr, XNVME_SPEC_ZND_DESCR_ZSATTR_ZS));
	}

	if (!err) {
		xnvme_cli_pinf("LGMT");
	}

exit:
	return err;
}

static int
test_eopen_with_zrwa(struct xnvme_cli *cli)
{
	return _eopen_with_zrwa(cli, true);
}

static int
test_eopen_without_zrwa(struct xnvme_cli *cli)
{
	return _eopen_with_zrwa(cli, false);
}

static int
test_flush(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	struct xnvme_spec_znd_idfy_ns *zns = (void *)xnvme_dev_get_ns_css(dev);
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.dev);
	struct xnvme_spec_znd_descr zone = {0};

	// For the write-payload
	void *dbuf = NULL;
	size_t dbuf_nbytes = geo->lba_nbytes;

	uint64_t zrwa_naddr = zns->zrwafg;
	int err;

	switch (geo->type) {
	case XNVME_GEO_CONVENTIONAL:
	case XNVME_GEO_ZONED:
		break;
	default:
		XNVME_DEBUG("FAILED: not nvm / zns, got; %d", geo->type);
		return -EINVAL;
	}

	if (cli->given[XNVME_CLI_OPT_SLBA]) {
		err = xnvme_znd_descr_from_dev(dev, cli->args.slba, &zone);
		if (err) {
			xnvme_cli_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	} else {
		err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_EMPTY, &zone);
		if (err) {
			xnvme_cli_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	}
	xnvme_cli_pinf("Using the following zone:");
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_fill(dbuf, dbuf_nbytes, "anum");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill()", err);
		goto exit;
	}

	// Open Zone with Random Write Area
	{
		struct xnvme_cmd_ctx ctx = {0};

		xnvme_dev_get_cmd_ctx(dev, &ctx);

		struct xnvme_cmd_prep_zns_zone_management_send_args zms = {
			.args_nbytes = sizeof(zms),
			.nsid = nsid,
			.slba = zone.zslba,
			.zsa = XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN,
			.zsaso = XNVME_SPEC_ZND_MGMT_OPEN_WITH_ZRWA,
		};

		err = xnvme_cmd_prep_zns_zone_management_send(&ctx.cmd, &zms);
		if (!err) {
			err = xnvme_cmd_pass(&ctx, NULL, 0, NULL, 0);
		}
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("zone_management_send", err);
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
			struct xnvme_cmd_ctx ctx = {0};
			uint64_t slba = zone.zslba + ai + count;

			xnvme_dev_get_cmd_ctx(dev, &ctx);

			struct xnvme_cmd_prep_nvm_write_args wr = {
				.args_nbytes = sizeof(wr),
				.nsid = nsid,
				.slba = slba,
			};

			err = xnvme_cmd_prep_nvm_write(&ctx.cmd, &wr);
			if (!err) {
				err = xnvme_cmd_pass(
					&ctx, dbuf, (size_t)xnvme_dev_get_geo(ctx.dev)->lba_nbytes,
					NULL, 0);
			}
			if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
				xnvme_cli_perr("write", err);
				xnvme_cli_pinf("slba: 0x%016lx", slba);
				xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
				err = err ? err : -EIO;
				goto exit;
			}
		}

		// Flush the ZRWA
		{
			struct xnvme_cmd_ctx ctx = {0};
			uint64_t lba = zone.zslba + ai + bound - 1;

			xnvme_dev_get_cmd_ctx(dev, &ctx);

			struct xnvme_cmd_prep_zns_zone_management_send_args flush = {
				.args_nbytes = sizeof(flush),
				.nsid = nsid,
				.slba = lba,
				.zsa = XNVME_SPEC_ZND_CMD_MGMT_SEND_FLUSH,
			};

			err = xnvme_cmd_prep_zns_zone_management_send(&ctx.cmd, &flush);
			if (!err) {
				err = xnvme_cmd_pass(&ctx, NULL, 0, NULL, 0);
			}
			if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
				xnvme_cli_perr("zone_management_send(FLUSH)", err);
				xnvme_cli_pinf("ai: %zu, lba: 0x%016lx", ai, lba);
				xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
				err = err ? err : -EIO;
				goto exit;
			}
		}
	}

exit:
	xnvme_cli_pinf("ZRWA-Flush: %s", err ? "FAILED" : "LGTM");

	xnvme_buf_free(dev, dbuf);

	return err;
}

static int
test_flush_implicit(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	struct xnvme_spec_znd_idfy_ns *zns = (void *)xnvme_dev_get_ns_css(dev);
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.dev);
	struct xnvme_spec_znd_descr zone = {0};

	// For the write-payload
	void *dbuf = NULL;
	size_t dbuf_nbytes = geo->lba_nbytes;

	// uint64_t zrwa_naddr = zns->zrwasz;
	uint64_t zrwa_naddr = zns->zrwafg + 1;
	int err;

	switch (geo->type) {
	case XNVME_GEO_CONVENTIONAL:
	case XNVME_GEO_ZONED:
		break;
	default:
		XNVME_DEBUG("FAILED: not nvm / zns, got; %d", geo->type);
		return -EINVAL;
	}

	if (cli->given[XNVME_CLI_OPT_SLBA]) {
		err = xnvme_znd_descr_from_dev(dev, cli->args.slba, &zone);
		if (err) {
			xnvme_cli_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	} else {
		err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_EMPTY, &zone);
		if (err) {
			xnvme_cli_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	}
	xnvme_cli_pinf("Using the following zone:");
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_buf_fill(dbuf, dbuf_nbytes, "anum");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill()", err);
		goto exit;
	}

	// Open Zone with Random Write Area
	{
		struct xnvme_cmd_ctx ctx = {0};

		xnvme_dev_get_cmd_ctx(dev, &ctx);

		struct xnvme_cmd_prep_zns_zone_management_send_args zms = {
			.args_nbytes = sizeof(zms),
			.nsid = nsid,
			.slba = zone.zslba,
			.zsa = XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN,
			.zsaso = XNVME_SPEC_ZND_MGMT_OPEN_WITH_ZRWA,
		};

		err = xnvme_cmd_prep_zns_zone_management_send(&ctx.cmd, &zms);
		if (!err) {
			err = xnvme_cmd_pass(&ctx, NULL, 0, NULL, 0);
		}
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("zone_management_send", err);
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
			struct xnvme_cmd_ctx ctx = {0};
			uint64_t slba = zone.zslba + ai + count;

			xnvme_dev_get_cmd_ctx(dev, &ctx);

			struct xnvme_cmd_prep_nvm_write_args wr = {
				.args_nbytes = sizeof(wr),
				.nsid = nsid,
				.slba = slba,
			};

			err = xnvme_cmd_prep_nvm_write(&ctx.cmd, &wr);
			if (!err) {
				err = xnvme_cmd_pass(
					&ctx, dbuf, (size_t)xnvme_dev_get_geo(ctx.dev)->lba_nbytes,
					NULL, 0);
			}
			if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
				xnvme_cli_perr("write", err);
				xnvme_cli_pinf("slba: 0x%016lx", slba);
				xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
				err = err ? err : -EIO;
				goto exit;
			}
		}
	}

exit:
	xnvme_cli_pinf("ZRWA-Flush-Implicit: %s", err ? "FAILED" : "LGTM");

	xnvme_buf_free(dev, dbuf);

	return err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub subs[] = {
	{
		"support",
		"Print ZRWA related idfy-fields and check for command support",
		"Print ZRWA related idfy-fields and check for command support",
		test_support,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ASYNC_OPTS,
		},
	},

	{
		"idfy",
		"Print ZRWA related idfy-fields",
		"Print ZRWA related idfy-fields",
		test_idfy,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ASYNC_OPTS,
		},
	},

	{
		"open-with-zrwa",
		"Verify that EOPEN with ZRWAA allocates ZRWA",
		"Verify that EOPEN with ZRWAA allocates ZRWA",
		test_eopen_with_zrwa,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},

			XNVME_CLI_ASYNC_OPTS,
		},
	},

	{
		"open-without-zrwa",
		"Verify that EOPEN without ZRWAA does not allocate ZRWA",
		"Verify that EOPEN without ZRWAA does not allocate ZRWA",
		test_eopen_without_zrwa,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},

			XNVME_CLI_ASYNC_OPTS,
		},
	},

	{
		"flush",
		"Verify operation of flush",
		"Verify operation of flush",
		test_flush,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},

			XNVME_CLI_ASYNC_OPTS,
		},
	},

	{
		"flush-explicit",
		"Verify operation of explicit-flush",
		"Verify operation of explicit-flush",
		test_flush,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},

			XNVME_CLI_ASYNC_OPTS,
		},
	},

	{
		"flush-implicit",
		"Verify operation of implicit-flush",
		"Verify operation of implicit-flush",
		test_flush_implicit,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},

			XNVME_CLI_ASYNC_OPTS,
		},
	},
};

static struct xnvme_cli cli = {
	.title = "Zone-Random-Write-Area (ZRWA) Verification",
	.descr_short = "Zone-Random-Write-Area (ZRWA) Verification",
	.subs = subs,
	.nsubs = sizeof subs / sizeof(*subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
