// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

#define IOSIZE_DEF 4096
#define QDEPTH_MAX 256
#define QDEPTH_DEF 16
#define START_OFFSET_DEF 0

struct cb_args {
	uint64_t *nerrors;
	uint64_t ncompletions;
	uint64_t nsubmissions;
	uint64_t start_offset;

	struct xnvme_cmd_ctx dst_ctx;
	char *buf;
};

static void
cb_func(struct xnvme_cmd_ctx *ctx, void *cb_arg)
{
	struct cb_args *work = cb_arg;

	work->ncompletions += 1;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		xnvme_cli_perr("cb_func()", errno);
		xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
		*work->nerrors += 1;
	} else {
		ssize_t res;

		res = xnvme_file_pwrite(&work->dst_ctx, work->buf, ctx->cpl.result,
					ctx->cmd.nvm.slba - work->start_offset);
		if (res || xnvme_cmd_ctx_cpl_status(&work->dst_ctx)) {
			xnvme_cli_perr("xnvme_file_pwrite(dst)", res);
			xnvme_cmd_ctx_pr(&work->dst_ctx, XNVME_PR_DEF);
			*work->nerrors += 1;
		}
	}

	xnvme_queue_put_cmd_ctx(ctx->async.queue, ctx);
}

int
copy_async(struct xnvme_cli *cli)
{
	const char *src_uri, *dst_uri;
	struct xnvme_dev *src_dev, *dst_dev;
	struct xnvme_opts src_opts = {.rdonly = 1, .direct = cli->args.direct};
	struct xnvme_opts dst_opts = {.wronly = 1, .create = 1, .direct = cli->args.direct};
	size_t buf_nbytes, tbytes, iosize, start_offset;
	char *buf = NULL;

	struct cb_args cb_args[QDEPTH_MAX] = {0};
	uint64_t nerrors = 0, ncompletions = 0, nsubmissions = 0;
	struct xnvme_queue *queue = NULL;
	uint32_t qdepth;
	int err;

	src_uri = cli->args.data_input;
	dst_uri = cli->args.data_output;
	tbytes = cli->args.data_nbytes;

	iosize = cli->given[XNVME_CLI_OPT_IOSIZE] ? cli->args.iosize : IOSIZE_DEF;
	if (!iosize) {
		err = -EINVAL;
		xnvme_cli_perr("iosize can't be set to 0", err);
		return err;
	}

	qdepth = cli->given[XNVME_CLI_OPT_QDEPTH] ? cli->args.qdepth : QDEPTH_DEF;
	if (!qdepth) {
		err = -EINVAL;
		xnvme_cli_perr("qdepth can't be set to 0", err);
		return err;
	}
	start_offset = cli->given[XNVME_CLI_OPT_OFFSET] ? cli->args.offset : START_OFFSET_DEF;

	src_dev = xnvme_file_open(src_uri, &src_opts);
	if (src_dev == NULL) {
		xnvme_cli_perr("xnvme_file_open(src)", errno);
		return errno;
	}
	dst_dev = xnvme_file_open(dst_uri, &dst_opts);
	if (dst_dev == NULL) {
		xnvme_cli_perr("xnvme_file_open(dst)", errno);
		xnvme_file_close(src_dev);
		return errno;
	}

	buf_nbytes = iosize * qdepth;
	buf = xnvme_buf_alloc(dst_dev, buf_nbytes);
	if (!buf) {
		xnvme_cli_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}
	xnvme_buf_fill(buf, buf_nbytes, "zero");

	err = xnvme_queue_init(src_dev, qdepth, 0, &queue);
	if (err) {
		xnvme_cli_perr("xnvme_queue_init()", err);
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_func, &cb_args);

	xnvme_cli_pinf("copy-async: "
		       "{src: %s, dst: %s, tbytes: %zu, buf_nbytes: %zu, iosize: %zu, qdepth: %u, "
		       "start_offset: %zu}",
		       src_uri, dst_uri, tbytes, buf_nbytes, iosize, qdepth, start_offset);

	xnvme_cli_timer_start(cli);

	for (uint32_t i = 0; i < qdepth; ++i) {
		struct cb_args *work = &cb_args[i];

		work->nerrors = &nerrors;
		work->buf = buf + (i * iosize);
		work->dst_ctx = xnvme_file_get_cmd_ctx(dst_dev);
		work->start_offset = start_offset;
	}
	for (size_t ofz = start_offset; (ofz - start_offset < tbytes) && !nerrors;) {
		struct cb_args *work = &cb_args[((ofz - start_offset) / iosize) % qdepth];
		struct xnvme_cmd_ctx *src_ctx = xnvme_cmd_ctx_from_queue(queue);
		size_t nbytes = XNVME_MIN_U64(iosize, tbytes - (ofz - start_offset));
		ssize_t res;

		src_ctx->async.cb_arg = work;

submit:
		res = xnvme_file_pread(src_ctx, work->buf, nbytes, ofz);
		switch (res) {
		case 0:
			work->nsubmissions += 1;
			goto next;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			xnvme_cli_perr("submission-error", err);
			xnvme_queue_put_cmd_ctx(queue, src_ctx);
			goto exit;
		}

next:
		ofz += iosize;
	}

	err = xnvme_queue_drain(queue);
	if (err < 0) {
		xnvme_cli_perr("xnvme_queue_drain()", err);
		goto exit;
	}

	xnvme_cli_timer_stop(cli);
	xnvme_cli_timer_bw_pr(cli, "wall-clock", tbytes);

exit:
	for (uint32_t i = 0; i < qdepth; ++i) {
		struct cb_args *work = &cb_args[i];

		nsubmissions += work->nsubmissions;
		ncompletions += work->ncompletions;
	}
	xnvme_cli_pinf("cb_args: {nsubmissions: %zu, ncompletions: %zu, nerrors: %zu}",
		       nsubmissions, ncompletions, nerrors);

	if (queue) {
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			xnvme_cli_perr("xnvme_queue_term()", err_exit);
		}
	}

	xnvme_buf_free(dst_dev, buf);
	xnvme_file_close(src_dev);
	xnvme_file_close(dst_dev);
	return 0;
}

