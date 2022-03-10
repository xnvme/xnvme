// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_FBSD_ENABLED
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>
#include <libxnvme_spec_fs.h>
#include <xnvme_dev.h>
#include <xnvme_be_fbsd.h>

int
xnvme_be_fbsd_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			  void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_fbsd_state *state = (void *)ctx->dev->be.state;
	const uint64_t ssw = ctx->dev->geo.ssw;
	ssize_t res;

	///< NOTE: opcode-dispatch (io)
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		res = pwrite(state->fd.ns, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba << ssw);
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		res = pread(state->fd.ns, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba << ssw);
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		res = pwrite(state->fd.ns, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba);
		break;

	case XNVME_SPEC_FS_OPC_READ:
		res = pread(state->fd.ns, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba);
		break;

	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		res = fsync(state->fd.ns);
		break;

	default:
		XNVME_DEBUG("FAILED: nosys opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	ctx->cpl.result = res;
	if (res < 0) {
		XNVME_DEBUG("FAILED: {pread,pwrite,fsync}(), errno: %d", errno);
		ctx->cpl.result = 0;
		ctx->cpl.status.sc = errno;
		ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
		return -errno;
	}

	return 0;
}

int
xnvme_be_fbsd_sync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			   size_t XNVME_UNUSED(dvec_nbytes), struct iovec *XNVME_UNUSED(mvec),
			   size_t XNVME_UNUSED(mvec_cnt), size_t XNVME_UNUSED(mvec_nbytes))
{
	struct xnvme_be_fbsd_state *state = (void *)ctx->dev->be.state;
	const uint64_t ssw = ctx->dev->geo.ssw;
	ssize_t res;

	///< NOTE: opcode-dispatch (io)
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		res = pwritev(state->fd.ns, dvec, dvec_cnt, ctx->cmd.nvm.slba << ssw);
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		res = preadv(state->fd.ns, dvec, dvec_cnt, ctx->cmd.nvm.slba << ssw);
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		res = pwritev(state->fd.ns, dvec, dvec_cnt, ctx->cmd.nvm.slba);
		break;

	case XNVME_SPEC_FS_OPC_READ:
		res = preadv(state->fd.ns, dvec, dvec_cnt, ctx->cmd.nvm.slba);
		break;

	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		res = fsync(state->fd.ns);
		break;

	default:
		XNVME_DEBUG("FAILED: nosys opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	ctx->cpl.result = res;
	if (res < 0) {
		XNVME_DEBUG("FAILED: {pread,pwrite,fsync}(), errno: %d", errno);
		ctx->cpl.result = 0;
		ctx->cpl.status.sc = errno;
		ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
		return -errno;
	}

	return 0;
}
#endif

struct xnvme_be_sync g_xnvme_be_fbsd_sync_psync = {
	.id = "psync",
#ifdef XNVME_BE_FBSD_ENABLED
	.cmd_io = xnvme_be_fbsd_sync_cmd_io,
	.cmd_iov = xnvme_be_fbsd_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
