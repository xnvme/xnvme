// Copyright (C) Rishabh Shukla <rishabh.sh@samsung.com>
// Copyright (C) Pranjal Dave <pranjal.58@partner.samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_WINDOWS_FS_ENABLED
#include <errno.h>
#include <libxnvme_spec_fs.h>
#include <xnvme_dev.h>
#include <xnvme_be_windows.h>
#include <windows.h>

/**
 * Pass a file IO Command in Windows
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of data-payload in bytes
 * @param mbuf pointer to meta-payload
 * @param mbuf_nbytes size of meta-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_be_windows_sync_fs_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
				void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	DWORD res = 0;
	int err = 0;
	uint64_t slba = 0;
	OVERLAPPED overlapped;
	struct xnvme_be_windows_state *state = (void *)ctx->dev->be.state;
	const uint64_t ssw = ctx->dev->geo.ssw;

	memset(&overlapped, 0, sizeof(OVERLAPPED));

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		slba = ctx->cmd.nvm.slba << ssw;
		overlapped.Offset = slba;
		overlapped.OffsetHigh = (slba >> 32);
		if (!WriteFile(state->sync_handle, dbuf, dbuf_nbytes, &res, &overlapped)) {
			if (res != (ssize_t)dbuf_nbytes) {
				err = GetLastError();
				XNVME_DEBUG("FAILED: W res: %ld != dbuf_nbytes: %zu, errno: %d",
					    res, dbuf_nbytes, err);
			}
		}
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		overlapped.Offset = ctx->cmd.nvm.slba;
		overlapped.OffsetHigh = (ctx->cmd.nvm.slba >> 32);
		if (!WriteFile(state->sync_handle, dbuf, dbuf_nbytes, &res, &overlapped)) {
			if (res != (ssize_t)dbuf_nbytes) {
				err = GetLastError();
				XNVME_DEBUG("FAILED: W res: %ld != dbuf_nbytes: %zu, errno: %d",
					    res, dbuf_nbytes, err);
			}
		}
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		slba = ctx->cmd.nvm.slba << ssw;
		overlapped.Offset = slba;
		overlapped.OffsetHigh = (slba >> 32);
		if (!ReadFile(state->sync_handle, dbuf, dbuf_nbytes, &res, &overlapped)) {
			if (res != (ssize_t)dbuf_nbytes) {
				err = GetLastError();
				XNVME_DEBUG("FAILED: R res: %ld != dbuf_nbytes: %zu, errno: %d",
					    res, dbuf_nbytes, err);
			}
		}
		break;

	case XNVME_SPEC_FS_OPC_READ:
		if (!ReadFile(state->sync_handle, dbuf, dbuf_nbytes, &res, &overlapped)) {
			if (res != (ssize_t)dbuf_nbytes) {
				err = GetLastError();
				XNVME_DEBUG("FAILED: R res: %ld != dbuf_nbytes: %zu, errno: %d",
					    res, dbuf_nbytes, err);
			}
		}
		break;

	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		if (FlushFileBuffers(state->sync_handle)) {
			res = dbuf_nbytes;
		} else {
			err = GetLastError();
			XNVME_DEBUG("Flush request failed with error code: %d", err);
		}
		break;

	default:
		XNVME_DEBUG("FAILED: nosys opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	ctx->cpl.result = res;
	if (res < 0) {
		XNVME_DEBUG("FAILED: {ReadFile,WriteFile,FlushFileBuffers}(), err: %d", err);
		ctx->cpl.result = 0;
		ctx->cpl.status.sc = err;
		ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
	}

	return err;
}

static int
_idfy_ctrlr_iocs_fs(struct xnvme_dev *XNVME_UNUSED(dev), void *dbuf)
{
	struct xnvme_spec_fs_idfy_ctrlr *ctrlr = dbuf;

	ctrlr->caps.direct = 1;

	ctrlr->iosizes.min = 1;
	ctrlr->iosizes.max = 1024 * 1024 * 128;
	ctrlr->iosizes.opt = 1024 * 64;

	ctrlr->limits.file_data_size = 1;

	ctrlr->ac = 0xAC;
	ctrlr->dc = 0xDC;

	return 0;
}

static int
_idfy_ns_iocs_fs(struct xnvme_dev *dev, void *dbuf)
{
	struct xnvme_spec_fs_idfy_ns *ns = dbuf;
	struct xnvme_be_windows_state *state = (void *)dev->be.state;

	DWORD file_size = GetFileSize(state->sync_handle, NULL);
	if (file_size == INVALID_FILE_SIZE) {
		XNVME_DEBUG("FAILED: GetFileSize, errno: %d", errno);
		return -ENOSYS;
	}

	ns->nsze = file_size;
	ns->ncap = file_size;
	ns->nuse = file_size;

	ns->ac = 0xAC;
	ns->dc = 0xDC;

	return 0;
}

static int
_idfy_ctrlr(struct xnvme_dev *XNVME_UNUSED(dev), void *dbuf)
{
	struct xnvme_spec_idfy_ctrlr *ctrlr = dbuf;

	ctrlr->mdts = XNVME_ILOG2(1024 * 1024);

	return 0;
}

static int
_idfy_ns(struct xnvme_dev *dev, void *dbuf)
{
	struct xnvme_spec_idfy_ns *ns = dbuf;

	struct xnvme_be_windows_state *state = (void *)dev->be.state;

	DWORD file_size = GetFileSize(state->sync_handle, NULL);
	if (file_size == INVALID_FILE_SIZE) {
		XNVME_DEBUG("FAILED: GetFileSize, err: %d", errno);
		return -ENOSYS;
	}

	ns->nsze = file_size;
	ns->ncap = file_size;
	ns->nuse = file_size;

	ns->nlbaf = 0;        ///< This means that there is only one
	ns->flbas.format = 0; ///< using the first one

	ns->lbaf[0].ms = 0;
	ns->lbaf[0].ds = XNVME_ILOG2(512);
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
		switch (ctx->cmd.idfy.csi) {
		case XNVME_SPEC_CSI_FS:
			return _idfy_ns_iocs_fs(ctx->dev, dbuf);

		default:
			break;
		}
		break;

	case XNVME_SPEC_IDFY_CTRLR_IOCS:
		switch (ctx->cmd.idfy.csi) {
		case XNVME_SPEC_CSI_FS:
			return _idfy_ctrlr_iocs_fs(ctx->dev, dbuf);

		default:
			break;
		}
		break;

	default:
		break;
	}

	///< TODO: set some appropriate status-code for other idfy-cmds
	ctx->cpl.status.sc = 0x3;
	ctx->cpl.status.sct = 0x3;
	return 1;
}

static int
_gfeat(struct xnvme_cmd_ctx *ctx, void *XNVME_UNUSED(dbuf))
{
	struct xnvme_spec_feat feat = {0};

	switch (ctx->cmd.gfeat.cdw10.fid) {
	case XNVME_SPEC_FEAT_NQUEUES:
		feat.nqueues.nsqa = 63;
		feat.nqueues.ncqa = 63;
		ctx->cpl.cdw0 = feat.val;
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported fid: %d", ctx->cmd.gfeat.cdw10.fid);
		return -ENOSYS;
	}

	return 0;
}

/**
 * Pass a file Admin Command in Windows
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param dbuf pointer to data-payload
 *
 * @return On success, 0 is returned. On error, negative ENOSYS is returned.
 */
