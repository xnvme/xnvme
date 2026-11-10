// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libxnvme.h>

XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cpl) == sizeof(struct xnvme_spec_cqe),
		    "struct xnvme_spec_cpl names struct xnvme_spec_cqe")

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
		struct xnvme_cmd_ctx vctx = want;
		struct iovec mvec[2] = {0};
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

		if (xnvme_cmd_passv(&vctx, NULL, 0, 0, mvec, 2, 0) != -EINVAL) {
			xnvme_cli_pinf("FAILED: xnvme_cmd_passv(mvec_cnt: 2) != -EINVAL");
			err = -EINVAL;
		}
	}

	{
		union xnvme_spec_sqe sqe = {0};
		FILE *stream = tmpfile();

		if (!stream) {
			xnvme_cli_perr("tmpfile()", -errno);
			return -errno;
		}
		if (xnvme_spec_cmd_fpr(stream, &sqe, XNVME_PR_DEF) !=
		    xnvme_spec_sqe_fpr(stream, &sqe, XNVME_PR_DEF)) {
			xnvme_cli_pinf("FAILED: xnvme_spec_cmd_fpr() != xnvme_spec_sqe_fpr()");
			err = -EINVAL;
		}
		fclose(stream);
	}

	return err;
}

static int
sub_deprecated_znd(struct xnvme_cli *XNVME_UNUSED(cli))
{
	const uint32_t nentries = 2;
	struct xnvme_spec_znd_descr *descr;
	struct xnvme_znd_report *report;
	int err = 0;

	report = calloc(1, sizeof(*report) + nentries * sizeof(*descr));
	if (!report) {
		err = -errno;
		xnvme_cli_perr("calloc()", err);
		return err;
	}
	report->nentries = nentries;
	report->zd_nbytes = sizeof(*descr);
	report->zrent_nbytes = sizeof(*descr);

	for (uint32_t nth = 0; nth < nentries; ++nth) {
		err = xnvme_znd_report_get_descr(report, nth, &descr, NULL);
		if (err) {
			xnvme_cli_perr("xnvme_znd_report_get_descr()", err);
			break;
		}
		if (XNVME_ZND_REPORT_DESCR(report, nth) != descr) {
			xnvme_cli_pinf("FAILED: XNVME_ZND_REPORT_DESCR() != "
				       "xnvme_znd_report_get_descr()");
			err = -EINVAL;
			break;
		}
	}

	free(report);

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
	if (xnvme_dev_open_uri("2MB", NULL, NULL) != -EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_dev_open_uri(uri, opts, NULL) != -EINVAL");
		err = -EINVAL;
	}
	if (xnvme_dev_open_uri(NULL, NULL, &dev) != -EINVAL || dev) {
		xnvme_cli_pinf("FAILED: xnvme_dev_open_uri(NULL, opts, dev) != -EINVAL");
		err = -EINVAL;
	}
	errno = 0;
	if (xnvme_dev_open(NULL, NULL) || errno != EINVAL) {
		xnvme_cli_pinf("FAILED: xnvme_dev_open(NULL, opts) != NULL with errno EINVAL");
		err = -EINVAL;
	}

	return err;
}

static void
cb_put_ctx(struct xnvme_cmd_ctx *ctx, void *XNVME_UNUSED(cb_arg))
{
	xnvme_queue_put_cmd_ctx(ctx->async.queue, ctx);
}

static int
sub_deprecated_queue(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = {0};
	struct xnvme_queue *queue = NULL;
	struct xnvme_cmd_ctx *ctx;
	struct xnvme_dev *dev;
	void *buf = NULL;
	int err;

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
	err = xnvme_queue_init(dev, 2, 0, &queue);
	if (err) {
		xnvme_cli_perr("xnvme_queue_init()", err);
		goto exit;
	}

	for (int i = 0; i < 2; ++i) {
		ctx = xnvme_cmd_ctx_from_queue(queue);
		if (!ctx || ctx->dev != dev) {
			xnvme_cli_pinf(
				"FAILED: xnvme_cmd_ctx_from_queue() != xnvme_queue_get_cmd_ctx()");
			err = -EINVAL;
			break;
		}
		xnvme_queue_put_cmd_ctx(queue, ctx);
	}

	if (err) {
		goto exit;
	}

	buf = xnvme_buf_alloc(dev, xnvme_dev_get_geo(dev)->lba_nbytes);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_put_ctx, NULL);

	ctx = xnvme_queue_get_cmd_ctx(queue);
	err = xnvme_nvm_read(ctx, xnvme_dev_get_nsid(dev), 0, 0, buf, NULL);
	if (err) {
		xnvme_cli_perr("xnvme_nvm_read()", err);
		xnvme_queue_put_cmd_ctx(queue, ctx);
		goto exit;
	}

	err = xnvme_queue_wait(queue);
	if (err != 1 || xnvme_queue_get_outstanding(queue)) {
		xnvme_cli_pinf("FAILED: xnvme_queue_wait(): %d, outstanding: %u", err,
			       xnvme_queue_get_outstanding(queue));
		err = -EIO;
		goto exit;
	}
	err = 0;

exit:
	if (queue) {
		xnvme_queue_term(queue);
	}
	if (buf) {
		xnvme_buf_free(dev, buf);
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
		err = -errno;
		xnvme_cli_perr("xnvme_dev_open()", err);
		return err;
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
		"deprecated-znd",
		"Use the deprecated macro reading a zone report",
		"Use the deprecated macro reading a zone report",
		sub_deprecated_znd,
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
