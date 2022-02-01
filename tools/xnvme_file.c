#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <libxnvme.h>
#include <libxnvmec.h>
#include <libxnvme_file.h>
#include <libxnvme_spec_fs.h>

#define IOSIZE_DEF 4096
#define QDEPTH_MAX 256
#define QDEPTH_DEF 16

struct cb_args {
	uint64_t nerrors;
	uint64_t ncompletions;
	uint64_t nsubmissions;
};

static void
cb_func(struct xnvme_cmd_ctx *ctx, void *cb_arg)
{
	struct cb_args *work = cb_arg;

	work->ncompletions += 1;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		xnvmec_perr("cb_func()", errno);
		xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
		work->nerrors += 1;
	}

	xnvme_queue_put_cmd_ctx(ctx->async.queue, ctx);
}

struct cb_args_copy {
	uint64_t *nerrors;
	uint64_t ncompletions;
	uint64_t nsubmissions;

	struct xnvme_cmd_ctx dst_ctx;
	char *buf;
};

static void
cb_func_copy(struct xnvme_cmd_ctx *ctx, void *cb_arg)
{
	struct cb_args_copy *work = cb_arg;

	work->ncompletions += 1;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		xnvmec_perr("cb_func_copy()", errno);
		xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
		*work->nerrors += 1;
	} else {
		ssize_t res;

		res = xnvme_file_pwrite(&work->dst_ctx, work->buf, ctx->cpl.result,
					ctx->cmd.nvm.slba);
		if (res || xnvme_cmd_ctx_cpl_status(&work->dst_ctx)) {
			xnvmec_perr("xnvme_file_pwrite(dst)", res);
			xnvme_cmd_ctx_pr(&work->dst_ctx, XNVME_PR_DEF);
			*work->nerrors += 1;
		}
	}

	xnvme_queue_put_cmd_ctx(ctx->async.queue, ctx);
}

int
read_write(struct xnvmec *cli)
{
	const char *uri = cli->args.sys_uri;
	int size = cli->args.data_nbytes;
	struct xnvme_opts opts = {.create = 1, .rdwr = 1};
	struct xnvme_cmd_ctx ctx;
	struct xnvme_dev *dev;
	ssize_t err;
	char *buf;

	xnvmec_pinf("opening '%s'\n", uri);
	dev = xnvme_file_open(uri, &opts);
	if (dev == NULL) {
		xnvmec_perr("no xnvme device. abort mission!\n", errno);
		return errno;
	}
	xnvmec_pinf("opened nvme device %s", uri);

	buf = xnvme_buf_alloc(dev, size);
	for (int u = 0; u < size; u++) {
		buf[u] = 'A';
	}

	ctx = xnvme_file_get_cmd_ctx(dev);
	err = xnvme_file_pwrite(&ctx, buf, size, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_file_pwrite()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		goto exit;
	}

	err = xnvme_file_pread(&ctx, buf, size, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_file_pread()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		goto exit;
	}

	xnvmec_pinf("Successfully wrote and read %d bytes to/from %s!", size, uri);

exit:
	xnvme_buf_free(dev, buf);
	xnvme_dev_close(dev);
	return 0;
}

int
dump_sync(struct xnvmec *cli)
{
	struct xnvme_opts opts = {.create = 1, .wronly = 1, .direct = cli->args.direct};
	struct xnvme_dev *fh;
	const char *fpath;
	size_t buf_nbytes, tbytes, iosize;
	char *buf;

	fpath = cli->args.data_output;
	iosize = cli->given[XNVMEC_OPT_IOSIZE] ? cli->args.iosize : IOSIZE_DEF;

	fh = xnvme_file_open(fpath, &opts);
	if (!fh) {
		xnvmec_perr("xnvme_file_open(fh)", errno);
		return errno;
	}
	tbytes = cli->args.data_nbytes;

	buf_nbytes = tbytes;
	buf = xnvme_buf_alloc(fh, buf_nbytes);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}
	xnvmec_buf_fill(buf, buf_nbytes, "anum");

	xnvmec_pinf("dump-sync: {fpath: %s, tbytes: %zu, buf_nbytes: %zu iosize: %zu}", fpath,
		    tbytes, buf_nbytes, iosize);

	xnvmec_timer_start(cli);

	for (size_t ofz = 0; ofz < tbytes; ofz += iosize) {
		struct xnvme_cmd_ctx ctx = xnvme_file_get_cmd_ctx(fh);
		size_t nbytes = XNVME_MIN_U64(iosize, tbytes - ofz);
		ssize_t res;

		res = xnvme_file_pwrite(&ctx, buf + ofz, nbytes, ofz);
		if (res || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvmec_perr("xnvme_file_pwrite(fh)", res);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			goto exit;
		}
	}

	xnvmec_timer_stop(cli);
	xnvmec_timer_bw_pr(cli, "wall-clock", tbytes);

