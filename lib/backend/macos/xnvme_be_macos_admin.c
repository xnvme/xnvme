// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_MACOS_ENABLED
#include <mach/mach_error.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/storage/nvme/NVMeSMARTLibExternal.h>

#include <xnvme_dev.h>
#include <xnvme_be_macos.h>
#include <xnvme_be_state.h>

static int
_gfeat(struct xnvme_cmd_ctx *ctx, void *XNVME_UNUSED(dbuf))
{
	union xnvme_spec_feat feat = {0};

	switch (xnvme_bits_get(ctx->cmd.gfeat.cdw10, XNVME_SPEC_CMD_GFEAT_CDW10_FID)) {
	case XNVME_SPEC_FEAT_NQUEUES:
		feat.nqueues.nsqa = 63;
		feat.nqueues.ncqa = 63;
		ctx->cpl.result =
			xnvme_bits_set(ctx->cpl.result, XNVME_SPEC_CQE_RESULT_CDW0, feat.val);
		break;

	default:
		XNVME_DEBUG(
			"FAILED: unsupported fid: %d",
			(int)xnvme_bits_get(ctx->cmd.gfeat.cdw10, XNVME_SPEC_CMD_GFEAT_CDW10_FID));
		return -ENOSYS;
	}

	return 0;
}

static int
xnvme_be_macos_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
		     void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	IOReturn err;
	int nsid;
	struct xnvme_be_macos_state *state = &xnvme_be_state(ctx->dev)->macos_state;
	IONVMeSMARTInterface **smart = state->nvme_smart_interface;

	switch (ctx->cmd.common.opc) {
	case XNVME_SPEC_ADM_OPC_LOG:
		err = (*smart)->GetLogPage(
			smart, dbuf,
			xnvme_bits_get(ctx->cmd.log.cdw10, XNVME_SPEC_CMD_LOG_CDW10_LID),
			dbuf_nbytes / 4 - 1);
		if (err != kIOReturnSuccess) {
			XNVME_DEBUG("Error in GetLogPage: %s", (char *)mach_error_string(err));
			return 1;
		}
		break;
	case XNVME_SPEC_ADM_OPC_IDFY:
		// The IOKit SMART interface only serves controller (nsid 0) and namespace identify
		// data, so other CNS values fail with an NVMe status instead of returning
		// controller data.
		switch (ctx->cmd.idfy.cns) {
		case XNVME_SPEC_IDFY_NS:
			nsid = ctx->cmd.common.nsid;
			break;
		case XNVME_SPEC_IDFY_CTRLR:
			nsid = 0; // 0 for controller identify data as per Apple docs
			break;
		default:
			XNVME_DEBUG("FAILED: unsupported cns: 0x%x", ctx->cmd.idfy.cns);
			ctx->cpl.status =
				xnvme_bits_set(ctx->cpl.status, XNVME_SPEC_CQE_STATUS_SCT,
					       XNVME_STATUS_CODE_TYPE_GENERIC);
			ctx->cpl.status = xnvme_bits_set(ctx->cpl.status, XNVME_SPEC_CQE_STATUS_SC,
							 XNVME_STATUS_CODE_INVALID_FIELD);
			ctx->cpl.status =
				xnvme_bits_set(ctx->cpl.status, XNVME_SPEC_CQE_STATUS_DNR, 1);
			return -EIO;
		}
		err = (*smart)->GetIdentifyData(smart, dbuf, nsid);
		if (err != kIOReturnSuccess) {
			XNVME_DEBUG("Error in GetIdentifyData: %s",
				    (char *)mach_error_string(err));
			return 1;
		}
		break;
	case XNVME_SPEC_ADM_OPC_GFEAT:
		return _gfeat(ctx, dbuf);
	default:
		XNVME_DEBUG("FAILED: ENOSYS opcode: %d", ctx->cmd.common.opc);
		return -ENOSYS;
	}

	return 0;
}

#endif

struct xnvme_be_admin g_xnvme_be_macos_admin = {
	.id = "nvme",
#ifdef XNVME_PLATFORM_MACOS_ENABLED
	.cmd_admin = xnvme_be_macos_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#endif
};
