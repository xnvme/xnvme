// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_RAMDISK_ENABLED
#include <errno.h>
#include <unistd.h>
#include <xnvme_be_ramdisk.h>
#include <xnvme_cmd.h>
#include <xnvme_dev.h>

static int
_idfy_ctrlr(struct xnvme_dev *XNVME_UNUSED(dev), void *dbuf)
{
	struct xnvme_spec_base_identify_controller *ctrlr = dbuf;
	ctrlr->mdts = 0;
	ctrlr->oncs =
		xnvme_bits_set(ctrlr->oncs, XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_NVMCPYS, 1);
	ctrlr->oncs =
		xnvme_bits_set(ctrlr->oncs, XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_NVMWZSV, 1);

	ctrlr->cdfs =
		xnvme_bits_set(ctrlr->cdfs, XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_CDFS_CDF0S, 1);
	return 0;
}

static int
_idfy_ns(struct xnvme_dev *dev, void *dbuf)
{
	struct xnvme_spec_nvm_identify_namespace *ns = dbuf;
	size_t ramdisk_size;
	const size_t lba_size = 512;

	ramdisk_size = xnvme_be_ramdisk_dev_get_size(dev);
	if (!ramdisk_size) {
		return -1;
	}

	ns->nsze = ramdisk_size / lba_size;
	ns->ncap = ramdisk_size / lba_size;
	ns->nuse = ramdisk_size / lba_size;

	ns->nlbaf = 0; ///< This means that there is only one
	ns->flbas = xnvme_bits_set(ns->flbas, XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_FLBAS_FIDXL,
				   0); ///< using the first one

	ns->lbaf[0] = xnvme_bits_set(0, XNVME_SPEC_NVM_LBA_FORMAT_LBADS, XNVME_ILOG2(lba_size));

	ns->mcl = 128;
	ns->mssrl = 128;
	ns->msrc = 127;

	return 0;
}

static int
_idfy(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes)
{
	// A device delivers the whole page, so what is not emulated reads as zero
	memset(dbuf, 0, dbuf_nbytes);

	switch (xnvme_bits_get(ctx->cmd.common.cdw10, XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS)) {
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_NVM_COMMAND_SET_IDENTIFY_NAMESPACE_DATA_STRUCTURE:
		return _idfy_ns(ctx->dev, dbuf);

	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_IDENTIFY_CONTROLLER_DATA_STRUCTURE:
		return _idfy_ctrlr(ctx->dev, dbuf);

	default:
		break;
	}

	/* ///< TODO: set some appropriate status-code for other idfy-cmds */
	/* ctx->cpl.status = xnvme_bits_set(ctx->cpl.status,
	 * XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SC, 0x3); */
	/* ctx->cpl.status = xnvme_bits_set(ctx->cpl.status,
	 * XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SCT, 0x3); */
	return 1;
}

static int
_ramdisk_gfeat(struct xnvme_cmd_ctx *ctx, void *XNVME_UNUSED(dbuf))
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

static int
_xnvme_be_ramdisk_admin_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
				  void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	switch (xnvme_cmd_sqe_get_opc(&ctx->cmd)) {
	case XNVME_SPEC_ADMIN_IDENTIFY_OPCODE:
		return _idfy(ctx, dbuf, dbuf_nbytes);

	case XNVME_SPEC_ADMIN_GET_FEATURES_OPCODE:
		return _ramdisk_gfeat(ctx, dbuf);

	default:
		XNVME_DEBUG("FAILED: ENOSYS opcode: %d", xnvme_cmd_sqe_get_opc(&ctx->cmd));
		return -ENOSYS;
	}
}
#endif

struct xnvme_be_admin g_xnvme_be_ramdisk_admin = {
	.id = "ramdisk",
#ifdef XNVME_BE_RAMDISK_ENABLED
	.cmd_admin = _xnvme_be_ramdisk_admin_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#endif
};
