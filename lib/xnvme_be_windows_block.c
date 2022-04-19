// Copyright (C) Vikash Kumar <vikash.k5@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_dev.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_WINDOWS_ENABLED
#include <libxnvme_spec.h>
#include <errno.h>
#include <windows.h>
#include <winioctl.h>
#include <Setupapi.h>
#include <xnvme_be_windows.h>
#include <xnvme_be_windows_nvme.h>

BOOL
get_drive_geometry(HANDLE dev_handle, DISK_GEOMETRY *pdg)
{
	BOOL ret;      // results flag
	DWORD ret_len; // discard results

	ret = DeviceIoControl(dev_handle,                    // device to be queried
			      IOCTL_DISK_GET_DRIVE_GEOMETRY, // operation to perform
			      NULL, 0,                       // no input buffer
			      pdg, sizeof(*pdg),             // output buffer
			      &ret_len,                      // # bytes returned
			      NULL);

	return (ret);
}

DWORD
get_device_size(HANDLE dev_handle)
{
	DISK_GEOMETRY pdg; // disk drive geometry structure
	BOOL res;
	DWORD disk_size; // size of the drive, in bytes

	res = get_drive_geometry(dev_handle, &pdg);

	if (res) {
		disk_size = pdg.Cylinders.QuadPart * (ULONG)pdg.TracksPerCylinder *
			    (ULONG)pdg.SectorsPerTrack * (ULONG)pdg.BytesPerSector;
	}

	return disk_size;
}

static int
_idfy_ctrlr(struct xnvme_dev *dev, void *dbuf)
{
	// struct xnvme_be_windows_state* state = (void*)dev->be.state;
	struct xnvme_spec_idfy_ctrlr *ctrlr = dbuf;

	ctrlr->mdts = XNVME_ILOG2(1024 * 1024);

	return 0;
}

static int
_idfy_ns(struct xnvme_dev *dev, void *dbuf)
{
	struct xnvme_spec_idfy_ns *ns = dbuf;
	struct xnvme_be_windows_state *state = (void *)dev->be.state;

	DWORD disk_size = get_device_size(state->async_handle);

	ns->nsze = disk_size;
	ns->ncap = disk_size;
	ns->nuse = disk_size;

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
			// case XNVME_SPEC_CSI_FS:
			// return _idfy_ns_iocs_block(ctx->dev, dbuf);

		default:
			break;
		}
		break;

	case XNVME_SPEC_IDFY_CTRLR_IOCS:
		switch (ctx->cmd.idfy.csi) {
			// case XNVME_SPEC_CSI_FS:
			// return _idfy_ctrlr_iocs_block(ctx->dev, dbuf);

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
xnvme_be_windows_block_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf,
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

struct xnvme_be_admin g_xnvme_be_windows_admin_block = {
	.id = "block",
#ifdef XNVME_BE_WINDOWS_ENABLED
	.cmd_admin = xnvme_be_windows_block_cmd_admin,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
#endif
};