exit:
	xnvme_buf_free(fh, buf);
	xnvme_file_close(fh);
	return 0;
}

int
dump_sync_iovec(struct xnvmec *cli)
{
	struct xnvme_opts opts = {.create = 1,
				  .wronly = 1,
				  .direct = cli->args.direct,
				  .be = cli->args.be,
				  .sync = cli->args.sync};
	struct xnvme_dev *fh;
	struct xnvme_cmd_ctx ctx;
	const char *fpath;
	size_t buf_nbytes, tbytes, iosize, tbytes_left;
	int npayloads, nios;
	size_t ofz = 0;
	int dvec_cnt = cli->args.vec_cnt;
	char *buf;

	fpath = cli->args.data_output;
	iosize = cli->given[XNVMEC_OPT_IOSIZE] ? cli->args.iosize : IOSIZE_DEF;

	fh = xnvme_file_open(fpath, &opts);
	if (!fh) {
		xnvmec_perr("xnvme_file_open(fh)", errno);
		return errno;
	}
	tbytes = cli->args.data_nbytes;
	tbytes_left = tbytes;

	buf_nbytes = tbytes;
	buf = xnvme_buf_alloc(fh, buf_nbytes);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}
	xnvmec_buf_fill(buf, buf_nbytes, "anum");

	xnvmec_pinf("dump-sync-iovec: {fpath: %s, tbytes: %zu, buf_nbytes: %zu iosize: %zu}",
		    fpath, tbytes, buf_nbytes, iosize);

	xnvmec_timer_start(cli);

	ctx = xnvme_file_get_cmd_ctx(fh);
	ctx.cmd.common.nsid = xnvme_dev_get_nsid(ctx.dev);
	ctx.cmd.common.opcode = XNVME_SPEC_FS_OPC_WRITE;
	ctx.cmd.nvm.slba = ofz;

	npayloads = (buf_nbytes + iosize - 1) / iosize;
	nios = (npayloads + dvec_cnt - 1) / dvec_cnt;
	for (int i = 0; i < nios; i++) {
		struct iovec dvec[dvec_cnt];
		int ndvec = XNVME_MIN_U64(dvec_cnt, (tbytes_left + iosize - 1) / iosize);
		int res;
		size_t nbytes = 0;

		for (int k = 0; k < ndvec; k++) {
			size_t iov_len = XNVME_MIN_U64(iosize, tbytes_left);
			dvec[k].iov_base = buf + ofz;
			dvec[k].iov_len = iov_len;
			ofz += iov_len;
			tbytes_left -= iov_len;
			nbytes += iov_len;
		}

		res = xnvme_cmd_passv(&ctx, dvec, ndvec, nbytes, NULL, 0, 0);
		if (res || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvmec_perr("xnvme_cmd_passv(fh)", res);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			goto exit;
		}
	}

	xnvmec_timer_stop(cli);
	xnvmec_timer_bw_pr(cli, "wall-clock", tbytes);

exit:
	xnvme_buf_free(fh, buf);
	xnvme_file_close(fh);
	return 0;
}

