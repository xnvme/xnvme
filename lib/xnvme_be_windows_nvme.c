// Copyright (C) Rishabh Shukla <rishabh.sh@samsung.com>
// Copyright (C) Pranjal Dave <pranjal.58@partner.samsung.com>
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

/**
 * For issuing the format command in Windows
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 *
 * @return On success, 0 is returned. On error, `GetLastError()` is returned.
 */
int
xnvme_be_windows_format(struct xnvme_dev *dev)
{
	int ret = 0;
	int err = 0;
	ULONG ret_len = 0;

	struct xnvme_be_windows_state *state = (void *)dev->be.state;

	ret = DeviceIoControl(state->sync_handle, IOCTL_STORAGE_REINITIALIZE_MEDIA, NULL, 0, NULL,
			      0, &ret_len, NULL);

	if (ret == 0) {
		err = GetLastError();
		XNVME_DEBUG("FAILED: DeviceIoControl(), err: %d", err);
	}

	return err;
}

int
_be_windows_nvme_storage_property(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes)
{
	int ret;
	int err = 0;
	PVOID buffer = NULL;
	ULONG buff_len;
	ULONG ret_len;
	DWORD ioctl;
	PSTORAGE_PROPERTY_QUERY query = NULL;
	PSTORAGE_PROTOCOL_SPECIFIC_DATA protocol_data = NULL;
	PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocol_data_descr = NULL;

	struct xnvme_be_windows_state *state = (void *)ctx->dev->be.state;
	// Allocate buffer for use.
	buff_len = __builtin_offsetof(STORAGE_PROPERTY_QUERY, AdditionalParameters) +
		   sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA) + dbuf_nbytes;

	buffer = calloc(1, buff_len);
	if (buffer == NULL) {
		err = GetLastError();
		XNVME_DEBUG("malloc failed err:%d", err);
		goto error_exit;
	}

	// Prepare the query structure
	query = (PSTORAGE_PROPERTY_QUERY)buffer;
	protocol_data_descr = (PSTORAGE_PROTOCOL_DATA_DESCRIPTOR)buffer;
	protocol_data = (PSTORAGE_PROTOCOL_SPECIFIC_DATA)query->AdditionalParameters;

	query->QueryType = PropertyStandardQuery;
	protocol_data->ProtocolDataOffset = sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);
	protocol_data->ProtocolDataLength = dbuf_nbytes;

	///< NOTE: opcode-dispatch (admin)
	// Prepare the specific request based on the admin command
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_ADM_OPC_IDFY:
		ioctl = IOCTL_STORAGE_QUERY_PROPERTY;
		query->PropertyId = StorageDeviceProtocolSpecificProperty;
		protocol_data->ProtocolType = ProtocolTypeNvme;
		protocol_data->DataType = NVMeDataTypeIdentify;
		protocol_data->ProtocolDataRequestValue = ctx->cmd.idfy.cns;
		protocol_data->ProtocolDataRequestSubValue = ctx->cmd.common.nsid;
		if (ctx->cmd.idfy.cns == XNVME_SPEC_IDFY_NS ||
		    ctx->cmd.idfy.cns == XNVME_SPEC_IDFY_CTRLR) {
			query->PropertyId = StorageAdapterProtocolSpecificProperty;
		}
		break;

	case XNVME_SPEC_ADM_OPC_LOG:
		ioctl = IOCTL_STORAGE_QUERY_PROPERTY;
		query->PropertyId = StorageDeviceProtocolSpecificProperty;
		protocol_data->ProtocolType = ProtocolTypeNvme;
		protocol_data->DataType = NVMeDataTypeLogPage;
		protocol_data->ProtocolDataRequestValue = ctx->cmd.log.lid;
		protocol_data->ProtocolDataRequestSubValue = ctx->cmd.common.nsid;
		protocol_data->ProtocolDataRequestSubValue2 = 0;
		protocol_data->ProtocolDataRequestSubValue3 = 0;
		break;

	case XNVME_SPEC_ADM_OPC_GFEAT: {
		DWORD cdw11 = 0;
		ioctl = IOCTL_STORAGE_QUERY_PROPERTY;
		query->PropertyId = StorageDeviceProtocolSpecificProperty;
		protocol_data->ProtocolType = ProtocolTypeNvme;
		protocol_data->DataType = NVMeDataTypeFeature;
		protocol_data->ProtocolDataRequestValue = ctx->cmd.gfeat.cdw10.val;
		protocol_data->ProtocolDataRequestSubValue = cdw11;
		protocol_data->ProtocolDataOffset = 0;
		protocol_data->ProtocolDataLength = 0;
		break;
	}
	case XNVME_SPEC_ADM_OPC_SFEAT: {
		PSTORAGE_PROPERTY_SET query = NULL;
		PSTORAGE_PROTOCOL_SPECIFIC_DATA_EXT protocol_data = NULL;
		DWORD cdw11 = 0;

		ioctl = IOCTL_STORAGE_SET_PROPERTY;
		query = (PSTORAGE_PROPERTY_SET)buffer;
		protocol_data = (PSTORAGE_PROTOCOL_SPECIFIC_DATA_EXT)query->AdditionalParameters;

		query->PropertyId = StorageDeviceProtocolSpecificProperty;
		query->SetType = PropertyStandardSet;
		if (ctx->cmd.sfeat.cdw10.fid == XNVME_SPEC_FEAT_VWCACHE) {
			query->PropertyId = StorageAdapterProtocolSpecificProperty;
		}

		protocol_data->ProtocolType = ProtocolTypeNvme;
		protocol_data->DataType = NVMeDataTypeFeature;
		protocol_data->ProtocolDataValue = ctx->cmd.sfeat.cdw10.val;
		protocol_data->ProtocolDataSubValue = cdw11;
		protocol_data->ProtocolDataOffset = 0;
		protocol_data->ProtocolDataLength = 0;
		break;
	}
	}
	// Send request down.
	ret = DeviceIoControl(state->sync_handle, ioctl, buffer, buff_len, buffer, buff_len,
			      &ret_len, NULL);
	if (0 == ret) {
		err = GetLastError();
		XNVME_DEBUG("FAILED: DeviceIoControl(), err: %d", err);
		goto error_exit;
	}

	// Validate the returned data.
	if ((protocol_data_descr->Version != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR)) ||
	    (protocol_data_descr->Size != sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))) {
		XNVME_DEBUG("Data descriptor header not valid, stop.");
		goto error_exit;
	}

	protocol_data = &protocol_data_descr->ProtocolSpecificData;

	memcpy(dbuf, (void *)((PCHAR)protocol_data + protocol_data->ProtocolDataOffset),
	       dbuf_nbytes);

