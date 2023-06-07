// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * On MacOS, fsync() does not behave in the same manner as on Linux and FreeBSD, e.g. a fsync()
 * does not flush to the storage device. To achieve the same behavior fcntl(..., F_FULLFSYNC) is
 * needed. To normalize the behavior, this is is applied here for cmd_io() and cmd_iov().
 */
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_MACOS_ENABLED
#include <errno.h>
#include <unistd.h>
#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>
#include <fcntl.h>

static int
cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_be_cbi_state *state = (void *)ctx->dev->be.state;
	ssize_t res;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		res = fsync(state->fd);
		ctx->cpl.result = res;
		if (res < 0) {
			XNVME_DEBUG("FAILED: {fsync}(), errno: %d", errno);
			ctx->cpl.result = 0;
			ctx->cpl.status.sc = errno;
			ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
			return -errno;
		}

		fcntl(state->fd, F_FULLFSYNC);
		ctx->cpl.result = res;
		if (res < 0) {
			XNVME_DEBUG("FAILED: {F_FULLFSYNC}(), errno: %d", errno);
			ctx->cpl.result = 0;
			ctx->cpl.status.sc = errno;
			ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
			return -errno;
		}
		return 0;
	default:
		return xnvme_be_cbi_sync_psync_cmd_io(ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes);
	}
}

int
cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt, size_t dvec_nbytes,
	struct iovec *mvec, size_t mvec_cnt, size_t mvec_nbytes)
{
	struct xnvme_be_cbi_state *state = (void *)ctx->dev->be.state;
	ssize_t res;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		res = fsync(state->fd);
		ctx->cpl.result = res;
		if (res < 0) {
			XNVME_DEBUG("FAILED: {fsync}(), errno: %d", errno);
			ctx->cpl.result = 0;
			ctx->cpl.status.sc = errno;
			ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
			return -errno;
		}

		fcntl(state->fd, F_FULLFSYNC);
		ctx->cpl.result = res;
		if (res < 0) {
			XNVME_DEBUG("FAILED: {F_FULLFSYNC}(), errno: %d", errno);
			ctx->cpl.result = 0;
			ctx->cpl.status.sc = errno;
			ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
			return -errno;
		}
		return 0;
	default:
		return xnvme_be_cbi_sync_psync_cmd_iov(ctx, dvec, dvec_cnt, dvec_nbytes, mvec,
						       mvec_cnt, mvec_nbytes);
	}
}
#endif

struct xnvme_be_sync g_xnvme_be_macos_sync_psync = {
	.id = "macos",
#ifdef XNVME_BE_MACOS_ENABLED
	.cmd_io = cmd_io,
	.cmd_iov = cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