int
xnvme_be_windows_fs_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf,
			      size_t XNVME_UNUSED(dbuf_nbytes), void *XNVME_UNUSED(mbuf),
			      size_t XNVME_UNUSED(mbuf_nbytes))
{
	///< NOTE: opcode-dispatch (admin)
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_ADM_OPC_IDFY:
		return _idfy(ctx, dbuf);

	case XNVME_SPEC_ADM_OPC_GFEAT:
		return _gfeat(ctx, dbuf);

	case XNVME_SPEC_ADM_OPC_LOG:
		XNVME_DEBUG("FAILED: not implemented yet.");
		return -ENOSYS;

	default:
		XNVME_DEBUG("FAILED: ENOSYS opcode: %hu", ctx->cmd.common.opcode);
		return -ENOSYS;
	}
}
#endif

struct xnvme_be_sync g_xnvme_be_windows_sync_fs = {
	.id = "file",
#ifdef XNVME_BE_WINDOWS_FS_ENABLED
	.cmd_io = xnvme_be_windows_sync_fs_cmd_io,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
#endif
};

struct xnvme_be_admin g_xnvme_be_windows_admin_fs = {
	.id = "file",
#ifdef XNVME_BE_WINDOWS_FS_ENABLED
	.cmd_admin = xnvme_be_windows_fs_cmd_admin,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
#endif
};