int
dump_async(struct xnvmec *cli)
{
	struct xnvme_opts opts = {.create = 1, .wronly = 1, .direct = cli->args.direct};
	struct xnvme_queue *queue = NULL;
	struct cb_args cb_args = {0};
	size_t buf_nbytes, tbytes, iosize;
	struct xnvme_dev *fh;
	const char *fpath;
	char *buf;
	uint32_t qdepth;
	int err;

	fpath = cli->args.data_output;
	iosize = cli->given[XNVMEC_OPT_IOSIZE] ? cli->args.iosize : IOSIZE_DEF;
	qdepth = cli->given[XNVMEC_OPT_QDEPTH] ? cli->args.qdepth : QDEPTH_DEF;

	fh = xnvme_file_open(fpath, &opts);
	if (fh == NULL) {
		xnvmec_perr("xnvme_file_open(fh)", errno);
		return errno;
	}
	tbytes = cli->args.data_nbytes;

	buf_nbytes = tbytes;
	buf = xnvme_buf_alloc(fh, buf_nbytes);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}
	xnvmec_buf_fill(buf, buf_nbytes, "anum");

	err = xnvme_queue_init(fh, qdepth, 0, &queue);
	if (err) {
		xnvmec_perr("xnvme_queue_init()", err);
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_func, &cb_args);

	xnvmec_pinf("dump-async{fpath: %s, tbytes: %zu, buf_nbytes: %zu, iosize: %zu, qdepth: %d}",
		    fpath, tbytes, buf_nbytes, iosize, qdepth);

	xnvmec_timer_start(cli);

	for (size_t ofz = 0; (ofz < tbytes) && !cb_args.nerrors;) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);
		size_t nbytes = XNVME_MIN_U64(iosize, tbytes - ofz);
		ssize_t res;

submit:
		res = xnvme_file_pwrite(ctx, buf + ofz, nbytes, ofz);
		switch (res) {
		case 0:
			cb_args.nsubmissions += 1;
			goto next;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			xnvmec_perr("submission-error", err);
			xnvme_queue_put_cmd_ctx(queue, ctx);
			goto exit;
		}

next:
		ofz += iosize;
	}

	err = xnvme_queue_drain(queue);
	if (err < 0) {
		xnvmec_perr("xnvme_queue_drain()", err);
		goto exit;
	}

	xnvmec_timer_stop(cli);
	xnvmec_timer_bw_pr(cli, "wall-clock", tbytes);

exit:
	xnvmec_pinf("cb_args: {nsubmissions: %zu, ncompletions: %zu, nerrors: %zu}",
		    cb_args.nsubmissions, cb_args.ncompletions, cb_args.nerrors);
	if (queue) {
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			xnvmec_perr("xnvme_queue_term()", err_exit);
		}
	}

	xnvme_buf_free(fh, buf);
	xnvme_file_close(fh);
	return 0;
}

int
dump_async_iovec(struct xnvmec *cli)
{
	struct xnvme_opts opts = {.create = 1,
				  .wronly = 1,
				  .direct = cli->args.direct,
				  .be = cli->args.be,
				  .async = cli->args.async};
	struct xnvme_dev *fh;
	struct xnvme_queue *queue = NULL;
	struct cb_args cb_args = {0};
	const char *fpath;
	size_t buf_nbytes, tbytes, iosize, tbytes_left;
	int npayloads, nios;
	uint32_t qdepth;
	size_t ofz = 0;
	int dvec_cnt = cli->args.vec_cnt;
	char *buf;
	int err;

	fpath = cli->args.data_output;
	iosize = cli->given[XNVMEC_OPT_IOSIZE] ? cli->args.iosize : IOSIZE_DEF;
	qdepth = cli->given[XNVMEC_OPT_QDEPTH] ? cli->args.qdepth : QDEPTH_DEF;

	fh = xnvme_file_open(fpath, &opts);
	if (!fh) {
		xnvmec_perr("xnvme_file_open(fh)", errno);
		return errno;
	}
	tbytes = cli->args.data_nbytes;
	tbytes_left = tbytes;

	buf_nbytes = tbytes;
	buf = xnvme_buf_alloc(fh, buf_nbytes);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}
	xnvmec_buf_fill(buf, buf_nbytes, "anum");

	err = xnvme_queue_init(fh, qdepth, 0, &queue);
	if (err) {
		xnvmec_perr("xnvme_queue_init()", err);
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_func, &cb_args);

	xnvmec_pinf("dump-async-iovec{fpath: %s, tbytes: %zu, buf_nbytes: %zu, iosize: %zu, "
		    "qdepth: %d}",
		    fpath, tbytes, buf_nbytes, iosize, qdepth);

	xnvmec_timer_start(cli);

	npayloads = (buf_nbytes + iosize - 1) / iosize;
	nios = (npayloads + dvec_cnt - 1) / dvec_cnt;
	for (int i = 0; i < nios; i++) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);
		struct iovec dvec[dvec_cnt];
		int ndvec = XNVME_MIN_U64(dvec_cnt, (tbytes_left + iosize - 1) / iosize);
		int res;
		size_t nbytes = 0;

		ctx->cmd.common.nsid = xnvme_dev_get_nsid(ctx->dev);
		ctx->cmd.common.opcode = XNVME_SPEC_FS_OPC_WRITE;
		ctx->cmd.nvm.slba = ofz;

		for (int k = 0; k < ndvec; k++) {
			size_t iov_len = XNVME_MIN_U64(iosize, tbytes_left);
			dvec[k].iov_base = buf + ofz;
			dvec[k].iov_len = iov_len;
			ofz += iov_len;
			tbytes_left -= iov_len;
			nbytes += iov_len;
		}

