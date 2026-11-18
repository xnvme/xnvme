// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <libxnvme.h>
#include <xnvme_cmd.h>
#include <xnvme_dev.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
#include <errno.h>
#include <windows.h>
#include <winioctl.h>
#include <Setupapi.h>
#include <xnvme_be_windows.h>
#include <xnvme_be_windows_nvme.h>
#include <xnvme_be_state.h>

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
	BOOL res = 0;
	DWORD disk_size = 0; // size of the drive, in bytes

	res = get_drive_geometry(dev_handle, &pdg);

	if (res) {
		disk_size = pdg.Cylinders.QuadPart * (ULONG)pdg.TracksPerCylinder *
			    (ULONG)pdg.SectorsPerTrack * (ULONG)pdg.BytesPerSector;
	}

	return disk_size;
}

static int
_idfy_ctrlr(struct xnvme_dev *XNVME_UNUSED(dev), void *dbuf)
{
	struct xnvme_spec_base_identify_controller *ctrlr = dbuf;

	ctrlr->mdts = XNVME_ILOG2(1024 * 1024);

	return 0;
}

static int
_idfy_ns(struct xnvme_dev *dev, void *dbuf)
{
	struct xnvme_spec_nvm_identify_namespace *ns = dbuf;
	struct xnvme_be_windows_state *state = &xnvme_be_state(dev)->windows_state;

	DWORD disk_size = get_device_size(state->async_handle);

	ns->nsze = disk_size;
	ns->ncap = disk_size;
	ns->nuse = disk_size;

	ns->nlbaf = 0; ///< This means that there is only one
	ns->flbas = xnvme_bits_set(ns->flbas, XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_FLBAS_FIDXL,
				   0); ///< using the first one

	ns->lbaf[0] = xnvme_bits_set(0, XNVME_SPEC_NVM_LBA_FORMAT_LBADS, XNVME_ILOG2(512));

	return 0;
}

static int
_idfy(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes)
{
	// A device delivers the whole page, so what is not emulated reads as zero
	memset(dbuf, 0, dbuf_nbytes);

	switch (xnvme_bits_get(ctx->cmd.common.cdw10, XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS)) {
	case XNVME_SPEC_BASE_CNS_NVM_COMMAND_SET_IDENTIFY_NAMESPACE_DATA_STRUCTURE:
		return _idfy_ns(ctx->dev, dbuf);

	case XNVME_SPEC_BASE_CNS_IDENTIFY_CONTROLLER_DATA_STRUCTURE:
		return _idfy_ctrlr(ctx->dev, dbuf);

	case XNVME_SPEC_BASE_CNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE:
		switch (xnvme_bits_get(ctx->cmd.common.cdw11,
				       XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI)) {
			// case XNVME_SPEC_CSI_FS:
			// return _idfy_ns_iocs_block(ctx->dev, dbuf);

		default:
			break;
		}
		break;

	case XNVME_SPEC_BASE_CNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_CONTROLLER_DATA_STRUCTURE:
		switch (xnvme_bits_get(ctx->cmd.common.cdw11,
				       XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI)) {
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
	ctx->cpl.status = xnvme_bits_set(ctx->cpl.status,
					 XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SC, 0x3);
	ctx->cpl.status = xnvme_bits_set(ctx->cpl.status,
					 XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SCT, 0x3);
	return 1;
}

static int
_gfeat(struct xnvme_cmd_ctx *ctx, void *XNVME_UNUSED(dbuf))
{
	struct xnvme_spec_feat feat = {0};

	switch (xnvme_bits_get(ctx->cmd.common.cdw10, XNVME_SPEC_ADMIN_GET_FEATURES_CDW10_FID)) {
	case XNVME_SPEC_FEAT_NQUEUES:
		feat.nqueues.nsqa = 63;
		feat.nqueues.ncqa = 63;
		ctx->cpl.dw0 = feat.val;
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported fid: %d",
			    (int)xnvme_bits_get(ctx->cmd.common.cdw10,
						XNVME_SPEC_ADMIN_GET_FEATURES_CDW10_FID));
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
static int
xnvme_be_windows_block_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
				 void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	///< NOTE: opcode-dispatch (admin)
	switch (xnvme_cmd_sqe_get_opc(&ctx->cmd)) {
	case XNVME_SPEC_ADMIN_IDENTIFY_OPCODE:
		return _idfy(ctx, dbuf, dbuf_nbytes);

	case XNVME_SPEC_ADMIN_GET_FEATURES_OPCODE:
		return _gfeat(ctx, dbuf);

	case XNVME_SPEC_ADMIN_GET_LOG_PAGE_OPCODE:
		XNVME_DEBUG("FAILED: not implemented yet.");
		return -ENOSYS;

	default:
		XNVME_DEBUG("FAILED: ENOSYS opcode: %hu", xnvme_cmd_sqe_get_opc(&ctx->cmd));
		return -ENOSYS;
	}
}
#endif

struct xnvme_be_admin g_xnvme_be_windows_admin_block = {
	.id = "block",
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
	.cmd_admin = xnvme_be_windows_block_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#endif
};
