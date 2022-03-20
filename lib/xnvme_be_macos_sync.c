// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_MACOS_ENABLED
#include <errno.h>
#include <unistd.h>
#include <libxnvme_spec_fs.h>
#include <xnvme_dev.h>
#include <xnvme_be_posix.h>
#include <fcntl.h>

int
xnvme_be_macos_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t mbuf_nbytes)
{
	struct xnvme_be_posix_state *state = (void *)ctx->dev->be.state;
	ssize_t res;

	///< NOTE: opcode-dispatch (io)
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		// NOTE: macOS requires special handling for fsync:
		// https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/fsync.2.html#//apple_ref/doc/man/2/fsync
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
		return xnvme_be_posix_sync_cmd_io(ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes);
	}
}
#endif

struct xnvme_be_sync g_xnvme_be_macos_sync_psync = {
	.id = "macos",
#ifdef XNVME_BE_MACOS_ENABLED
	.cmd_io = xnvme_be_macos_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