submit:
		res = xnvme_cmd_passv(ctx, dvec, ndvec, nbytes, NULL, 0, 0);
		switch (res) {
		case 0:
			cb_args.nsubmissions += 1;
			continue;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			xnvmec_perr("submission-error", err);
			xnvme_queue_put_cmd_ctx(queue, ctx);
			goto exit;
		}
	}

	xnvme_queue_wait(queue);

	xnvmec_timer_stop(cli);
	xnvmec_timer_bw_pr(cli, "wall-clock", tbytes);

exit:
	xnvme_buf_free(fh, buf);
	xnvme_file_close(fh);
	return 0;
}

int
load_sync(struct xnvmec *cli)
{
	struct xnvme_opts opts = {.rdonly = 1, .direct = cli->args.direct};
	struct xnvme_dev *fh;
	const char *fpath;
	size_t buf_nbytes, tbytes, iosize;
	char *buf;

	fpath = cli->args.data_input;
	iosize = cli->given[XNVMEC_OPT_IOSIZE] ? cli->args.iosize : IOSIZE_DEF;

	fh = xnvme_file_open(fpath, &opts);
	if (!fh) {
		xnvmec_perr("xnvme_file_open(fh)", errno);
		return errno;
	}
	tbytes = xnvme_dev_get_geo(fh)->tbytes;

	buf_nbytes = tbytes;
	buf = xnvme_buf_alloc(fh, buf_nbytes);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}
	xnvmec_buf_fill(buf, buf_nbytes, "zero");

	xnvmec_pinf("load-sync: {fpath: %s, tbytes: %zu, buf_nbytes: %zu iosize: %zu}", fpath,
		    tbytes, buf_nbytes, iosize);

	xnvmec_timer_start(cli);

	for (size_t ofz = 0; ofz < tbytes; ofz += iosize) {
		struct xnvme_cmd_ctx ctx = xnvme_file_get_cmd_ctx(fh);
		size_t nbytes = XNVME_MIN_U64(iosize, tbytes - ofz);
		ssize_t res;

		res = xnvme_file_pread(&ctx, buf + ofz, nbytes, ofz);
		if (res || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvmec_perr("xnvme_file_pread(fh)", res);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			goto exit;
		}
	}

	xnvmec_timer_stop(cli);
	xnvmec_timer_bw_pr(cli, "wall-clock", tbytes);

exit:
	xnvme_buf_free(fh, buf);
	xnvme_file_close(fh);

	return 0;
}

int
load_async(struct xnvmec *cli)
{
	const char *fpath;
	struct xnvme_opts opts = {.rdonly = 1, .direct = cli->args.direct};
	struct xnvme_dev *fh;
	size_t buf_nbytes, tbytes, iosize;
	char *buf;

	struct cb_args cb_args = {0};
	struct xnvme_queue *queue = NULL;
	uint32_t qdepth;
	int err;

	fpath = cli->args.data_input;
	iosize = cli->given[XNVMEC_OPT_IOSIZE] ? cli->args.iosize : IOSIZE_DEF;
	qdepth = cli->given[XNVMEC_OPT_QDEPTH] ? cli->args.qdepth : QDEPTH_DEF;

	fh = xnvme_file_open(fpath, &opts);
	if (fh == NULL) {
		xnvmec_perr("xnvme_file_open(fh)", errno);
		return errno;
	}
	tbytes = xnvme_dev_get_geo(fh)->tbytes;

	buf_nbytes = tbytes;
	buf = xnvme_buf_alloc(fh, buf_nbytes);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}
	xnvmec_buf_fill(buf, buf_nbytes, "zero");

	err = xnvme_queue_init(fh, qdepth, 0, &queue);
	if (err) {
		xnvmec_perr("xnvme_queue_init()", err);
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_func, &cb_args);

	xnvmec_pinf("load-async{fpath: %s, tbytes: %zu, buf_nbytes: %zu, iosize: %zu, qdepth: %d}",
		    fpath, tbytes, buf_nbytes, iosize, qdepth);

	xnvmec_timer_start(cli);

	for (size_t ofz = 0; (ofz < tbytes) && !cb_args.nerrors;) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);
		size_t nbytes = XNVME_MIN_U64(iosize, tbytes - ofz);
		ssize_t res;

