// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

static int
sub_show(struct xnvme_cli *cli)
{
	struct xnvme_spec_cmd cmd = {0};
	int err;

	err = xnvme_buf_from_file(&cmd, sizeof(cmd), cli->args.cmd_input);
	if (err) {
		xnvme_cli_perr("xnvme_buf_from_file()", err);
		xnvme_cli_pinf("Error reading: '%s'", cli->args.cmd_input);
		return err;
	}

	xnvme_spec_cmd_pr(&cmd, XNVME_PR_DEF);

	return 0;
}

static int
sub_create(struct xnvme_cli *cli)
{
	struct xnvme_spec_cmd cmd = {0};
	uint32_t *cdw = (void *)&cmd;
	int err;

	if (cli->given[XNVME_CLI_OPT_NSID]) {
		cmd.common.nsid = cli->args.nsid;
	}

	if (cli->given[XNVME_CLI_OPT_OPCODE]) {
		cmd.common.opcode = cli->args.opcode;
	}

	for (int oi = XNVME_CLI_OPT_CDW00; oi <= XNVME_CLI_OPT_CDW15; ++oi) {
		if (!cli->given[oi]) {
			continue;
		}

		switch (oi) {
		case XNVME_CLI_OPT_CDW00:
			cdw[0] = cli->args.cdw[0];
			break;
		case XNVME_CLI_OPT_CDW01:
			cdw[1] = cli->args.cdw[1];
			break;
		case XNVME_CLI_OPT_CDW02:
			cdw[2] = cli->args.cdw[2];
			break;
		case XNVME_CLI_OPT_CDW03:
			cdw[3] = cli->args.cdw[3];
			break;
		case XNVME_CLI_OPT_CDW04:
			cdw[4] = cli->args.cdw[4];
			break;
		case XNVME_CLI_OPT_CDW05:
			cdw[5] = cli->args.cdw[5];
			break;
		case XNVME_CLI_OPT_CDW06:
			cdw[6] = cli->args.cdw[6];
			break;
		case XNVME_CLI_OPT_CDW07:
			cdw[7] = cli->args.cdw[7];
			break;
		case XNVME_CLI_OPT_CDW08:
			cdw[8] = cli->args.cdw[8];
			break;
		case XNVME_CLI_OPT_CDW09:
			cdw[9] = cli->args.cdw[9];
			break;
		case XNVME_CLI_OPT_CDW10:
			cdw[10] = cli->args.cdw[10];
			break;
		case XNVME_CLI_OPT_CDW11:
			cdw[11] = cli->args.cdw[11];
			break;
		case XNVME_CLI_OPT_CDW12:
			cdw[12] = cli->args.cdw[12];
			break;
		case XNVME_CLI_OPT_CDW13:
			cdw[13] = cli->args.cdw[13];
			break;
		case XNVME_CLI_OPT_CDW14:
			cdw[14] = cli->args.cdw[14];
			break;
		case XNVME_CLI_OPT_CDW15:
			cdw[15] = cli->args.cdw[15];
			break;
		}
	}

	err = xnvme_buf_to_file(&cmd, sizeof(cmd), cli->args.cmd_output);
	if (err) {
		xnvme_cli_perr("xnvme_buf_to_file()", err);
		xnvme_cli_pinf("Error writing: '%s'", cli->args.cmd_output);
		return err;
	}

	if (cli->args.verbose) {
		xnvme_spec_cmd_pr(&cmd, XNVME_PR_DEF);
	}

	return 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"show",
		"Print the given NVMe command-file",
		"Print the given NVMe command-file",
		sub_show,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_CMD_INPUT, XNVME_CLI_LREQ},
		},
	},
	{
		"create",
		"Create an NVMe command-file",
		"Create an NVMe command-file",
		sub_create,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_CMD_OUTPUT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_CDW00, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW01, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW02, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW03, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW04, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW04, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW05, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW06, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW07, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW08, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW09, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW10, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW11, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW12, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW13, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW14, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CDW15, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_OPCODE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "nvmec: NVMe Command Builder",
	.descr_short = ""
		       "Construct, and show,  NVMe Command files for use with"
		       "e.g.the xNVMe CLI",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
