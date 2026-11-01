// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <libxnvme.h>
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

static int
xnvme_be_windows_read(HANDLE handle, struct xnvme_spec_cmd cmd, void *dbuf, size_t dbuf_nbytes)
{
	struct scsi_pass_through_direct_with_buffer sptdwb;
	int ret = 0;
	int err = 0;
	ULONG length = 0, returned = 0;
	LARGE_INTEGER la;
	la.QuadPart = cmd.nvm.slba;
	uint32_t nlb = cmd.nvm.nlb + 1;

	memset(&sptdwb, 0, sizeof(struct scsi_pass_through_direct_with_buffer));
	sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptdwb.sptd.PathId = 0;
	sptdwb.sptd.TargetId = 0;
	sptdwb.sptd.Lun = 0;
	sptdwb.sptd.CdbLength = CDB10GENERIC_LENGTH;
	sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
	sptdwb.sptd.SenseInfoLength = SPT_SENSE_LENGTH;
	sptdwb.sptd.SenseInfoOffset =
		__builtin_offsetof(struct scsi_pass_through_direct_with_buffer, uc_sense_buf);
	sptdwb.sptd.DataTransferLength = dbuf_nbytes;
	sptdwb.sptd.TimeOutValue = 5;
	sptdwb.sptd.DataBuffer = dbuf;
	sptdwb.sptd.Cdb[0] = SCSIOP_READ;
	sptdwb.sptd.Cdb[2] = ((FOUR_BYTE *)&la.LowPart)->Byte3;
	sptdwb.sptd.Cdb[3] = ((FOUR_BYTE *)&la.LowPart)->Byte2;
	sptdwb.sptd.Cdb[4] = ((FOUR_BYTE *)&la.LowPart)->Byte1;
	sptdwb.sptd.Cdb[5] = ((FOUR_BYTE *)&la.LowPart)->Byte0;
	sptdwb.sptd.Cdb[7] = ((FOUR_BYTE *)&nlb)->Byte1;
	sptdwb.sptd.Cdb[8] = ((FOUR_BYTE *)&nlb)->Byte0;

	length = sizeof(struct scsi_pass_through_direct_with_buffer);
	ret = DeviceIoControl(handle, IOCTL_SCSI_PASS_THROUGH_DIRECT, &sptdwb,
			      sizeof(SCSI_PASS_THROUGH_DIRECT), &sptdwb, length, &returned, FALSE);
	if (ret == 0) {
		err = GetLastError();
		XNVME_DEBUG("FAILED: DeviceIoControl(), err: %d", err);
		return err;
	}
	return 0;
}

static int
xnvme_be_windows_write(HANDLE handle, struct xnvme_spec_cmd cmd, void *dbuf, size_t dbuf_nbytes)
{
	struct scsi_pass_through_direct_with_buffer sptdwb;
	int ret = 0;
	int err = 0;
	ULONG length = 0, returned = 0;
	LARGE_INTEGER la;
	la.QuadPart = cmd.nvm.slba;
	uint32_t nlb = cmd.nvm.nlb + 1;

	memset(&sptdwb, 0, sizeof(struct scsi_pass_through_direct_with_buffer));
	sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptdwb.sptd.PathId = 0;
	sptdwb.sptd.TargetId = 0;
	sptdwb.sptd.Lun = 0;
	sptdwb.sptd.CdbLength = CDB10GENERIC_LENGTH;
	sptdwb.sptd.SenseInfoLength = SPT_SENSE_LENGTH;
	sptdwb.sptd.SenseInfoOffset =
		__builtin_offsetof(struct scsi_pass_through_direct_with_buffer, uc_sense_buf);
	sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
	sptdwb.sptd.DataTransferLength = dbuf_nbytes;
	sptdwb.sptd.TimeOutValue = 5;
	sptdwb.sptd.DataBuffer = dbuf;
	sptdwb.sptd.Cdb[0] = SCSIOP_WRITE;
	sptdwb.sptd.Cdb[2] = ((FOUR_BYTE *)&la.LowPart)->Byte3;
	sptdwb.sptd.Cdb[3] = ((FOUR_BYTE *)&la.LowPart)->Byte2;
	sptdwb.sptd.Cdb[4] = ((FOUR_BYTE *)&la.LowPart)->Byte1;
	sptdwb.sptd.Cdb[5] = ((FOUR_BYTE *)&la.LowPart)->Byte0;
	sptdwb.sptd.Cdb[7] = ((FOUR_BYTE *)&nlb)->Byte1;
	sptdwb.sptd.Cdb[8] = ((FOUR_BYTE *)&nlb)->Byte0;

	length = sizeof(struct scsi_pass_through_direct_with_buffer);
	ret = DeviceIoControl(handle, IOCTL_SCSI_PASS_THROUGH_DIRECT, &sptdwb, length, &sptdwb,
			      length, &returned, FALSE);
	if (ret == 0) {
		err = GetLastError();
		XNVME_DEBUG("FAILED: DeviceIoControl(), err: %d", err);
		return err;
	}
	return 0;
}

static int
xnvme_be_windows_sync_nvme_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
				  void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_windows_state *state = &xnvme_be_state(ctx->dev)->windows_state;
	int ret = 0;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_READ:
		ret = xnvme_be_windows_read(state->sync_handle, ctx->cmd, dbuf, dbuf_nbytes);
		break;

	case XNVME_SPEC_NVM_OPC_WRITE:
		ret = xnvme_be_windows_write(state->sync_handle, ctx->cmd, dbuf, dbuf_nbytes);
		break;

	default:
		XNVME_DEBUG("FAILED: nosys opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	ctx->cpl.result = ret;
	if (ret < 0) {
		XNVME_DEBUG("FAILED: {dev_read,dev_write}(), errno: %d", ret);
		ctx->cpl.result = 0;
		ctx->cpl.status.sc = ret;
		ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
		return ret;
	}

	return 0;
}
#endif

struct xnvme_be_sync g_xnvme_be_windows_sync_nvme = {
	.id = "nvme",
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
	.cmd_io = xnvme_be_windows_sync_nvme_cmd_io,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
#endif
};
