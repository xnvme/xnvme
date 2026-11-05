// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <string.h>
#include <libxnvme.h>

static int
sub_deprecated(struct xnvme_cli *XNVME_UNUSED(cli))
{
	struct xnvme_dev *dev = (struct xnvme_dev *)&dev; // Any handle; it is only stored
	struct xnvme_opts expected = {0};
	struct xnvme_cmd_ctx want = {0};
	int err = 0;

	xnvme_opts_set_defaults(&expected);
	xnvme_dev_get_cmd_ctx(dev, &want);

	for (int i = 0; i < 2; ++i) {
		struct xnvme_opts opts = xnvme_opts_default();
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		struct xnvme_cmd_ctx fctx = xnvme_file_get_cmd_ctx(dev);

		if (memcmp(&opts, &expected, sizeof(opts))) {
			xnvme_cli_pinf(
				"FAILED: xnvme_opts_default() != xnvme_opts_set_defaults()");
			err = -EINVAL;
		}
		if (ctx.dev != want.dev || ctx.opts != want.opts) {
			xnvme_cli_pinf(
				"FAILED: xnvme_cmd_ctx_from_dev() != xnvme_dev_get_cmd_ctx()");
			err = -EINVAL;
		}
		if (fctx.dev != want.dev || fctx.opts != want.opts) {
			xnvme_cli_pinf(
				"FAILED: xnvme_file_get_cmd_ctx() != xnvme_dev_get_cmd_ctx()");
			err = -EINVAL;
		}
	}

	return err;
}

static int
sub_null_args(struct xnvme_cli *XNVME_UNUSED(cli))
{
	struct xnvme_dev *dev = (struct xnvme_dev *)&dev; // Any handle; it is only stored
	struct xnvme_cmd_ctx ctx = {0};
	int err = 0;

	if (xnvme_dev_get_cmd_ctx(NULL, &ctx) != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_dev_get_cmd_ctx(NULL, ctx) != -EINVAL");
		err = -EINVAL;
	}
	if (xnvme_dev_get_cmd_ctx(dev, NULL) != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_dev_get_cmd_ctx(dev, NULL) != -EINVAL");
		err = -EINVAL;
	}
	if (xnvme_dev_get_cmd_ctx(dev, &ctx) || ctx.dev != dev) {
		xnvme_cli_pinf("FAILED: xnvme_dev_get_cmd_ctx(dev, ctx)");
		err = -EINVAL;
	}
	if (xnvme_opts_set_defaults(NULL) != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_opts_set_defaults(NULL) != -EINVAL");
		err = -EINVAL;
	}

	return err;
}

static int
sub_deprecated_queue(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = {0};
	struct xnvme_queue *queue = NULL;
	struct xnvme_dev *dev;
	int err;

	xnvme_opts_set_defaults(&opts);
	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}
	dev = xnvme_dev_open(cli->args.uri, &opts);
	if (!dev) {
		xnvme_cli_perr("xnvme_dev_open()", -errno);
		return -errno;
	}
	err = xnvme_queue_init(dev, 2, 0, &queue);
	if (err) {
		xnvme_cli_perr("xnvme_queue_init()", err);
		goto exit;
	}

	for (int i = 0; i < 2; ++i) {
		struct xnvme_cmd_ctx *ctx = xnvme_cmd_ctx_from_queue(queue);

		if (!ctx || ctx->dev != dev) {
			xnvme_cli_pinf(
				"FAILED: xnvme_cmd_ctx_from_queue() != xnvme_queue_get_cmd_ctx()");
			err = -EINVAL;
			break;
		}
		xnvme_queue_put_cmd_ctx(queue, ctx);
	}

exit:
	if (queue) {
		xnvme_queue_term(queue);
	}
	xnvme_dev_close(dev);

	return err;
}

static int
sub_deprecated_mem(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = {0};
	struct xnvme_dev *dev;
	size_t align;
	void *buf;
	int err;

	xnvme_opts_set_defaults(&opts);
	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}
	dev = xnvme_dev_open(cli->args.uri, &opts);
	if (!dev) {
		xnvme_cli_perr("xnvme_dev_open()", -errno);
		return -errno;
	}

	err = xnvme_buf_get_alignment(dev, &align);
	if (err) {
		xnvme_cli_perr("xnvme_buf_get_alignment()", err);
		goto exit;
	}
	buf = xnvme_buf_virt_alloc(align, align);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_virt_alloc()", err);
		goto exit;
	}

	err = xnvme_mem_map(dev, buf, align);
	if (err) {
		xnvme_cli_pinf("FAILED: xnvme_mem_map() != xnvme_buf_register(), err: %d", err);
	} else {
		err = xnvme_mem_unmap(dev, buf);
		if (err) {
			xnvme_cli_pinf(
				"FAILED: xnvme_mem_unmap() != xnvme_buf_unregister(), err: %d",
				err);
		}
	}

	xnvme_buf_virt_free(buf);

exit:
	xnvme_dev_close(dev);

	return err;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"deprecated",
		"Call the deprecated functions",
		"Call the deprecated functions",
		sub_deprecated,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
	{
		"null-args",
		"Pass NULL to the functions filling an output argument",
		"Pass NULL to the functions filling an output argument",
		sub_null_args,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
	{
		"deprecated-queue",
		"Call the deprecated functions taking a queue",
		"Call the deprecated functions taking a queue",
		sub_deprecated_queue,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			XNVME_CLI_ASYNC_OPTS,
		},
	},
	{
		"deprecated-mem",
		"Call the deprecated functions registering memory",
		"Call the deprecated functions registering memory",
		sub_deprecated_mem,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			XNVME_CLI_ADMIN_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Exercise the deprecated functions of libxnvme_compat.h",
	.descr_short = "Exercise the deprecated functions of libxnvme_compat.h",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
