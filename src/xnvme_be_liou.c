// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#define _GNU_SOURCE
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#define XNVME_BE_LIOU_BID 0xBEAF
#define XNVME_BE_LIOU_NAME "LIOU"

int
xnvme_be_liou_ident_from_uri(const char *uri, struct xnvme_ident *ident)
{
	char tail[XNVME_IDENT_URI_LEN] = { 0 };
	char uri_prefix[10] = { 0 };
	int uri_has_prefix = 0;
	uint32_t cntid, nsid;
	int matches;

	uri_has_prefix = uri_parse_prefix(uri, uri_prefix) == 0;
	if (uri_has_prefix && strncmp(uri_prefix, "liou", 4)) {
		errno = EINVAL;
		return -1;
	}

	strncpy(ident->uri, uri, XNVME_IDENT_URI_LEN);
	ident->bid = XNVME_BE_LIOU_BID;

	matches = sscanf(uri, uri_has_prefix ? \
			 "liou://" XNVME_LINUX_NS_SCAN : \
			 XNVME_LINUX_NS_SCAN,
			 &cntid, &nsid, tail);
	if (matches == 2) {
		ident->type = XNVME_IDENT_NS;
		ident->nsid = nsid;
		ident->nst = XNVME_SPEC_NSTYPE_NOCHECK;
		snprintf(ident->be_uri, XNVME_IDENT_URI_LEN,
			 XNVME_LINUX_NS_FMT, cntid, nsid);
		return 0;
	}

	matches = sscanf(uri, uri_has_prefix ? \
			 "liou://" XNVME_LINUX_CTRLR_SCAN : \
			 XNVME_LINUX_CTRLR_SCAN,
			 &cntid, tail);
	if (matches == 1) {
		ident->type = XNVME_IDENT_CTRLR;
		snprintf(ident->be_uri, XNVME_IDENT_URI_LEN,
			 XNVME_LINUX_CTRLR_FMT, cntid);
		return 0;
	}

	errno = EINVAL;
	return -1;
}

#ifndef XNVME_BE_LIOU_ENABLED
struct xnvme_be xnvme_be_liou = {
	.bid = XNVME_BE_LIOU_BID,
	.name = XNVME_BE_LIOU_NAME,

	.ident_from_uri = xnvme_be_nosys_ident_from_uri,

	.enumerate = xnvme_be_nosys_enumerate,

	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,

	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,

	.async_init = xnvme_be_nosys_async_init,
	.async_term = xnvme_be_nosys_async_term,
	.async_poke = xnvme_be_nosys_async_poke,
	.async_wait = xnvme_be_nosys_async_wait,

	.cmd_pass = xnvme_be_nosys_cmd_pass,
	.cmd_pass_admin = xnvme_be_nosys_cmd_pass_admin,
};
#else
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/nvme_ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <paths.h>
#include <liburing.h>

#include <xnvme_async.h>
#include <xnvme_be_lioc.h>
#include <xnvme_be_liou.h>
#include <xnvme_dev.h>

int
xnvme_be_liou_enumerate(struct xnvme_enumeration *list, int XNVME_UNUSED(opts))
{
	struct dirent **ns = NULL;
	int nns = 0;

	// TODO: add enumeration of NS vs CTRLR and use _PATH_DEV

	nns = scandir("/sys/block", &ns, xnvme_path_nvme_filter, alphasort);
	for (int ni = 0; ni < nns; ++ni) {
		struct xnvme_ident ident = { 0 };
		int nsid, fd;

		snprintf(ident.be_uri, sizeof(ident.uri), _PATH_DEV "%s",
			 ns[ni]->d_name);
		snprintf(ident.uri, sizeof(ident.uri), "liou://" _PATH_DEV "%s",
			 ns[ni]->d_name);

		ident.type = XNVME_IDENT_NS;
		ident.bid = XNVME_BE_LIOU_BID;

		fd = open(ident.be_uri, O_RDWR | O_DIRECT);
		nsid = ioctl(fd, NVME_IOCTL_ID);
		close(fd);
		if (nsid < 1) {
			XNVME_DEBUG("FAILED: retrieving nsid, got: %x", nsid);
			continue;
		}

		ident.nsid = nsid;

		// Attempt to "open" the device, to determine NST
		{
			struct xnvme_dev *dev = NULL;

			dev = xnvme_be_lioc_dev_open(&ident, 0x0);
			if (!dev) {
				XNVME_DEBUG("FAILED: xnvme_be_lioc_dev_open()");
				continue;
			}

			ident = dev->ident;

			xnvme_be_lioc_dev_close(dev);
		}

		if (xnvme_enumeration_append(list, &ident)) {
			XNVME_DEBUG("FAILED: adding entry");
		}
	}

	for (int ni = 0; ni < nns; ++ni) {
		free(ns[ni]);
	}
	free(ns);

	return 0;
}


