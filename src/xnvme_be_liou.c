// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#define XNVME_BE_LIOU_NAME "liou"

#ifdef XNVME_BE_LIOU_ENABLED
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

// TODO: replace this with liburing 0.7 barriers
#define xnvme_be_liou_barrier()  __asm__ __volatile__("":::"memory")

static int g_opcodes[] = {
	IORING_OP_READV,
	IORING_OP_WRITEV,
	IORING_OP_READ_FIXED,
	IORING_OP_WRITE_FIXED,
	IORING_OP_READ,
	IORING_OP_WRITE,
};
int g_nopcodes = sizeof g_opcodes / sizeof(*g_opcodes);

/**
 * Check whether the Kernel supports the io_uring features used by xNVMe
 *
 * @return On success, 0 is returned. On error, negative errno is returned,
 * specifically -ENOSYS;
 */
static int
xnvme_be_liou_check_support(void)
{
	struct io_uring_probe *probe;
	int err = 0;

	probe = io_uring_get_probe();
	if (!probe) {
		XNVME_DEBUG("FAILED: io_uring_get_probe()");
		err = -ENOSYS;
		goto exit;
	}

	for (int i = 0; i < g_nopcodes; ++i) {
		if (!io_uring_opcode_supported(probe, g_opcodes[i])) {
			err = -ENOSYS;
			XNVME_DEBUG("FAILED: Kernel does not support opc: %d",
				    g_opcodes[i]);
			goto exit;
		}
	}

exit:
	free(probe);

	return err;
}

int
xnvme_be_liou_async_init(struct xnvme_dev *dev, struct xnvme_async_ctx **ctx,
			 uint16_t depth, int flags)
{
	struct xnvme_be_liou_state *state = (void *)dev->be.state;
	struct xnvme_async_ctx_liou *lctx = NULL;
	int err = 0;
	int iou_flags = 0;

	*ctx = calloc(1, sizeof(**ctx));
	if (!*ctx) {
		XNVME_DEBUG("FAILED: calloc(ctx), err: %s", strerror(errno));
		return -errno;
	}
	(*ctx)->depth = depth;

	lctx = (void *)(*ctx);

	if ((flags & XNVME_ASYNC_SQPOLL) || (state->poll_sq)) {
		lctx->poll_sq = 1;
	}
	if ((flags & XNVME_ASYNC_IOPOLL) || (state->poll_io)) {
		lctx->poll_io = 1;
	}

	XNVME_DEBUG("lctx->poll_sq: %d", lctx->poll_sq);
	XNVME_DEBUG("lctx->poll_io: %d", lctx->poll_io);

	// NOTE: Disabling IOPOLL, to avoid lock-up, until fixed in `_poke`
	if (lctx->poll_io) {
		printf("ENOSYS: IORING_SETUP_IOPOLL\n");
		lctx->poll_io = 0;
	}

	//
	// Ring-initialization
	//
	if (lctx->poll_sq) {
		iou_flags |= IORING_SETUP_SQPOLL;
	}
	if (lctx->poll_io) {
		iou_flags |= IORING_SETUP_IOPOLL;
	}

	err = io_uring_queue_init(depth, &lctx->ring, iou_flags);
	if (err) {
		XNVME_DEBUG("FAILED: alloc. qpair");
		free(*ctx);
		return err;
	}

	if (lctx->poll_sq) {
		io_uring_register_files(&lctx->ring, &(state->fd), 1);
	}

	return 0;
}

int
xnvme_be_liou_async_term(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_async_ctx *ctx)
{
	struct xnvme_async_ctx_liou *lctx = NULL;

	if (!ctx) {
		XNVME_DEBUG("FAILED: ctx: %p", (void *)ctx);
		return -EINVAL;
	}

	lctx = (void *)ctx;

	io_uring_unregister_files(&lctx->ring);
	io_uring_queue_exit(&lctx->ring);
	free(ctx);

	return 0;
}