error_exit:

	free(buffer);

	return err;
}

int
xnvme_be_windows_nvme_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
				void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	int ret = 0;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_ADM_OPC_IDFY:
	case XNVME_SPEC_ADM_OPC_LOG:
	case XNVME_SPEC_ADM_OPC_GFEAT:
	case XNVME_SPEC_ADM_OPC_SFEAT:
		ret = _be_windows_nvme_storage_property(ctx, dbuf, dbuf_nbytes);
		break;

	case XNVME_SPEC_NVM_OPC_FMT:
		ret = xnvme_be_windows_format(ctx->dev);
		break;

	default:
		XNVME_DEBUG("FAILED: ENOSYS opcode: %d", ctx->cmd.common.opcode);
		ret = -ENOSYS;
	}

	return ret;
}

int
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

int
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

int
xnvme_be_windows_sync_nvme_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
				  void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_windows_state *state = (void *)ctx->dev->be.state;
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
#ifdef XNVME_BE_WINDOWS_ENABLED
	.cmd_io = xnvme_be_windows_sync_nvme_cmd_io,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
#endif
};

struct xnvme_be_admin g_xnvme_be_windows_admin_nvme = {
	.id = "nvme",
#ifdef XNVME_BE_WINDOWS_ENABLED
	.cmd_admin = xnvme_be_windows_nvme_cmd_admin,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
#endif
};
