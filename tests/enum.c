// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

#define MAX_DEVS 100
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
	int err;

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
		err = xnvme_cli_enumeration_alloc(&listing[i], MAX_DEVS);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_cli_enumeration_alloc()");
			return err;
		}

		err = xnvme_enumerate(cli->args.sys_uri, &opts, *enumerate_cb, listing[i]);
		if (err) {
			xnvme_cli_perr("xnvme_enumerate()", err);
			xnvme_cli_pinf("The %ld' xnvme_enumerate() failed", i + 1);
			return err;
		}

		if (listing[i]->nentries < 1) {
			xnvme_cli_pinf("The enumeration %ld did not contain any entries", i);
			return ENOENT;
		}

		if (i && (listing[i]->nentries != listing[i - 1]->nentries)) {
			xnvme_cli_pinf("The enumeration %ld did not match the prev", i);

			xnvme_cli_enumeration_pr(listing[i], XNVME_PR_DEF);
			xnvme_cli_enumeration_pr(listing[i - 1], XNVME_PR_DEF);
			return EIO;
		}
	}
	xnvme_cli_pinf("LGTM: xnvme_enumerate()");

	return 0;
}

static int
test_enum_open(struct xnvme_cli *cli)
{
	struct xnvme_cli_enumeration *listing = NULL;
	struct xnvme_opts enum_opts = {0};
	int count = 1;
	int err;

	err = xnvme_cli_to_opts(cli, &enum_opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}

	err = xnvme_cli_enumeration_alloc(&listing, MAX_DEVS);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_cli_enumeration_alloc()");
		return err;
	}

	err = xnvme_enumerate(cli->args.sys_uri, &enum_opts, *enumerate_cb, listing);
	if (err) {
		xnvme_cli_perr("xnvme_enumerate()", err);
		return err;
	}
	xnvme_cli_enumeration_pr(listing, XNVME_PR_DEF);

	if (cli->args.count) {
		count = cli->args.count > MAX_HANDLES ? MAX_HANDLES : cli->args.count;
	}

	if (listing->nentries < 1) {
		xnvme_cli_pinf("The enumeration did not contain any entries");
		return ENOENT;
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
				err = errno;
				xnvme_cli_pinf("xnvme_dev_open(%s)", listing->entries[i].uri);
				xnvme_cli_perr("xnvme_dev_open()", err);
				return err;
			}
			if (cli->args.verbose) {
				xnvme_dev_pr(dev[i], XNVME_PR_DEF);
			}
		}

		for (int hidx = 0; hidx < count; ++hidx) {
			xnvme_dev_close(dev[hidx]);
			dev[hidx] = NULL;
		}
	}

	xnvme_cli_pinf("LGTM: xnvme_enumerate() + xnvme_dev_open() * count");
	return 0;
}

int
enumerate_keep_open_cb(struct xnvme_dev *dev, void *cb_args)
{
	struct xnvme_dev **devs = cb_args;

	for (int64_t i = 0; i < MAX_DEVS; ++i) {
		if (devs[i] == NULL) {
			devs[i] = dev;
			break;
		}
	}

	return XNVME_ENUMERATE_DEV_KEEP_OPEN;
}

static int
test_enum_keep_open(struct xnvme_cli *cli)
{
	struct xnvme_dev *devs[MAX_DEVS];
	struct xnvme_opts enum_opts = {0};
	struct xnvme_cmd_ctx ctx = {0};
	struct xnvme_spec_idfy *idfy_ctrlr = NULL;
	int err;
	int found = 0;

	for (int64_t i = 0; i < MAX_DEVS; ++i) {
		devs[i] = NULL;
	}

	err = xnvme_cli_to_opts(cli, &enum_opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}

	err = xnvme_enumerate(cli->args.sys_uri, &enum_opts, *enumerate_keep_open_cb, devs);
	if (err) {
		xnvme_cli_perr("xnvme_enumerate()", err);
		return err;
	}

	for (int64_t i = 0; i < MAX_DEVS; ++i) {
		if (devs[i] == NULL) {
			continue;
		}
		++found;
		if (cli->args.verbose) {
			xnvme_dev_pr(devs[i], XNVME_PR_DEF);
		}

		// Send a identify controller command to make sure the device really is open.
		idfy_ctrlr = xnvme_buf_alloc(devs[i], sizeof(*idfy_ctrlr));
		if (!idfy_ctrlr) {
			XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
			return -errno;
		}
		ctx = xnvme_cmd_ctx_from_dev(devs[i]);
		err = xnvme_adm_idfy_ctrlr(&ctx, idfy_ctrlr);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_adm_idfy_ctrlr()");
			return err;
		}
		xnvme_buf_free(devs[i], idfy_ctrlr);

		xnvme_dev_close(devs[i]);
	}
	if (!found) {
		xnvme_cli_pinf("No devices left open by enumeration.");
		return ENOENT;
	}

	xnvme_cli_pinf("LGTM: xnvme_enumerate() + keep_open");
	return 0;
}

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
	struct xnvme_opts opts = xnvme_opts_default();
	struct backend_cb_args cb_args = {0};
	int err;

	for (int i = 0;; ++i) {
		const struct xnvme_be_attr *attr = xnvme_be_attr_get(i);

		if (!attr) {
			break;
		}
		opts.be = attr->name;
		strncpy(cb_args.expected, attr->name, 1023);

		err = xnvme_enumerate(cli->args.sys_uri, &opts, backend_cb, &cb_args);
		if (err) {
			XNVME_DEBUG("FAILED: %s->enumerate(...), err: '%s', i: %d", attr->name,
				    strerror(-err), i);
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
		"keep_open",
		"Call xnvme_enumerate() once, keep devices open",
		"Call xnvme_enumerate() once, keep devices open\n"
		"Dump info on each opened device with --verbose",
		test_enum_keep_open,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SYS_URI, XNVME_CLI_LOPT},
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