submit:
		res = xnvme_file_pread(ctx, buf + ofz, nbytes, ofz);
		switch (res) {
		case 0:
			cb_args.nsubmissions += 1;
			goto next;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			xnvmec_perr("submission-error", err);
			xnvme_queue_put_cmd_ctx(queue, ctx);
			goto exit;
		}

next:
		ofz += iosize;
	}

	err = xnvme_queue_drain(queue);
	if (err < 0) {
		xnvmec_perr("xnvme_queue_drain()", err);
		goto exit;
	}

	xnvmec_timer_stop(cli);
	xnvmec_timer_bw_pr(cli, "wall-clock", tbytes);

exit:
	xnvmec_pinf("cb_args: {nsubmissions: %zu, ncompletions: %zu, nerrors: %zu}",
		    cb_args.nsubmissions, cb_args.ncompletions, cb_args.nerrors);
	if (queue) {
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			xnvmec_perr("xnvme_queue_term()", err_exit);
		}
	}

	xnvme_buf_free(fh, buf);
	xnvme_file_close(fh);

	return 0;
}

int
copy_file_sync(struct xnvmec *cli)
{
	struct xnvme_opts src_opts = {.rdonly = 1, .direct = cli->args.direct};
	struct xnvme_opts dst_opts = {.create = 1, .wronly = 1, .direct = cli->args.direct};
	struct xnvme_dev *src_fh, *dst_fh;
	const char *src_fpath, *dst_fpath;
	size_t buf_nbytes, tbytes, iosize;
	char *buf = NULL;

	src_fpath = cli->args.data_input;
	dst_fpath = cli->args.data_output;
	iosize = cli->given[XNVMEC_OPT_IOSIZE] ? cli->args.iosize : IOSIZE_DEF;

	src_fh = xnvme_file_open(src_fpath, &src_opts);
	if (src_fh == NULL) {
		xnvmec_perr("xnvme_file_open(src)", errno);
		return errno;
	}
	dst_fh = xnvme_file_open(dst_fpath, &dst_opts);
	if (dst_fh == NULL) {
		xnvmec_perr("xnvme_file_open(dst)", errno);
		goto exit;
	}
	tbytes = xnvme_dev_get_geo(src_fh)->tbytes;

	buf_nbytes = iosize;
	buf = xnvme_buf_alloc(src_fh, buf_nbytes);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}
	xnvmec_buf_fill(buf, buf_nbytes, "zero");

	xnvmec_pinf("copy-sync: {src: %s, dst: %s, tbytes: %zu, buf_nbytes: %zu, iosize: %zu",
		    src_fpath, dst_fpath, tbytes, buf_nbytes, iosize);

	xnvmec_timer_start(cli);

	for (size_t ofz = 0; ofz < tbytes; ofz += iosize) {
		struct xnvme_cmd_ctx src_ctx = xnvme_file_get_cmd_ctx(src_fh);
		struct xnvme_cmd_ctx dst_ctx = xnvme_file_get_cmd_ctx(dst_fh);
		size_t nbytes = XNVME_MIN_U64(iosize, tbytes - ofz);
		ssize_t res;

		res = xnvme_file_pread(&src_ctx, buf, nbytes, ofz);
		if (res || xnvme_cmd_ctx_cpl_status(&src_ctx)) {
			xnvmec_perr("xnvme_file_pread(src)", res);
			xnvme_cmd_ctx_pr(&src_ctx, XNVME_PR_DEF);
			goto exit;
		}

		res = xnvme_file_pwrite(&dst_ctx, buf, nbytes, ofz);
		if (res || xnvme_cmd_ctx_cpl_status(&dst_ctx)) {
			xnvmec_perr("xnvme_file_pwrite(dst)", res);
			xnvme_cmd_ctx_pr(&dst_ctx, XNVME_PR_DEF);
			goto exit;
		}
	}

	xnvmec_timer_stop(cli);
	xnvmec_timer_bw_pr(cli, "wall-clock", tbytes);