int
xnvme_be_liou_async_poke(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_async_ctx *ctx, uint32_t max)
{
	struct xnvme_async_ctx_liou *lctx = (void *)ctx;
	struct io_uring_cq *ring = &lctx->ring.cq;
	unsigned cq_ring_mask = *ring->kring_mask;
	unsigned completed = 0;
	unsigned head;

	max = max ? max : lctx->outstanding;
	max = max > lctx->outstanding ? lctx->outstanding : max;

	head = *ring->khead;
	do {
		struct io_uring_cqe *cqe;
		struct xnvme_req *req;

		xnvme_be_liou_barrier();
		if (head == *ring->ktail) {
			break;
		}
		cqe = &ring->cqes[head & cq_ring_mask];

		req = (struct xnvme_req *)(uintptr_t) cqe->user_data;
		if (!req) {
			XNVME_DEBUG("-{[THIS SHOULD NOT HAPPEN]}-");
			XNVME_DEBUG("cqe->user_data is NULL! => NO REQ!");
			XNVME_DEBUG("cqe->res: %d", cqe->res);
			XNVME_DEBUG("cqe->flags: %u", cqe->flags);

			io_uring_cqe_seen(&lctx->ring, cqe);
			ctx->outstanding -= 1;

			return -EIO;
		}

		// Map cqe-result to req-completion
		req->cpl.status.sc = cqe->res;

		req->async.cb(req, req->async.cb_arg);

		++completed;
		++head;
	} while (completed < max);

	lctx->outstanding -= completed;
	*ring->khead = head;

	xnvme_be_liou_barrier();

	return completed;
}

int
xnvme_be_liou_async_wait(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx)
{
	int acc = 0;

	while (ctx->outstanding) {
		struct timespec ts1 = {.tv_sec = 0, .tv_nsec = 1000};
		int err;

		err = xnvme_be_liou_async_poke(dev, ctx, 0);
		if (!err) {
			acc += 1;
			continue;
		}

		switch (err) {
		case 0:
		case -EAGAIN:
		case -EBUSY:
			nanosleep(&ts1, NULL);
			continue;

		default:
			return err;
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

	xnvme_buf_virt_free(buf);
}

static inline int
async_io(struct xnvme_dev *dev, int opcode, struct xnvme_spec_cmd *cmd,
	 void *dbuf, size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes,
	 int XNVME_UNUSED(opts), struct xnvme_req *req)
{
	struct xnvme_be_liou_state *state = (void *)dev->be.state;
	struct xnvme_async_ctx_liou *lctx = (void *)req->async.ctx;
	struct io_uring_sqe *sqe = NULL;
	int err = 0;

	if (lctx->outstanding == lctx->depth) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}
	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOSYS;
	}

	sqe = io_uring_get_sqe(&lctx->ring);
	if (!sqe) {
		return -EAGAIN;
	}

	sqe->opcode = opcode;
	sqe->addr = (unsigned long) dbuf;
	sqe->len = dbuf_nbytes;
		sqe->off = cmd->lblk.slba << dev->ssw;
	sqe->flags = lctx->poll_sq ? IOSQE_FIXED_FILE : 0;
	sqe->ioprio = 0;
	// NOTE: we only ever register a single file, the raw device, so the
	// provided index will always be 0
	sqe->fd = lctx->poll_sq ? 0 : state->fd;
	sqe->rw_flags = 0;
	sqe->user_data = (unsigned long)req;
	sqe->__pad2[0] = sqe->__pad2[1] = sqe->__pad2[2] = 0;

	err = io_uring_submit(&lctx->ring);
	if (err < 0) {
		XNVME_DEBUG("io_uring_submit(%d), err: %d", opcode, err);
		return err;
	}

	lctx->outstanding += 1;

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
		return -EINVAL;
	}

	switch (cmd->common.opcode) {
	case XNVME_SPEC_OPC_WRITE:
		return async_io(dev, IORING_OP_WRITE, cmd, dbuf, dbuf_nbytes,
				mbuf, mbuf_nbytes, opts, req);

	case XNVME_SPEC_OPC_READ:
		return async_io(dev, IORING_OP_READ, cmd, dbuf, dbuf_nbytes,
				mbuf, mbuf_nbytes, opts, req);
	}

	return -ENOSYS;
}

void
xnvme_be_liou_state_term(struct xnvme_be_lioc_state *state)
{
	if (!state) {
		return;
	}

	close(state->fd);
}

int
xnvme_be_liou_state_init(struct xnvme_dev *dev)
{
	struct xnvme_be_liou_state *state = (void *)dev->be.state;
	struct stat dev_stat;
	uint32_t opt_val;
	int err;

	err = xnvme_be_liou_check_support();
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_liou_check_support()");
		return err;
	}

	state->fd = open(dev->ident.trgt, O_RDWR | O_DIRECT);
	if (state->fd < 0) {
		XNVME_DEBUG("FAILED: open(parts.trgt(%s)), state->fd(%d)\n",
			    dev->ident.trgt, state->fd);
		return -errno;
	}

	err = fstat(state->fd, &dev_stat);
	if (err < 0) {
		return -errno;
	}
	if (!S_ISBLK(dev_stat.st_mode)) {
		XNVME_DEBUG("FAILED: device is not a block device");
		return -ENOTBLK;
	}

	if (xnvme_ident_opt_to_val(&dev->ident, "poll_io", &opt_val)) {
		state->poll_io = opt_val == 1;
	}
	if (xnvme_ident_opt_to_val(&dev->ident, "poll_sq", &opt_val)) {
		state->poll_sq = opt_val == 1;
	}
	if (xnvme_ident_opt_to_val(&dev->ident, "pseudo", &opt_val)) {
		state->pseudo = opt_val == 1;
	}

	// NOTE: Disabling IOPOLL, to avoid lock-up, until fixed
	if (state->poll_io) {
		printf("ENOSYS: IORING_SETUP_IOPOLL\n");
		state->poll_io = 0;
	}

	XNVME_DEBUG("state->poll_io: %d", state->poll_io);
	XNVME_DEBUG("state->poll_sq: %d", state->poll_sq);
	XNVME_DEBUG("state->pseudo: %d", state->pseudo);

	return 0;
}

