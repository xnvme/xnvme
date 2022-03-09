#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvmec.h>

static int
sub_show(struct xnvmec *cli)
{
	struct xnvme_spec_cmd cmd = {0};
	int err;

	err = xnvmec_buf_from_file(&cmd, sizeof(cmd), cli->args.cmd_input);
	if (err) {
		xnvmec_perr("xnvmec_buf_from_file()", err);
		xnvmec_pinf("Error reading: '%s'", cli->args.cmd_input);
		return err;
	}

	xnvme_spec_cmd_pr(&cmd, XNVME_PR_DEF);

	return 0;
}

static int
sub_create(struct xnvmec *cli)
{
	struct xnvme_spec_cmd cmd = {0};
	uint32_t *cdw = (void *)&cmd;
	int err;

	if (cli->given[XNVMEC_OPT_NSID]) {
		cmd.common.nsid = cli->args.nsid;
	}

	if (cli->given[XNVMEC_OPT_OPCODE]) {
		cmd.common.opcode = cli->args.opcode;
	}

	for (int oi = XNVMEC_OPT_CDW00; oi <= XNVMEC_OPT_CDW15; ++oi) {
		if (!cli->given[oi]) {
			continue;
		}

		switch (oi) {
		case XNVMEC_OPT_CDW00:
			cdw[0] = cli->args.cdw[0];
			break;
		case XNVMEC_OPT_CDW01:
			cdw[1] = cli->args.cdw[1];
			break;
		case XNVMEC_OPT_CDW02:
			cdw[2] = cli->args.cdw[2];
			break;
		case XNVMEC_OPT_CDW03:
			cdw[3] = cli->args.cdw[3];
			break;
		case XNVMEC_OPT_CDW04:
			cdw[4] = cli->args.cdw[4];
			break;
		case XNVMEC_OPT_CDW05:
			cdw[5] = cli->args.cdw[5];
			break;
		case XNVMEC_OPT_CDW06:
			cdw[6] = cli->args.cdw[6];
			break;
		case XNVMEC_OPT_CDW07:
			cdw[7] = cli->args.cdw[7];
			break;
		case XNVMEC_OPT_CDW08:
			cdw[8] = cli->args.cdw[8];
			break;
		case XNVMEC_OPT_CDW09:
			cdw[9] = cli->args.cdw[9];
			break;
		case XNVMEC_OPT_CDW10:
			cdw[10] = cli->args.cdw[10];
			break;
		case XNVMEC_OPT_CDW11:
			cdw[11] = cli->args.cdw[11];
			break;
		case XNVMEC_OPT_CDW12:
			cdw[12] = cli->args.cdw[12];
			break;
		case XNVMEC_OPT_CDW13:
			cdw[13] = cli->args.cdw[13];
			break;
		case XNVMEC_OPT_CDW14:
			cdw[14] = cli->args.cdw[14];
			break;
		case XNVMEC_OPT_CDW15:
			cdw[15] = cli->args.cdw[15];
			break;
		}
	}

	err = xnvmec_buf_to_file(&cmd, sizeof(cmd), cli->args.cmd_output);
	if (err) {
		xnvmec_perr("xnvmec_buf_to_file()", err);
		xnvmec_pinf("Error writing: '%s'", cli->args.cmd_output);
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
static struct xnvmec_sub g_subs[] = {
	{
		"show",
		"Print the given NVMe command-file",
		"Print the given NVMe command-file",
		sub_show,
		{
			{XNVMEC_OPT_CMD_INPUT, XNVMEC_LREQ},
		},
	},
	{
		"create",
		"Create an NVMe command-file",
		"Create an NVMe command-file",
		sub_create,
		{
			{XNVMEC_OPT_CMD_OUTPUT, XNVMEC_LREQ}, {XNVMEC_OPT_CDW00, XNVMEC_LOPT},
			{XNVMEC_OPT_CDW01, XNVMEC_LOPT},      {XNVMEC_OPT_CDW02, XNVMEC_LOPT},
			{XNVMEC_OPT_CDW03, XNVMEC_LOPT},      {XNVMEC_OPT_CDW04, XNVMEC_LOPT},
			{XNVMEC_OPT_CDW04, XNVMEC_LOPT},      {XNVMEC_OPT_CDW05, XNVMEC_LOPT},
			{XNVMEC_OPT_CDW06, XNVMEC_LOPT},      {XNVMEC_OPT_CDW07, XNVMEC_LOPT},
			{XNVMEC_OPT_CDW08, XNVMEC_LOPT},      {XNVMEC_OPT_CDW09, XNVMEC_LOPT},
			{XNVMEC_OPT_CDW10, XNVMEC_LOPT},      {XNVMEC_OPT_CDW11, XNVMEC_LOPT},
			{XNVMEC_OPT_CDW12, XNVMEC_LOPT},      {XNVMEC_OPT_CDW13, XNVMEC_LOPT},
			{XNVMEC_OPT_CDW14, XNVMEC_LOPT},      {XNVMEC_OPT_CDW15, XNVMEC_LOPT},
			{XNVMEC_OPT_OPCODE, XNVMEC_LOPT},     {XNVMEC_OPT_NSID, XNVMEC_LOPT},
		},
	},
};

static struct xnvmec g_cli = {
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
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_NONE);
}