exit:
	xnvme_buf_free(src_fh, buf);
	xnvme_file_close(src_fh);
	xnvme_file_close(dst_fh);
	return 0;
}

int
copy_file_async(struct xnvmec *cli)
{
	struct xnvme_opts src_opts = {.rdonly = 1, .direct = cli->args.direct};
	struct xnvme_opts dst_opts = {.create = 1, .wronly = 1, .direct = cli->args.direct};
	struct xnvme_dev *src_fh, *dst_fh;
	const char *src_fpath, *dst_fpath;
	size_t buf_nbytes, tbytes, iosize;
	char *buf = NULL;

	struct cb_args_copy cb_args_copy[QDEPTH_MAX] = {0};
	uint64_t nerrors = 0, ncompletions = 0, nsubmissions = 0;
	struct xnvme_queue *queue = NULL;
	uint32_t qdepth;
	int err;

	src_fpath = cli->args.data_input;
	dst_fpath = cli->args.data_output;
	iosize = cli->given[XNVMEC_OPT_IOSIZE] ? cli->args.iosize : IOSIZE_DEF;
	qdepth = cli->given[XNVMEC_OPT_QDEPTH] ? cli->args.qdepth : QDEPTH_DEF;

	src_fh = xnvme_file_open(src_fpath, &src_opts);
	if (src_fh == NULL) {
		xnvmec_perr("xnvme_file_open(src)", errno);
		return errno;
	}
	dst_fh = xnvme_file_open(dst_fpath, &dst_opts);
	if (dst_fh == NULL) {
		xnvmec_perr("xnvme_file_open(dst)", errno);
		goto exit;
	}
	tbytes = xnvme_dev_get_geo(src_fh)->tbytes;

	buf_nbytes = iosize * qdepth;
	buf = xnvme_buf_alloc(src_fh, buf_nbytes);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}
	xnvmec_buf_fill(buf, buf_nbytes, "zero");

	err = xnvme_queue_init(src_fh, qdepth, 0, &queue);
	if (err) {
		xnvmec_perr("xnvme_queue_init()", err);
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_func_copy, &cb_args_copy);

	xnvmec_pinf("copy-async: "
		    "{src: %s, dst: %s, tbytes: %zu, buf_nbytes: %zu, iosize: %zu, qdepth: %u}",
		    src_fpath, dst_fpath, tbytes, buf_nbytes, iosize, qdepth);

	xnvmec_timer_start(cli);

	for (uint32_t i = 0; i < qdepth; ++i) {
		struct cb_args_copy *work = &cb_args_copy[i];

		work->nerrors = &nerrors;
		work->buf = buf + (i * iosize);
		work->dst_ctx = xnvme_file_get_cmd_ctx(dst_fh);
	}

	for (size_t ofz = 0; (ofz < tbytes) && !nerrors;) {
		struct cb_args_copy *work = &cb_args_copy[(ofz / iosize) % qdepth];
		struct xnvme_cmd_ctx *src_ctx = xnvme_cmd_ctx_from_queue(queue);
		size_t nbytes = XNVME_MIN_U64(iosize, tbytes - ofz);
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
			xnvmec_perr("submission-error", err);
			xnvme_queue_put_cmd_ctx(queue, src_ctx);
			goto exit;
		}

next:
		ofz += iosize;
	}

	err = xnvme_queue_drain(queue);
	if (err < 0) {
		xnvmec_perr("xnvme_queue_drain()", err);
		goto exit;
	}

	xnvmec_timer_stop(cli);
	xnvmec_timer_bw_pr(cli, "wall-clock", tbytes);