int
xnvme_be_liou_dev_from_ident(const struct xnvme_ident *ident,
			     struct xnvme_dev **dev)
{
	int err;

	err = xnvme_dev_alloc(dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_dev_alloc()");
		return err;
	}
	(*dev)->ident = *ident;
	(*dev)->be = xnvme_be_liou;

	err = xnvme_be_liou_state_init(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_liou_state_init()");
		free(*dev);
		return err;
	}
	err = xnvme_be_lioc_dev_idfy(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_lioc_dev_idfy()");
		xnvme_be_liou_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}
	err = xnvme_be_dev_derive_geometry(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_dev_derive_geometry()");
		xnvme_be_liou_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}
	// TODO: consider this. Due to Kernel-segment constraint force mdts down
	if (((*dev)->geo.mdts_nbytes / (*dev)->geo.lba_nbytes) > 127) {
		(*dev)->geo.mdts_nbytes = (*dev)->geo.lba_nbytes * 127;
	}

	return 0;
}

void
xnvme_be_liou_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_be_lioc_dev_close(dev);
}

int
xnvme_be_liou_enumerate(struct xnvme_enumeration *list, const char *sys_uri,
			int XNVME_UNUSED(opts))
{
	struct dirent **ns = NULL;
	int nns = 0;

	if (sys_uri) {
		XNVME_DEBUG("FAILED: sys_uri: %s is not supported", sys_uri);
		return -ENOSYS;
	}

	nns = scandir("/sys/block", &ns, xnvme_path_nvme_filter, alphasort);
	for (int ni = 0; ni < nns; ++ni) {
		char uri[XNVME_IDENT_URI_LEN] = { 0 };
		struct xnvme_ident ident = { 0 };
		struct xnvme_dev *dev;

		snprintf(uri, XNVME_IDENT_URI_LEN - 1,
			 XNVME_BE_LIOU_NAME ":" _PATH_DEV "%s",
			 ns[ni]->d_name);
		if (xnvme_ident_from_uri(uri, &ident)) {
			continue;
		}

		if (xnvme_be_liou_dev_from_ident(&ident, &dev)) {
			XNVME_DEBUG("FAILED: xnvme_be_liou_dev_from_ident()");
			continue;
		}

		xnvme_be_liou_dev_close(dev);
		free(dev);

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
#endif

static const char *g_schemes[] = {
	XNVME_BE_LIOU_NAME,
	"file",
};

struct xnvme_be xnvme_be_liou = {
#ifdef XNVME_BE_LIOU_ENABLED
	.func = {
		.cmd_pass = xnvme_be_liou_cmd_pass,
		.cmd_pass_admin = xnvme_be_lioc_cmd_pass_admin,

		.async_init = xnvme_be_liou_async_init,
		.async_term = xnvme_be_liou_async_term,
		.async_poke = xnvme_be_liou_async_poke,
		.async_wait = xnvme_be_liou_async_wait,

		.buf_alloc = xnvme_be_liou_buf_alloc,
		.buf_vtophys = xnvme_be_lioc_buf_vtophys,
		.buf_realloc = xnvme_be_lioc_buf_realloc,
		.buf_free = xnvme_be_liou_buf_free,

		.enumerate = xnvme_be_liou_enumerate,

		.dev_from_ident = xnvme_be_liou_dev_from_ident,
		.dev_close = xnvme_be_liou_dev_close,
	},
#else
	.func = XNVME_BE_NOSYS_FUNC,
#endif
	.attr = {
		.name = XNVME_BE_LIOU_NAME,
#ifdef XNVME_BE_LIOU_ENABLED
		.enabled = 1,
#else
		.enabled = 0,
#endif
		.schemes = g_schemes,
		.nschemes = sizeof g_schemes / sizeof(*g_schemes),
	},
	.state = { 0 },
};
