#include <errno.h>
#include <libxnvmec.h>

static int
sub_optional(struct xnvmec *cli)
{
	int err = 0;

	xnvmec_pinf("be: '%s', given: %d", cli->args.be, cli->given[XNVMEC_OPT_BE]);
	xnvmec_pinf("mem: '%s', given: %d", cli->args.mem, cli->given[XNVMEC_OPT_MEM]);
	xnvmec_pinf("sync: '%s', given: %d", cli->args.sync, cli->given[XNVMEC_OPT_SYNC]);
	xnvmec_pinf("async: '%s', given: %d", cli->args.async, cli->given[XNVMEC_OPT_ASYNC]);
	xnvmec_pinf("admin: '%s', given: %d", cli->args.admin, cli->given[XNVMEC_OPT_ADMIN]);

	return err;
}

static struct xnvmec_sub g_subs[] = {
	{
		"optional",
		"Optional command-line arguments",
		"Optional command-line arguments",
		sub_optional,
		{
			{XNVMEC_OPT_BE, XNVMEC_LOPT},
			{XNVMEC_OPT_MEM, XNVMEC_LOPT},
			{XNVMEC_OPT_SYNC, XNVMEC_LOPT},
			{XNVMEC_OPT_ASYNC, XNVMEC_LOPT},
			{XNVMEC_OPT_ADMIN, XNVMEC_LOPT},
		},
	},
};

static struct xnvmec g_cli = {
	.title = "Exercise the diffent parsers ",
	.descr_short = "Exercise the different parsers",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_NONE);
}