exit:
	for (uint32_t i = 0; i < qdepth; ++i) {
		struct cb_args_copy *work = &cb_args_copy[i];

		nsubmissions += work->nsubmissions;
		ncompletions += work->ncompletions;
	}
	xnvmec_pinf("cb_args_copy: {nsubmissions: %zu, ncompletions: %zu, nerrors: %zu}",
		    nsubmissions, ncompletions, nerrors);

	if (queue) {
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			xnvmec_perr("xnvme_queue_term()", err_exit);
		}
	}

	xnvme_buf_free(src_fh, buf);
	xnvme_file_close(src_fh);
	xnvme_file_close(dst_fh);
	return 0;
}

static struct xnvmec_sub g_subs[] = {
	{
		"write-read",
		"Write and read a file",
		"Write and read a file",
		read_write,
		{
			{XNVMEC_OPT_SYS_URI, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_NBYTES, XNVMEC_POSA},
		},
	},
	{
		"dump-sync",
		"Write a buffer of 'data-nbytes' to file",
		"Write a buffer of 'data-nbytes' to file",
		dump_sync,
		{
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_NBYTES, XNVMEC_LREQ},
			{XNVMEC_OPT_IOSIZE, XNVMEC_LOPT},
			{XNVMEC_OPT_DIRECT, XNVMEC_LFLG},
		},
	},
	{
		"dump-sync-iovec",
		"Write a buffer of 'data-nbytes' to file using vectored ios",
		"Write a buffer of 'data-nbytes' to file",
		dump_sync_iovec,
		{
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_NBYTES, XNVMEC_LREQ},
			{XNVMEC_OPT_IOSIZE, XNVMEC_LOPT},
			{XNVMEC_OPT_DIRECT, XNVMEC_LFLG},
			{XNVMEC_OPT_SYNC, XNVMEC_LOPT},
			{XNVMEC_OPT_BE, XNVMEC_LOPT},
			{XNVMEC_OPT_VEC_CNT, XNVMEC_LOPT},
		},
	},
	{
		"dump-async",
		"Write a buffer of 'data-nbytes' to file --data-output",
		"Write a buffer of 'data-nbytes' to file --data-output",
		dump_async,
		{
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_NBYTES, XNVMEC_LREQ},
			{XNVMEC_OPT_IOSIZE, XNVMEC_LOPT},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
			{XNVMEC_OPT_DIRECT, XNVMEC_LFLG},
		},
	},
	{
		"dump-async-iovec",
		"Write a buffer of 'data-nbytes' to file --data-output using vectored ios",
		"Write a buffer of 'data-nbytes' to file --data-output",
		dump_async_iovec,
		{
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_NBYTES, XNVMEC_LREQ},
			{XNVMEC_OPT_IOSIZE, XNVMEC_LOPT},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
			{XNVMEC_OPT_DIRECT, XNVMEC_LFLG},
			{XNVMEC_OPT_ASYNC, XNVMEC_LOPT},
			{XNVMEC_OPT_BE, XNVMEC_LOPT},
			{XNVMEC_OPT_VEC_CNT, XNVMEC_LOPT},
		},
	},
	{
		"load-sync",
		"Read the entire file into memory",
		"Read the entire file into memory",
		load_sync,
		{
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_POSA},
			{XNVMEC_OPT_IOSIZE, XNVMEC_LOPT},
			{XNVMEC_OPT_DIRECT, XNVMEC_LFLG},
		},
	},
	{
		"load-async",
		"Read the entire file into memory",
		"Read the entire file into memory",
		load_async,
		{
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_POSA},
			{XNVMEC_OPT_IOSIZE, XNVMEC_LOPT},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
			{XNVMEC_OPT_DIRECT, XNVMEC_LFLG},
		},
	},
	{
		"copy-sync",
		"Copy file --data-input to --data--output",
		"Copy file --data-input to --data--output",
		copy_file_sync,
		{
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_POSA},
			{XNVMEC_OPT_IOSIZE, XNVMEC_LOPT},
			{XNVMEC_OPT_DIRECT, XNVMEC_LFLG},
		},
	},
	{
		"copy-async",
		"Copy file -- asynchronously",
		"Copy file",
		copy_file_async,
		{
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_POSA},
			{XNVMEC_OPT_IOSIZE, XNVMEC_LOPT},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
			{XNVMEC_OPT_DIRECT, XNVMEC_LFLG},
		},
	},
};

static struct xnvmec g_cli = {
	.title = "xNVMe file - Exercise the xnvme_file API",
	.descr_short = "Exercise the xnvme_file API",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_NONE);
}
