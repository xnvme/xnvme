// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

// preadv() and pwritev() are a BSD extension, not POSIX
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_POSIX_ENABLED
#include <errno.h>
#include <unistd.h>
#include <xnvme_cmd.h>
#include <xnvme_dev.h>
#include <xnvme_be_posix.h>
#include <xnvme_be_state.h>

int
xnvme_be_posix_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t mbuf_nbytes)
{
	struct xnvme_be_posix_state *state = &xnvme_be_state(ctx->dev)->posix_state;
	const uint64_t ssw = ctx->dev->geo.ssw;
	ssize_t res;
	uint8_t sc;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		sc = res = ENOTSUP;
		goto exit;
	}

	switch (xnvme_cmd_sqe_get_opc(&ctx->cmd)) {
	case XNVME_SPEC_NVM_WRITE_OPCODE:
		res = pwrite(state->fd, dbuf, dbuf_nbytes,
			     xnvme_cmd_sqe_get_cdw10_11(&ctx->cmd) << ssw);
		sc = res != (ssize_t)dbuf_nbytes ? errno : 0;
		break;
	case XNVME_SPEC_NVM_READ_OPCODE:
		res = pread(state->fd, dbuf, dbuf_nbytes,
			    xnvme_cmd_sqe_get_cdw10_11(&ctx->cmd) << ssw);
		sc = res != (ssize_t)dbuf_nbytes ? errno : 0;
		break;
	case XNVME_SPEC_FS_OPC_WRITE:
		res = pwrite(state->fd, dbuf, dbuf_nbytes, xnvme_cmd_sqe_get_cdw10_11(&ctx->cmd));
		sc = res != (ssize_t)dbuf_nbytes ? errno : 0;
		break;
	case XNVME_SPEC_FS_OPC_READ:
		res = pread(state->fd, dbuf, dbuf_nbytes, xnvme_cmd_sqe_get_cdw10_11(&ctx->cmd));
		sc = res != (ssize_t)dbuf_nbytes ? errno : 0;
		break;

	case XNVME_SPEC_IO_FLUSH_OPCODE:
	case XNVME_SPEC_FS_OPC_FLUSH:
		res = fsync(state->fd);
		sc = res ? errno : 0;
		break;

	default:
		sc = res = ENOSYS;
		break;
	}

exit:
	xnvme_cmd_cpl_set_dw0_1(&ctx->cpl, res);
	if (sc) {
		XNVME_DEBUG("FAILED: OPC(%d){pread,pwrite,fsync}(), errno: %d",
			    xnvme_cmd_sqe_get_opc(&ctx->cmd), sc);
		xnvme_cmd_cpl_set_dw0_1(&ctx->cpl, 0);
		ctx->cpl.status = xnvme_bits_set(
			ctx->cpl.status, XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SC, sc);
		ctx->cpl.status = xnvme_bits_set(ctx->cpl.status,
						 XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SCT,
						 XNVME_STATUS_CODE_TYPE_VENDOR);
		return -sc;
	}

	return 0;
}

int
xnvme_be_posix_sync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			    size_t dvec_nbytes, void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_be_posix_state *state = &xnvme_be_state(ctx->dev)->posix_state;
	const uint64_t ssw = ctx->dev->geo.ssw;
	ssize_t res;
	uint8_t sc;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		sc = res = ENOTSUP;
		goto exit;
	}

	switch (xnvme_cmd_sqe_get_opc(&ctx->cmd)) {
	case XNVME_SPEC_NVM_WRITE_OPCODE:
		res = pwritev(state->fd, dvec, dvec_cnt,
			      xnvme_cmd_sqe_get_cdw10_11(&ctx->cmd) << ssw);
		sc = res != (ssize_t)dvec_nbytes ? errno : 0;
		break;

	case XNVME_SPEC_NVM_READ_OPCODE:
		res = preadv(state->fd, dvec, dvec_cnt,
			     xnvme_cmd_sqe_get_cdw10_11(&ctx->cmd) << ssw);
		sc = res != (ssize_t)dvec_nbytes ? errno : 0;
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		res = pwritev(state->fd, dvec, dvec_cnt, xnvme_cmd_sqe_get_cdw10_11(&ctx->cmd));
		sc = res != (ssize_t)dvec_nbytes ? errno : 0;
		break;

	case XNVME_SPEC_FS_OPC_READ:
		res = preadv(state->fd, dvec, dvec_cnt, xnvme_cmd_sqe_get_cdw10_11(&ctx->cmd));
		sc = res != (ssize_t)dvec_nbytes ? errno : 0;
		break;

	case XNVME_SPEC_IO_FLUSH_OPCODE:
	case XNVME_SPEC_FS_OPC_FLUSH:
		res = fsync(state->fd);
		sc = res ? errno : 0;
		break;

	default:
		sc = res = ENOSYS;
		break;
	}

exit:
	xnvme_cmd_cpl_set_dw0_1(&ctx->cpl, res);
	if (sc) {
		XNVME_DEBUG("FAILED: OPC(%d){pread,pwrite,fsync}(), errno: %d",
			    xnvme_cmd_sqe_get_opc(&ctx->cmd), sc);
		xnvme_cmd_cpl_set_dw0_1(&ctx->cpl, 0);
		ctx->cpl.status = xnvme_bits_set(
			ctx->cpl.status, XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SC, sc);
		ctx->cpl.status = xnvme_bits_set(ctx->cpl.status,
						 XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SCT,
						 XNVME_STATUS_CODE_TYPE_VENDOR);
		return -sc;
	}

	return 0;
}
#endif

struct xnvme_be_sync g_xnvme_be_posix_sync = {
	.id = "psync",
#ifdef XNVME_BE_POSIX_ENABLED
	.cmd_io = xnvme_be_posix_sync_cmd_io,
	.cmd_iov = xnvme_be_posix_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
