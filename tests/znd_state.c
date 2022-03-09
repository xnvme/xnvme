// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_spec_pp.h>
#include <libxnvme_nvm.h>
#include <libxnvme_znd.h>
#include <libxnvmec.h>

struct action {
	enum xnvme_spec_znd_cmd_mgmt_send_action action;
	enum xnvme_spec_znd_state state;
};

static struct action actions[] = {
	{.action = XNVME_SPEC_ZND_CMD_MGMT_SEND_RESET, .state = XNVME_SPEC_ZND_STATE_EMPTY},
	{.action = XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN, .state = XNVME_SPEC_ZND_STATE_EOPEN},
	{.action = XNVME_SPEC_ZND_CMD_MGMT_SEND_CLOSE, .state = XNVME_SPEC_ZND_STATE_CLOSED},
	{.action = XNVME_SPEC_ZND_CMD_MGMT_SEND_FINISH, .state = XNVME_SPEC_ZND_STATE_FULL},
};
size_t nactions = sizeof actions / sizeof *actions;

/**
 * Check that a zone can be reset, opened, closed, and finished
 * When no slba is given, then 0x0 is used
 */
static int
cmd_transition(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint32_t nsid;
	uint64_t slba;
	int err;

	nsid = cli->given[XNVMEC_OPT_NSID] ? cli->args.nsid : xnvme_dev_get_nsid(cli->args.dev);
	slba = cli->given[XNVMEC_OPT_SLBA] ? cli->args.slba : 0x0;

	for (size_t i = 0; i < nactions; ++i) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		struct xnvme_spec_znd_descr zone = {0};

		err = xnvme_znd_mgmt_send(&ctx, nsid, slba, false, actions[i].action, 0x0, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvmec_perr("xnvme_znd_mgmt_send()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}

		err = xnvme_znd_descr_from_dev(dev, slba, &zone);
		if (err) {
			xnvmec_perr("xnvme_znd_descr_from_dev()", err);
			goto exit;
		}

		if (zone.zs != actions[i].state) {
			xnvmec_perr("zone.zs: %u != expected: %u", actions[i].state);
			err = EIO;
			goto exit;
		}
	}

	xnvmec_pinf("LGTM");

exit:
	return err < 0 ? err : 0;
}

/**
 * Verify that Changed Zone List log page can be retrieved when supported
 */
static int
cmd_changes(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_spec_idfy_ctrlr *idfy_ctrlr = xnvme_dev_get_ctrlr_css(dev);
	struct xnvme_spec_znd_log_changes *changes;
	int err = 0;

	if (!idfy_ctrlr->oaes.zone_changes) {
		XNVME_DEBUG("INFO: skipping; not supported");
		xnvmec_pinf("LGTM");
		return 0;
	}

	changes = xnvme_znd_log_changes_from_dev(dev);
	if (!changes) {
		xnvmec_perr("xnvme_znd_log_changes_from_dev(), errno: %d", errno);
		err = errno;
		goto exit;
	}
	xnvme_buf_free(dev, changes);

	xnvmec_pinf("LGTM");

exit:
	return err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub g_subs[] = {
	{"transition",
	 "Check zone-transition: reset a zone, open, close it, and fill it.",
	 "Check zone-transition: reset a zone, open, close it, and fill it.",
	 cmd_transition,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},
		 {XNVMEC_OPT_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_SLBA, XNVMEC_LOPT},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
	 }},

	{"changes",
	 "Retrieve the Changed Zone List log page",
	 "Retrieve the Changed Zone List log page",
	 cmd_changes,
	 {
		 {XNVMEC_OPT_URI, XNVMEC_POSA},
		 {XNVMEC_OPT_NSID, XNVMEC_LOPT},

		 {XNVMEC_OPT_DEV_NSID, XNVMEC_LOPT},
		 {XNVMEC_OPT_BE, XNVMEC_LOPT},
		 {XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		 {XNVMEC_OPT_SYNC, XNVMEC_LOPT},
	 }},

};

static struct xnvmec g_cli = {
	.title = "Tests for Zoned Random Write Area",
	.descr_short = "Tests for Zoned Random Write Area",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
