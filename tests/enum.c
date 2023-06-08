// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>

#define MAX_LISTINGS 1024
#define MAX_HANDLES 1024

int
enumerate_cb(struct xnvme_dev *dev, void *cb_args)
{
	struct xnvme_cli_enumeration *list = cb_args;
	const struct xnvme_ident *ident = xnvme_dev_get_ident(dev);

	if (xnvme_cli_enumeration_append(list, ident)) {
		XNVME_DEBUG("FAILED: adding ident");
	}

	return XNVME_ENUMERATE_DEV_CLOSE;
}

static int
test_enum(struct xnvme_cli *cli)
{
	struct xnvme_cli_enumeration *listing[MAX_LISTINGS] = {0};
	struct xnvme_opts opts = {0};
	uint64_t nlistings = 2;
	int nerr = 0, err;

	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}

	nlistings = XNVME_MAX(nlistings, cli->args.count);
	if (nlistings > MAX_LISTINGS) {
		xnvme_cli_pinf("--count: %ld > %d", cli->args.count, MAX_LISTINGS);
		return -EINVAL;
	}
	xnvme_cli_pinf("Will enumerate %ld times", nlistings);

	for (uint64_t i = 0; i < nlistings; ++i) {
		err = xnvme_cli_enumeration_alloc(&listing[i], 100);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_cli_enumeration_alloc()");
			return err;
		}

		err = xnvme_enumerate(cli->args.sys_uri, &opts, *enumerate_cb, listing[i]);
		if (err) {
			nerr += 1;
			xnvme_cli_perr("xnvme_enumerate()", err);
			xnvme_cli_pinf("The %ld' xnvme_enumerate() failed", i + 1);
		}

		if (i && (listing[i]->nentries != listing[i - 1]->nentries)) {
			nerr += 1;
			xnvme_cli_pinf("The enumeration %ld did not match the prev", i);

			xnvme_cli_enumeration_pr(listing[i], XNVME_PR_DEF);
			xnvme_cli_enumeration_pr(listing[i - 1], XNVME_PR_DEF);
		}
	}
	if (nerr) {
		goto exit;
	}

exit:
	printf("\n");
	if (nerr) {
		xnvme_cli_pinf("--={[ Got Errors - see details above ]}=--");
		xnvme_cli_pinf("nerr: %d", nerr);
	} else {
		xnvme_cli_pinf("LGTM: xnvme_enumerate()");
	}
	printf("\n");

	return nerr ? -errno : 0;
}

static int
test_enum_open(struct xnvme_cli *cli)
{
	struct xnvme_cli_enumeration *listing = NULL;
	struct xnvme_opts enum_opts = {0};
	int count = 1;
	int nerr = 0, err;

	err = xnvme_cli_to_opts(cli, &enum_opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}

	err = xnvme_cli_enumeration_alloc(&listing, 100);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_cli_enumeration_alloc()");
		return err;
	}

	err = xnvme_enumerate(cli->args.sys_uri, &enum_opts, *enumerate_cb, listing);
	if (err) {
		nerr += 1;
		xnvme_cli_perr("xnvme_enumerate()", err);
		goto exit;
	}
	xnvme_cli_enumeration_pr(listing, XNVME_PR_DEF);

	if (cli->args.count) {
		count = cli->args.count > MAX_HANDLES ? MAX_HANDLES : cli->args.count;
	}

	for (int64_t i = 0; i < listing->nentries; ++i) {
		struct xnvme_dev *dev[MAX_HANDLES] = {0};

		for (int hidx = 0; hidx < count; ++hidx) {
			struct xnvme_opts opts = {0};

			err = xnvme_cli_to_opts(cli, &opts);
			if (err) {
				xnvme_cli_perr("xnvme_cli_to_opts()", err);
				return err;
			}

			opts.nsid = listing->entries[i].nsid;

			dev[hidx] = xnvme_dev_open(listing->entries[i].uri, &opts);
			if (!dev[hidx]) {
				nerr += 1;
				xnvme_cli_pinf("xnvme_dev_open(%s)", listing->entries[i].uri);
				xnvme_cli_perr("xnvme_dev_open()", errno);
			}
			if (cli->args.verbose) {
				xnvme_dev_pr(dev[i], XNVME_PR_DEF);
			}
		}

		for (int hidx = 0; hidx < count; ++hidx) {
			xnvme_dev_close(dev[hidx]);
			dev[hidx] = NULL;
		}

		if (nerr) {
			goto exit;
		}
	}

