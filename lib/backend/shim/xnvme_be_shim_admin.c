// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_SHIM_ENABLED
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xnvme_be_posix.h>
#include <xnvme_be_shim.h>
#include <xnvme_cmd.h>
#include <xnvme_dev.h>
#include <xnvme_be_state.h>
#include <xnvme_spec.h>

static int
idfy_ctrlr_iocs_fs(struct xnvme_dev *XNVME_UNUSED(dev), void *dbuf)
{
	struct xnvme_spec_fs_idfy_ctrlr *ctrlr = dbuf;

	ctrlr->caps = xnvme_bits_set(ctrlr->caps, XNVME_SPEC_FS_IDFY_CTRLR_CAPS_DIRECT, 1);

	ctrlr->iosizes.min = 1;
	ctrlr->iosizes.max = 1024 * 1024 * 128;
	ctrlr->iosizes.opt = 1024 * 64;

	ctrlr->limits.file_data_size = 1;

	ctrlr->ac = 0xAC;
	ctrlr->dc = 0xDC;

	return 0;
}

static int
idfy_ns_iocs_fs(struct xnvme_dev *dev, void *dbuf)
{
	struct xnvme_be_posix_state *state = &xnvme_be_state(dev)->posix_state;
	struct xnvme_spec_fs_idfy_ns *ns = dbuf;
	struct stat stat = {0};
	int err;

	err = fstat(state->fd, &stat);
	if (err) {
		XNVME_DEBUG("FAILED: fstat, err: %d", err);
		return -ENOSYS;
	}

	ns->nsze = stat.st_size;
	ns->ncap = stat.st_size;
	ns->nuse = stat.st_size;

	ns->ac = 0xAC;
	ns->dc = 0xDC;

	return 0;
}

static int
idfy_ctrlr(struct xnvme_dev *XNVME_UNUSED(dev), void *dbuf)
{
	struct xnvme_spec_base_identify_controller *ctrlr = dbuf;

	ctrlr->mdts = XNVME_ILOG2(1024 * 1024);

	return 0;
}

static int
idfy_ns(struct xnvme_dev *dev, void *dbuf)
{
	struct xnvme_be_posix_state *state = &xnvme_be_state(dev)->posix_state;
	struct xnvme_spec_nvm_identify_namespace *ns = dbuf;
	struct stat stat = {0};
	int err;

	err = fstat(state->fd, &stat);
	if (err) {
		XNVME_DEBUG("FAILED: fstat, err: %d", err);
		return -ENOSYS;
	}

	ns->nsze = stat.st_size;
	ns->ncap = stat.st_size;
	ns->nuse = stat.st_size;

	ns->nlbaf = 0; ///< This means that there is only one
	ns->flbas = xnvme_bits_set(ns->flbas, XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_FLBAS_FIDXL,
				   0); ///< using the first one

	ns->lbaf[0] = xnvme_bits_set(0, XNVME_SPEC_NVM_LBA_FORMAT_LBADS, XNVME_ILOG2(512));

	return 0;
}

static int
idfy(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes)
{
	// A device delivers the whole page, so what is not emulated reads as zero
	memset(dbuf, 0, dbuf_nbytes);

	switch (xnvme_bits_get(ctx->cmd.common.cdw10, XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS)) {
	case XNVME_SPEC_BASE_CNS_NVM_COMMAND_SET_IDENTIFY_NAMESPACE_DATA_STRUCTURE:
		return idfy_ns(ctx->dev, dbuf);

	case XNVME_SPEC_BASE_CNS_IDENTIFY_CONTROLLER_DATA_STRUCTURE:
		return idfy_ctrlr(ctx->dev, dbuf);

	case XNVME_SPEC_BASE_CNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE:
		switch (xnvme_bits_get(ctx->cmd.common.cdw11,
				       XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI)) {
		case XNVME_SPEC_CSI_FS:
			return idfy_ns_iocs_fs(ctx->dev, dbuf);

		default:
			break;
		}
		break;

	case XNVME_SPEC_BASE_CNS_NAMESPACE_IDENTIFICATION_DESCRIPTOR_LIST:
		return xnvme_spec_idfy_ns_desc_fill(dbuf, dbuf_nbytes, XNVME_SPEC_CSI_FS);

	case XNVME_SPEC_BASE_CNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_CONTROLLER_DATA_STRUCTURE:
		switch (xnvme_bits_get(ctx->cmd.common.cdw11,
				       XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI)) {
		case XNVME_SPEC_CSI_FS:
			return idfy_ctrlr_iocs_fs(ctx->dev, dbuf);

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
gfeat(struct xnvme_cmd_ctx *ctx, void *XNVME_UNUSED(dbuf))
{

	switch (xnvme_bits_get(ctx->cmd.common.cdw10, XNVME_SPEC_ADMIN_GET_FEATURES_CDW10_FID)) {
	case XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES:
		ctx->cpl.dw0 =
			xnvme_bits_set(0, XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES_CQE_DW0_NSQA,
				       63) |
			xnvme_bits_set(0, XNVME_SPEC_BASE_FEATURE_NUMBER_OF_QUEUES_CQE_DW0_NCQA,
				       63);
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported fid: %d",
			    (int)xnvme_bits_get(ctx->cmd.common.cdw10,
						XNVME_SPEC_ADMIN_GET_FEATURES_CDW10_FID));
		return -ENOSYS;
	}

	return 0;
}

static int
shim_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *XNVME_UNUSED(mbuf),
	       size_t XNVME_UNUSED(mbuf_nbytes))
{
	///< NOTE: opcode-dispatch (admin)
	switch (xnvme_cmd_sqe_get_opc(&ctx->cmd)) {
	case XNVME_SPEC_ADMIN_IDENTIFY_OPCODE:
		return idfy(ctx, dbuf, dbuf_nbytes);

	case XNVME_SPEC_ADMIN_GET_FEATURES_OPCODE:
		return gfeat(ctx, dbuf);

	case XNVME_SPEC_ADMIN_GET_LOG_PAGE_OPCODE:
		XNVME_DEBUG("FAILED: not implemented yet.");
		return -ENOSYS;

	default:
		XNVME_DEBUG("FAILED: ENOSYS opcode: %d", xnvme_cmd_sqe_get_opc(&ctx->cmd));
		return -ENOSYS;
	}
}
#endif

struct xnvme_be_admin g_xnvme_be_shim_admin = {
	.id = "file_as_ns",
#ifdef XNVME_BE_SHIM_ENABLED
	.cmd_admin = shim_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#endif
};