struct xnvme_async_ctx *
xnvme_be_liou_async_init(struct xnvme_dev *XNVME_UNUSED(dev), uint32_t depth,
			 uint16_t XNVME_UNUSED(flags))
{
	struct xnvme_async_ctx *ctx = NULL;
	int ret = 0;

	ctx = calloc(1, sizeof(*ctx));
	if (!ctx) {
		XNVME_DEBUG("FAILED: calloc(ctx), err: %s", strerror(errno));
		return NULL;
	}
	ctx->depth = depth;

	ctx->be_ctx = calloc(1, sizeof(struct io_uring));
	if (!ctx->be_ctx) {
		XNVME_DEBUG("FAILED: calloc(be_ctx), err: %s", strerror(errno));
		free(ctx);
		return NULL;
	}

	ret = io_uring_queue_init(depth, ctx->be_ctx, 0x0);
	if (ret) {
		XNVME_DEBUG("FAILED: alloc. qpair");
		free(ctx);
		errno = -ret;
		return NULL;
	}

	// TODO: register fd and buffers

	return ctx;
}

int
xnvme_be_liou_async_term(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_async_ctx *ctx)
{
	if (!ctx) {
		XNVME_DEBUG("FAILED: ctx: %p", (void *)ctx);
		errno = EINVAL;
		return -1;
	}
	if (!ctx->be_ctx) {
		free(ctx);

		XNVME_DEBUG("FAILED: be_ctx: %p", (void *)ctx->be_ctx);
		errno = EINVAL;
		return -1;
	}

	io_uring_queue_exit(ctx->be_ctx);
	free(ctx);

	return 0;
}

int
xnvme_be_liou_async_poke(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_async_ctx *ctx,
			 uint32_t XNVME_UNUSED(max))
{
	struct io_uring *ring = ctx->be_ctx;
	struct io_uring_cqe *cqe = NULL;
	struct xnvme_req *req = NULL;
	int ret = 0;

	ret = io_uring_peek_cqe(ring, &cqe);
	if (ret) {
		XNVME_DEBUG("FAILED: io_uring_peek_cqe, ret: '%d'", ret);
		errno = -ret;
		return -1;
	}

	req = (struct xnvme_req *)cqe->user_data;
	if (!req) {
		XNVME_DEBUG("cqe->user_data is NULL! => NO REQ!");
		XNVME_DEBUG("cqe->res: %d", cqe->res);
		XNVME_DEBUG("cqe->flags: %u", cqe->flags);
		errno = EIO;
		return -1;
	}

	// TODO: transform the result-code and possibly flags
	req->cpl.cdw0 = cqe->res > 0 ? cqe->res : 0;
	req->cpl.status.sc = cqe->res < 0 ? -cqe->res : 0x0;

	io_uring_cqe_seen(ring, cqe);

	ctx->outstanding -= 1;
	req->async.cb(req, req->async.cb_arg);

	return 0;
}

int
xnvme_be_liou_async_wait(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx)
{
	int acc = 0;

	int crap = 0;

	while (ctx->outstanding) {
		int err;

		if (crap > 60) {
			errno = EIO;
			return -1;
		}

		err = xnvme_be_liou_async_poke(dev, ctx, 0);
		if (!err) {
			acc += 1;
			continue;
		}

		switch (errno) {
		case EAGAIN:
		case EBUSY:
			crap += 1;
			continue;

		default:
			return -1;
		}
	}

	return acc;
}

void *
xnvme_be_liou_buf_alloc(const struct xnvme_dev *dev, size_t nbytes,
			uint64_t *phys)
{
	void *buf = NULL;

	buf = xnvme_be_lioc_buf_alloc(dev, nbytes, phys);

	// TODO: register buffer with io_uring

	return buf;
}

void
xnvme_be_liou_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	// TODO: io_uring unregister buffer
	//
	xnvme_buf_virt_free(buf);
}

static inline int
async_write(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *dbuf,
	    size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes,
	    int XNVME_UNUSED(opts), struct xnvme_req *req)
{
	struct xnvme_be_liou *state = (struct xnvme_be_liou *)dev->be;
	struct io_uring *ring = req->async.ctx->be_ctx;
	struct io_uring_sqe *sqe = NULL;
	struct iovec *iovec = (struct iovec *) & (req->async.iov);
	int res = 0;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		errno = ENOSYS;
		return -1;
	}

	sqe = io_uring_get_sqe(ring);
	if (!sqe) {
		errno = EAGAIN;
		return -1;
	}

	iovec->iov_base = dbuf;
	iovec->iov_len = dbuf_nbytes;

	sqe->opcode = IORING_OP_WRITEV;
	sqe->flags = 0;
	sqe->ioprio = 0;
	sqe->fd = state->fd;
	sqe->off = cmd->lblk.slba << dev->ssw;
	sqe->addr = (unsigned long) iovec;
	sqe->len = 1;
	sqe->rw_flags = 0;
	sqe->user_data = (unsigned long)req;
	sqe->__pad2[0] = sqe->__pad2[1] = sqe->__pad2[2] = 0;

	req->async.ctx->outstanding += 1;
	res = io_uring_submit(ring);
	if (res < 0) {
		req->async.ctx->outstanding -= 1;
		errno = -res;
		return -1;
	}

	return 0;
}

