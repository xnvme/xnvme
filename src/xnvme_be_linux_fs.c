// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifdef XNVME_BE_LINUX_FS_ENABLED
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include <errno.h>
#include <xnvme_be_linux.h>

int
xnvme_be_linux_fs_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			 void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_linux_state *state = (void *)ctx->dev->be.state;
	ssize_t nbytes;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		nbytes = pwrite(state->fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba << ctx->dev->ssw);
		if (nbytes != (ssize_t)dbuf_nbytes) {
			XNVME_DEBUG("FAILED: W nbytes: %ld != dbuf_nbytes: %zu, errno: %d",
				    nbytes, dbuf_nbytes, errno);
			return errno ? -errno : -EIO;
		}
		return 0;

	case XNVME_SPEC_NVM_OPC_READ:
		nbytes = pread(state->fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba << ctx->dev->ssw);
		if (nbytes != (ssize_t)dbuf_nbytes) {
			XNVME_DEBUG("FAILED: R nbytes: %ld != dbuf_nbytes: %zu, errno: %d",
				    nbytes, dbuf_nbytes, errno);
			return errno ? -errno : -EIO;
		}
		return 0;

	default:
		XNVME_DEBUG("FAILED: nosys opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}
}

static int
_idfy_ctrlr(struct xnvme_dev *XNVME_UNUSED(dev), void *dbuf)
{
	struct xnvme_spec_idfy_ctrlr *ctrlr = dbuf;

	ctrlr->mdts = 9;

	return 0;
}

static int
_idfy_ns(struct xnvme_dev *XNVME_UNUSED(dev), void *dbuf)
{
	struct xnvme_spec_idfy_ns *ns = dbuf;

	ns->nsze = 1024;
	ns->ncap = 1024;
	ns->nuse = 1024;

	ns->nlbaf = 0;          ///< This means that there is only one
	ns->flbas.format = 0;   ///< using the first one

	ns->lbaf[0].ms = 0;
	ns->lbaf[0].ds = 9;
	ns->lbaf[0].rp = 0;

	return 0;
}

static int
_idfy(struct xnvme_cmd_ctx *ctx, void *dbuf)
{
	switch (ctx->cmd.idfy.cns) {
	case XNVME_SPEC_IDFY_NS:
		return _idfy_ns(ctx->dev, dbuf);

	case XNVME_SPEC_IDFY_CTRLR:
		return _idfy_ctrlr(ctx->dev, dbuf);

	case XNVME_SPEC_IDFY_NS_IOCS:
	case XNVME_SPEC_IDFY_CTRLR_IOCS:
	default:
		goto failed;
	}

	ctx->cpl.status.val = 0;
	return 0;

failed:
	///< TODO: set some appropriate status-code for other idfy-cmds
	ctx->cpl.status.sc = 0x3;
	ctx->cpl.status.sct = 0x3;
	return 1;
}

static int
_gfeat(struct xnvme_cmd_ctx *ctx, void *XNVME_UNUSED(dbuf))
{
	struct xnvme_spec_feat feat = { 0 };

	switch (ctx->cmd.gfeat.fid) {
	case XNVME_SPEC_FEAT_NQUEUES:
		feat.nqueues.nsqa = 63;
		feat.nqueues.ncqa = 63;
		ctx->cpl.cdw0 = feat.val;
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported fid: %d", ctx->cmd.gfeat.fid);
		return -ENOSYS;
	}

	return 0;
}

int
xnvme_be_linux_fs_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf,
			    size_t XNVME_UNUSED(dbuf_nbytes), void *XNVME_UNUSED(mbuf),
			    size_t XNVME_UNUSED(mbuf_nbytes))
{
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_ADM_OPC_IDFY:
		return _idfy(ctx, dbuf);

	case XNVME_SPEC_ADM_OPC_GFEAT:
		return _gfeat(ctx, dbuf);

	case XNVME_SPEC_ADM_OPC_LOG:
		XNVME_DEBUG("FAILED: not implemented yet.");
		return -ENOSYS;

	default:
		XNVME_DEBUG("FAILED: ENOSYS opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}
}

int
xnvme_be_linux_fs_supported(struct xnvme_dev *XNVME_UNUSED(dev), uint32_t XNVME_UNUSED(opts))
{
	return 1;
}

struct xnvme_be_sync g_linux_fs = {
	.cmd_io = xnvme_be_linux_fs_cmd_io,
	.cmd_admin = xnvme_be_linux_fs_cmd_admin,
	.id = "linux_fs",
	.enabled = 1,
	.supported = xnvme_be_linux_fs_supported,
};
#else
#include <xnvme_be_linux.h>
#include <xnvme_be_nosys.h>
struct xnvme_be_sync g_linux_fs = {
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.id = "linux_fs",
	.enabled = 0,
	.supported = xnvme_be_nosys_sync_supported,
};
#endif

