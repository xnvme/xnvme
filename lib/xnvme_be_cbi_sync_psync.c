// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_CBI_SYNC_PSYNC_ENABLED
#include <errno.h>
#include <unistd.h>
#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>

int
xnvme_be_cbi_sync_psync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			       void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_cbi_state *state = (void *)ctx->dev->be.state;
	const uint64_t ssw = ctx->dev->geo.ssw;
	ssize_t res;
	uint8_t sc;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		res = pwrite(state->fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba << ssw);
		sc = res != (ssize_t)dbuf_nbytes ? errno : 0;
		break;
	case XNVME_SPEC_NVM_OPC_READ:
		res = pread(state->fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba << ssw);
		sc = res != (ssize_t)dbuf_nbytes ? errno : 0;
		break;
	case XNVME_SPEC_FS_OPC_WRITE:
		res = pwrite(state->fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba);
		sc = res != (ssize_t)dbuf_nbytes ? errno : 0;
		break;
	case XNVME_SPEC_FS_OPC_READ:
		res = pread(state->fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba);
		sc = res != (ssize_t)dbuf_nbytes ? errno : 0;
		break;

	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		res = fsync(state->fd);
		sc = res ? errno : 0;
		break;

	default:
		sc = res = ENOSYS;
		break;
	}

	ctx->cpl.result = res;
	if (sc) {
		XNVME_DEBUG("FAILED: OPC(%d){pread,pwrite,fsync}(), errno: %d",
			    ctx->cmd.common.opcode, sc);
		ctx->cpl.result = 0;
		ctx->cpl.status.sc = sc;
		ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
		return -sc;
	}

	return 0;
}

int
xnvme_be_cbi_sync_psync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
				size_t dvec_nbytes, struct iovec *XNVME_UNUSED(mvec),
				size_t XNVME_UNUSED(mvec_cnt), size_t XNVME_UNUSED(mvec_nbytes))
{
	struct xnvme_be_cbi_state *state = (void *)ctx->dev->be.state;
	const uint64_t ssw = ctx->dev->geo.ssw;
	ssize_t res;
	uint8_t sc;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		res = pwritev(state->fd, dvec, dvec_cnt, ctx->cmd.nvm.slba << ssw);
		sc = res != (ssize_t)dvec_nbytes ? errno : 0;
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		res = preadv(state->fd, dvec, dvec_cnt, ctx->cmd.nvm.slba << ssw);
		sc = res != (ssize_t)dvec_nbytes ? errno : 0;
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		res = pwritev(state->fd, dvec, dvec_cnt, ctx->cmd.nvm.slba);
		sc = res != (ssize_t)dvec_nbytes ? errno : 0;
		break;

	case XNVME_SPEC_FS_OPC_READ:
		res = preadv(state->fd, dvec, dvec_cnt, ctx->cmd.nvm.slba);
		sc = res != (ssize_t)dvec_nbytes ? errno : 0;
		break;

	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		res = fsync(state->fd);
		sc = res ? errno : 0;
		break;

	default:
		sc = res = ENOSYS;
		break;
	}

	ctx->cpl.result = res;
	if (sc) {
		XNVME_DEBUG("FAILED: OPC(%d){pread,pwrite,fsync}(), errno: %d",
			    ctx->cmd.common.opcode, sc);
		ctx->cpl.result = 0;
		ctx->cpl.status.sc = sc;
		ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
		return -sc;
	}

	return 0;
}
#endif

struct xnvme_be_sync g_xnvme_be_cbi_sync_psync = {
	.id = "psync",
#ifdef XNVME_BE_CBI_SYNC_PSYNC_ENABLED
	.cmd_io = xnvme_be_cbi_sync_psync_cmd_io,
	.cmd_iov = xnvme_be_cbi_sync_psync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
