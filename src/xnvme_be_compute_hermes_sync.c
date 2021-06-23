// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Niclas Hedam <n.hedam@samsung.com, nhed@itu.dk>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_HERMES_ENABLED
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <libxnvme_spec_fs.h>
#include <xnvme_dev.h>
#include <xnvme_be_posix.h>

#include <xnvme_hermes.h>

int
xnvme_be_hermes_sync(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
		     void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_posix_state *state = (void *)ctx->dev->be.state;
	ssize_t res;

	struct hermes_download_prog_ioctl_argp argp = {
		.len = dbuf_nbytes,
		.prog = (uint64_t) dbuf,
	};

	///< NOTE: opcode-dispatch (io)
	switch (ctx->cmd.compute.opcode) {
	case XNVME_SPEC_COMPUTE_OPC_LOAD:
		res = ioctl(state->fd, HERMES_DOWNLOAD_PROG_IOCTL, &argp);
		break;
	case XNVME_SPEC_COMPUTE_OPC_PUSH:
		res = write(state->fd, dbuf, dbuf_nbytes);
		break;
	case XNVME_SPEC_COMPUTE_OPC_PULL:
		res = read(state->fd, dbuf, dbuf_nbytes);
		break;
	case XNVME_SPEC_COMPUTE_OPC_UNLOAD:
		res = 0;
		break; // happens automatially
	case XNVME_SPEC_COMPUTE_OPC_EXEC:
		res = 0;
		break; // programs are executed on load in hermes
	default:
		XNVME_DEBUG("FAILED: nosys opcode: %d", ctx->cmd.compute.opcode);
		return -ENOSYS;
	}

	ctx->cpl.result = res;
	if (res < 0) {
		XNVME_DEBUG("FAILED: BPF, errno: %d", errno);
		ctx->cpl.result = 0;
		ctx->cpl.status.sc = errno;
		ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
		return -errno;
	}

	return 0;
}
#endif

struct xnvme_be_sync g_xnvme_be_compute_sync = {
	.id = "hermes",
#ifdef XNVME_BE_HERMES_ENABLED
	.cmd_io = xnvme_be_hermes_sync,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
#endif
};
