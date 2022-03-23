// Copyright (C) Mads Ynddal <m.ynddal@samsung.com>
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

#if defined(XNVME_BE_FBSD_ENABLED) || defined(XNVME_BE_MACOS_ENABLED) || \
	defined(XNVME_BE_POSIX_ENABLED) || defined(XNVME_BE_LINUX_BLOCK_ENABLED)
#include <errno.h>
#include <unistd.h>
#include <libxnvme_spec_fs.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_FBSD_ENABLED
#include <sys/uio.h>
#endif

int
_xnvme_be_psync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
		       void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes), int fd)
{
	const uint64_t ssw = ctx->dev->geo.ssw;
	ssize_t res;

	///< NOTE: opcode-dispatch (io)
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		res = pwrite(fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba << ssw);
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		res = pread(fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba << ssw);
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		res = pwrite(fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba);
		break;

	case XNVME_SPEC_FS_OPC_READ:
		res = pread(fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba);
		break;

	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		res = fsync(fd);
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

// NOTE: preadv/pwritev is not strictly POSIX-compliant, but we use it for FreeBSD, macOS and Linux
// backends. This is not used in the POSIX backend.
#if defined(XNVME_BE_FBSD_ENABLED) || defined(XNVME_BE_MACOS_ENABLED) || \
	defined(XNVME_BE_LINUX_BLOCK_ENABLED)
int
_xnvme_be_psync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			size_t XNVME_UNUSED(dvec_nbytes), struct iovec *XNVME_UNUSED(mvec),
			size_t XNVME_UNUSED(mvec_cnt), size_t XNVME_UNUSED(mvec_nbytes), int fd)
{
	const uint64_t ssw = ctx->dev->geo.ssw;
	ssize_t res;

	///< NOTE: opcode-dispatch (io)
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		res = pwritev(fd, dvec, dvec_cnt, ctx->cmd.nvm.slba << ssw);
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		res = preadv(fd, dvec, dvec_cnt, ctx->cmd.nvm.slba << ssw);
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		res = pwritev(fd, dvec, dvec_cnt, ctx->cmd.nvm.slba);
		break;

	case XNVME_SPEC_FS_OPC_READ:
		res = preadv(fd, dvec, dvec_cnt, ctx->cmd.nvm.slba);
		break;

	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		res = fsync(fd);
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
#endif