int
copy_sync(struct xnvme_cli *cli)
{
	const char *src_uri, *dst_uri;
	struct xnvme_dev *src_dev, *dst_dev;
	struct xnvme_opts src_opts = {.rdonly = 1, .direct = cli->args.direct};
	struct xnvme_opts dst_opts = {.wronly = 1, .create = 1, .direct = cli->args.direct};
	size_t buf_nbytes, tbytes, iosize, start_offset;
	char *buf = NULL;

	src_uri = cli->args.data_input;
	dst_uri = cli->args.data_output;
	tbytes = cli->args.data_nbytes;

	iosize = cli->given[XNVME_CLI_OPT_IOSIZE] ? cli->args.iosize : IOSIZE_DEF;
	start_offset = cli->given[XNVME_CLI_OPT_OFFSET] ? cli->args.offset : START_OFFSET_DEF;

	src_dev = xnvme_file_open(src_uri, &src_opts);
	if (!src_dev) {
		xnvme_cli_perr("xnvme_file_open(src)", errno);
		return errno;
	}
	dst_dev = xnvme_file_open(dst_uri, &dst_opts);
	if (!dst_dev) {
		xnvme_cli_perr("xnvme_file_open(dst)", errno);
		xnvme_file_close(src_dev);
		return errno;
	}

	buf_nbytes = iosize;
	buf = xnvme_buf_alloc(dst_dev, buf_nbytes);
	if (!buf) {
		xnvme_cli_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}
	xnvme_buf_fill(buf, buf_nbytes, "zero");

	xnvme_cli_pinf("copy-sync: {src: %s, dst: %s, tbytes: %zu, buf_nbytes: %zu, iosize: %zu, "
		       "start_offset: %zu}",
		       src_uri, dst_uri, tbytes, buf_nbytes, iosize, start_offset);

	xnvme_cli_timer_start(cli);

	for (size_t ofz = start_offset; ofz - start_offset < tbytes; ofz += iosize) {
		struct xnvme_cmd_ctx src_ctx = xnvme_file_get_cmd_ctx(src_dev);
		struct xnvme_cmd_ctx dst_ctx = xnvme_file_get_cmd_ctx(dst_dev);
		size_t nbytes = XNVME_MIN_U64(iosize, tbytes - (ofz - start_offset));
		ssize_t res;

		res = xnvme_file_pread(&src_ctx, buf, nbytes, ofz);
		if (res || xnvme_cmd_ctx_cpl_status(&src_ctx)) {
			xnvme_cli_perr("xnvme_file_pread(src)", res);
			xnvme_cmd_ctx_pr(&src_ctx, XNVME_PR_DEF);
			goto exit;
		}

		res = xnvme_file_pwrite(&dst_ctx, buf, nbytes, ofz - start_offset);
		if (res || xnvme_cmd_ctx_cpl_status(&dst_ctx)) {
			xnvme_cli_perr("xnvme_file_pwrite(dst)", res);
			xnvme_cmd_ctx_pr(&dst_ctx, XNVME_PR_DEF);
			goto exit;
		}
	}

	xnvme_cli_timer_stop(cli);
	xnvme_cli_timer_bw_pr(cli, "wall-clock", tbytes);

exit:
	xnvme_buf_free(dst_dev, buf);
	xnvme_file_close(src_dev);
	xnvme_file_close(dst_dev);
	return 0;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"sync",
		"Copy --data-nbytes from --data-input to --data--output",
		"Copy --data-nbytes from --data-input to --data--output",
		copy_sync,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_DATA_INPUT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_DATA_NBYTES, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_IOSIZE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DIRECT, XNVME_CLI_LFLG},
			{XNVME_CLI_OPT_OFFSET, XNVME_CLI_LOPT},
		},
	},
	{
		"async",
		"Copy --data-nbytes from --data-input to --data--output",
		"Copy --data-nbytes from --data-input to --data--output",
		copy_async,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_DATA_INPUT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_DATA_NBYTES, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_IOSIZE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_QDEPTH, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DIRECT, XNVME_CLI_LFLG},
			{XNVME_CLI_OPT_OFFSET, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "xNVMe dd - Copy bytes from input to output",
	.descr_short = "Copy bytes from input to output",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