static inline int
async_read(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *dbuf,
	   size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes,
	   int XNVME_UNUSED(opts), struct xnvme_req *req)
{
	struct xnvme_be_liou *state = (struct xnvme_be_liou *)dev->be;
	struct io_uring *ring = req->async.ctx->be_ctx;
	struct io_uring_sqe *sqe = NULL;
	struct iovec *iovec = (struct iovec *) & (req->async.iov);
	int res = 0;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		errno = ENOSYS;
		return -1;
	}

	sqe = io_uring_get_sqe(ring);
	if (!sqe) {
		errno = EAGAIN;
		return -1;
	}

	iovec->iov_base = dbuf;
	iovec->iov_len = dbuf_nbytes;

	sqe->opcode = IORING_OP_READV;
	sqe->flags = 0;
	sqe->ioprio = 0;
	sqe->fd = state->fd;
	sqe->off = cmd->lblk.slba << dev->ssw;
	sqe->addr = (unsigned long) iovec;
	sqe->len = 1;
	sqe->rw_flags = 0;
	sqe->user_data = (unsigned long)req;
	sqe->__pad2[0] = sqe->__pad2[1] = sqe->__pad2[2] = 0;

	res = io_uring_submit(ring);
	if (res < 0) {
		errno = -res;
		return -1;
	}

	req->async.ctx->outstanding += 1;

	return 0;
}

int
xnvme_be_liou_cmd_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
		       void *dbuf, size_t dbuf_nbytes, void *mbuf,
		       size_t mbuf_nbytes, int opts, struct xnvme_req *req)
{
	if (!(opts & XNVME_CMD_ASYNC)) {
		return xnvme_be_lioc_cmd_pass(dev, cmd, dbuf, dbuf_nbytes, mbuf,
					      mbuf_nbytes, opts, req);
	}

	if (!req) {
		XNVME_DEBUG("FAILED: missing req");
		errno = EINVAL;
		return -1;
	}

	switch (cmd->common.opcode) {
	case XNVME_SPEC_OPC_WRITE:
		return async_write(dev, cmd, dbuf, dbuf_nbytes, mbuf,
				   mbuf_nbytes, opts, req);

	case XNVME_SPEC_OPC_READ:
		return async_read(dev, cmd, dbuf, dbuf_nbytes, mbuf,
				  mbuf_nbytes, opts, req);

	default:
		errno = ENOSYS;
		return -1;
	}
}

struct xnvme_dev *
xnvme_be_liou_dev_open(const struct xnvme_ident *ident, int XNVME_UNUSED(flags))
{
	struct xnvme_dev *dev = NULL;
	struct xnvme_be_lioc *state = NULL;

	dev = xnvme_be_lioc_dev_open(ident, XNVME_BE_LIOC_WRITABLE);
	if (!dev) {
		XNVME_DEBUG("FAILED: xnvme_be_lioc_dev_open");
		return NULL;
	}

	state = (struct xnvme_be_lioc *)dev->be;
	state->be = xnvme_be_liou;
	// Mangle the backend... e.g. expand with stuff

	// TODO: iouring stuff e.g. register fd, setup buffer-registration

	return dev;
}

void
xnvme_be_liou_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	// TODO: unregister fd
	// TODO: de-allocate queue-stuff

	xnvme_be_lioc_dev_close(dev);
}

struct xnvme_be xnvme_be_liou = {
	.bid = XNVME_BE_LIOU_BID,
	.name = XNVME_BE_LIOU_NAME,

	.ident_from_uri = xnvme_be_liou_ident_from_uri,

	.enumerate = xnvme_be_liou_enumerate,

	.dev_open = xnvme_be_liou_dev_open,
	.dev_close = xnvme_be_liou_dev_close,

	.buf_alloc = xnvme_be_liou_buf_alloc,
	.buf_realloc = xnvme_be_lioc_buf_realloc,
	.buf_free = xnvme_be_liou_buf_free,
	.buf_vtophys = xnvme_be_lioc_buf_vtophys,

	.async_init = xnvme_be_liou_async_init,
	.async_term = xnvme_be_liou_async_term,
	.async_poke = xnvme_be_liou_async_poke,
	.async_wait = xnvme_be_liou_async_wait,

	.cmd_pass = xnvme_be_liou_cmd_pass,
	.cmd_pass_admin = xnvme_be_lioc_cmd_pass_admin,
};
#endif
