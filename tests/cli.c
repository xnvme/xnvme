// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <libxnvme.h>

static int
sub_optional(struct xnvme_cli *cli)
{
	int err = 0;

	xnvme_cli_pinf("be: '%s', given: %d", cli->args.be, cli->given[XNVME_CLI_OPT_BE]);
	xnvme_cli_pinf("mem: '%s', given: %d", cli->args.mem, cli->given[XNVME_CLI_OPT_MEM]);
	xnvme_cli_pinf("sync: '%s', given: %d", cli->args.sync, cli->given[XNVME_CLI_OPT_SYNC]);
	xnvme_cli_pinf("async: '%s', given: %d", cli->args.async, cli->given[XNVME_CLI_OPT_ASYNC]);
	xnvme_cli_pinf("admin: '%s', given: %d", cli->args.admin, cli->given[XNVME_CLI_OPT_ADMIN]);

	return err;
}

static int
sub_cpuids(struct xnvme_cli *cli)
{
	int err = 0;

	xnvme_cli_pinf("cpumask: '%s', given: %d", cli->args.cpumask ? cli->args.cpumask : "NULL",
		       cli->given[XNVME_CLI_OPT_CPUMASK]);
	xnvme_cli_pinf("cpulist: '%s', given: %d", cli->args.cpulist ? cli->args.cpulist : "NULL",
		       cli->given[XNVME_CLI_OPT_CPULIST]);

	xnvme_cli_pinf("ncpus: %d", cli->args.ncpus);
	for (int i = 0; i < cli->args.ncpus; i++) {
		xnvme_cli_pinf("cpus[%d]: %d", i, cli->args.cpus[i]);
	}

	return err;
}

/**
 * Expected values for the 'numeric' sub-command; these must match the arguments given by the
 * test-cases registered in 'tests/meson.build'
 */
#define EXPECTED_NQUEUES 4
#define EXPECTED_DEV_ID 7
#define EXPECTED_MAX_IO_BYTES 131072

static int
check_num(const char *name, int given, uint64_t val, uint64_t expected)
{
	if (!given) {
		if (val) {
			xnvme_cli_pinf("FAILED: %s not given, yet val: %" PRIu64, name, val);
			return -EINVAL;
		}
		return 0;
	}

	if (val != expected) {
		xnvme_cli_pinf("FAILED: %s val: %" PRIu64 ", expected: %" PRIu64, name, val,
			       expected);
		return -EINVAL;
	}

	return 0;
}

/**
 * Verify that numeric options make it all the way into 'xnvme_cli_args'; an option missing its
 * assignment in the option-to-args switch parses fine but silently drops the value
 */
static int
sub_numeric(struct xnvme_cli *cli)
{
	int err = 0;

	err |= check_num("nqueues", cli->given[XNVME_CLI_OPT_NQUEUES], cli->args.nqueues,
			 EXPECTED_NQUEUES);
	err |= check_num("dev-id", cli->given[XNVME_CLI_OPT_DEV_ID], cli->args.dev_id,
			 EXPECTED_DEV_ID);
	err |= check_num("max-io-bytes", cli->given[XNVME_CLI_OPT_MAX_IO_BYTES],
			 cli->args.max_io_bytes, EXPECTED_MAX_IO_BYTES);

	return err;
}

/**
 * The device keeps its own copy of the string options it is opened with: xnvme_dev_get_opts()
 * returns the same strings after the caller's options, and the strings they were filled from,
 * are overwritten
 */
static int
sub_opts(struct xnvme_cli *cli)
{
	const char *given[] = {
		cli->args.core_mask, cli->args.iova_mode, cli->args.adrfam,
		cli->args.subnqn,    cli->args.hostnqn,
	};
	char want[sizeof(given) / sizeof(*given)][XNVME_OPTS_NQN_LEN];
	struct xnvme_opts opts = {0};
	const struct xnvme_opts *got;
	struct xnvme_dev *dev;
	int err;

	for (size_t i = 0; i < sizeof(given) / sizeof(*given); ++i) {
		snprintf(want[i], sizeof(want[i]), "%s", given[i] ? given[i] : "");
	}

	xnvme_opts_set_defaults(&opts);
	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}
	err = xnvme_dev_open_uri(cli->args.uri, &opts, &dev);
	if (err) {
		xnvme_cli_perr("xnvme_dev_open_uri()", err);
		return err;
	}

	memset(&opts, 'x', sizeof(opts));
	for (size_t i = 0; i < sizeof(given) / sizeof(*given); ++i) {
		if (given[i]) {
			memset((char *)given[i], 'x', strlen(given[i]));
		}
	}

	got = xnvme_dev_get_opts(dev);
	if (strcmp(got->core_mask, want[0]) || strcmp(got->iova_mode, want[1]) ||
	    strcmp(got->adrfam, want[2]) || strcmp(got->subnqn, want[3]) ||
	    strcmp(got->hostnqn, want[4])) {
		xnvme_cli_pinf("FAILED: the device's options are not those it was opened with");
		xnvme_opts_pr(got, XNVME_PR_DEF);
		err = -EINVAL;
	}
	xnvme_dev_close(dev);

	return err;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"optional",
		"Optional command-line arguments",
		"Optional command-line arguments",
		sub_optional,
		{
			XNVME_CLI_ASYNC_OPTS,
		},
	},
	{
		"cpu-ids",
		"Command-line arguments for pinning CPUs",
		"Command-line arguments for pinning CPUs",
		sub_cpuids,
		{
			{XNVME_CLI_OPT_CPUMASK, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CPULIST, XNVME_CLI_LOPT},
		},
	},
	{
		"numeric",
		"Numeric command-line arguments",
		"Numeric command-line arguments",
		sub_numeric,
		{
			{XNVME_CLI_OPT_NQUEUES, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DEV_ID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_MAX_IO_BYTES, XNVME_CLI_LOPT},
		},
	},
	{
		"opts",
		"String options kept by the device",
		"Open 'uri' with the given string options and check that the device keeps them",
		sub_opts,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_BE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_CORE_MASK, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_IOVA_MODE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ADRFAM, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_SUBNQN, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_HOSTNQN, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Exercise the diffent parsers ",
	.descr_short = "Exercise the different parsers",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
