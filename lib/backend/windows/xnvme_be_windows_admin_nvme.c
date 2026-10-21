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

/**
 * For issuing the format command in Windows
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 *
 * @return On success, 0 is returned. On error, `GetLastError()` is returned.
 */
static int
xnvme_be_windows_format(struct xnvme_dev *dev)
{
	int ret = 0;
	int err = 0;
	ULONG ret_len = 0;

	struct xnvme_be_windows_state *state = &xnvme_be_state(dev)->windows_state;

	ret = DeviceIoControl(state->sync_handle, IOCTL_STORAGE_REINITIALIZE_MEDIA, NULL, 0, NULL,
			      0, &ret_len, NULL);

	if (ret == 0) {
		err = GetLastError();
		XNVME_DEBUG("FAILED: DeviceIoControl(), err: %d", err);
	}

	return err;
}

static int
_be_windows_nvme_storage_property(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes)
{
	int ret;
	int err = 0;
	PVOID buffer = NULL;
	ULONG buff_len;
	ULONG ret_len;
	DWORD ioctl = 0;
	PSTORAGE_PROPERTY_QUERY query = NULL;
	PSTORAGE_PROTOCOL_SPECIFIC_DATA protocol_data = NULL;
	PSTORAGE_PROTOCOL_DATA_DESCRIPTOR protocol_data_descr = NULL;

	struct xnvme_be_windows_state *state = &xnvme_be_state(ctx->dev)->windows_state;
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

static int
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
#endif

struct xnvme_be_admin g_xnvme_be_windows_admin_nvme = {
	.id = "nvme",
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
	.cmd_admin = xnvme_be_windows_nvme_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#endif
};