exit:
	printf("\n");
	if (nerr) {
		xnvme_cli_pinf("--={[ Got Errors - see details above ]}=--");
		xnvme_cli_pinf("nerr: %d", nerr);
	} else {
		xnvme_cli_pinf("LGTM: xnvme_enumerate() + xnvme_dev_open() * count");
	}
	printf("\n");

	return nerr ? -errno : 0;
}

// This is identical to g_xnvme_be_registry
static struct xnvme_be *g_xnvme_be_test_registry[] = {
	&xnvme_be_spdk,    &xnvme_be_linux,   &xnvme_be_fbsd, &xnvme_be_macos,
	&xnvme_be_windows, &xnvme_be_ramdisk, &xnvme_be_vfio, NULL,
};

struct backend_cb_args {
	int err;
	char expected[1024];
};

int
backend_cb(struct xnvme_dev *dev, void *cb_args)
{
	struct backend_cb_args *args = cb_args;
	const struct xnvme_opts *opts = xnvme_dev_get_opts(dev);

	if (strcmp(args->expected, opts->be)) {
		xnvme_cli_pinf("FAILED: Expected backend: %s is not matching actual: %s",
			       args->expected, opts->be);
		args->err = -EIO;
	} else {
		xnvme_cli_pinf("LGTM: Expected backend: %s is matching actual: %s", args->expected,
			       opts->be);
	}

	return XNVME_ENUMERATE_DEV_CLOSE;
}

static int
test_enum_backend(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = {0};
	struct backend_cb_args cb_args = {0};
	int err;

	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}

	for (int i = 0; g_xnvme_be_test_registry[i]; ++i) {
		struct xnvme_be be = *g_xnvme_be_test_registry[i];

		if (!be.attr.enabled) {
			XNVME_DEBUG("INFO: skipping be: '%s'; !enabled", be.attr.name);
			continue;
		}

		enum xnvme_be_mixin_type mtype = XNVME_BE_DEV;

		for (uint64_t j = 0; (j < be.nobjs); ++j) {
			const struct xnvme_be_mixin *mixin = &be.objs[j];

			if (mixin->mtype != mtype) {
				continue;
			}

			be.dev = *mixin->dev;
		}

		strncpy(cb_args.expected, be.attr.name, 1023);

		err = be.dev.enumerate(cli->args.sys_uri, &opts, backend_cb, &cb_args);
		if (err) {
			XNVME_DEBUG("FAILED: %s->enumerate(...), err: '%s', i: %d",
				    g_xnvme_be_test_registry[i]->attr.name, strerror(-err), i);
		}

		if (cb_args.err) {
			return cb_args.err;
		}
	}

	return cb_args.err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"multi",
		"Call xnvme_enumerate() 'count' times",
		"Call xnvme_enumerate() 'count' times",
		test_enum,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SYS_URI, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_VERBOSE, XNVME_CLI_LFLG},
			XNVME_CLI_CORE_OPTS,
		},
	},
	{
		"open",
		"Call xnvme_enumerate() once, open each device 'count' times",
		"Call xnvme_enumerate() once, open each device 'count' times\n"
		"Dump info on each opened device with --verbose",
		test_enum_open,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SYS_URI, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_VERBOSE, XNVME_CLI_LFLG},
			XNVME_CLI_CORE_OPTS,
		},
	},
	{
		"backend",
		"Call xnvme_enumerate() and verify that the same backend is used for "
		"enum and open",
		"Call xnvme_enumerate() and verify that the same backend is used for "
		"enum and open",
		test_enum_backend,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Test xNVMe enumeration",
	.descr_short = "Test xNVMe enumeration",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
