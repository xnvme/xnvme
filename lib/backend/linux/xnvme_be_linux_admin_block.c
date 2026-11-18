// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libxnvme.h>
#include <xnvme_cmd.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <xnvme_be_posix.h>
#include <xnvme_be_linux.h>
#include <xnvme_spec.h>

#ifdef XNVME_BE_LINUX_BLOCK_ZONED_ENABLED
#include <linux/blkzoned.h>
#endif

static int
_idfy_ctrlr(struct xnvme_dev *dev, void *dbuf)
{
	struct xnvme_spec_base_identify_controller *ctrlr = dbuf;
	uint64_t val;
	int err;

	err = xnvme_be_linux_sysfs_dev_attr_to_num(dev, "queue/max_hw_sectors_kb", &val);
	if (err) {
		XNVME_DEBUG("FAILED: reading 'max_hw_sectors_kb' from sysfs");
		return err;
	}

	XNVME_DEBUG("max_hw_sectors_kb: %zu", val);

	ctrlr->mdts = XNVME_ILOG2(val);

	return 0;
}

static int
_idfy_ns_iocs(struct xnvme_dev *dev, void *dbuf)
{
	struct xnvme_spec_znd_idfy_ns *zns = dbuf;
	uint64_t tbytes, nsect, nbytes, val;
	uint64_t nr_zones = 0;
	int err;

	err = xnvme_be_linux_sysfs_dev_attr_to_num(dev, "size", &val);
	if (err) {
		XNVME_DEBUG("FAILED: reading 'size' from sysfs");
		return err;
	}

	tbytes = val << LINUX_BLOCK_SSW;

	err = xnvme_be_linux_block_nrzones(dev, &nr_zones);
	if (err) {
		XNVME_DEBUG("FAILED blk_zones(), err(%d), errno(%d)", err, errno);
		return err;
	}
	err = xnvme_be_linux_sysfs_dev_attr_to_num(dev, "queue/logical_block_size", &nbytes);
	if (err) {
		XNVME_DEBUG("FAILED: reading 'logical_block_size' from sysfs");
		return err;
	}

	nsect = tbytes / (nr_zones * nbytes);

	zns->lbafe[0].zsze = nsect;
	zns->mar = 0; ///< This means "no limit" but we have no of knowing
	zns->mor = 0; ///< This means "no limit" but we have no of knowing
	zns->lbafe[0].zdes = 0;

	return 0;
}

static int
_idfy_ns(struct xnvme_dev *dev, void *dbuf)
{
	struct xnvme_spec_nvm_identify_namespace *ns = dbuf;
	uint64_t nbytes, val;
	int err;

	err = xnvme_be_linux_sysfs_dev_attr_to_num(dev, "size", &val);
	if (err) {
		XNVME_DEBUG("FAILED: reading 'size' from sysfs");
		return err;
	}
	err = xnvme_be_linux_sysfs_dev_attr_to_num(dev, "queue/logical_block_size", &nbytes);
	if (err) {
		XNVME_DEBUG("FAILED: reading 'logical_block_size' from sysfs");
		return err;
	}

	val /= nbytes >> LINUX_BLOCK_SSW; ///< Size sysfs attr is in terms of LINUX block sector
	ns->nsze = val;
	ns->ncap = val;
	ns->nuse = val;

	ns->nlbaf = 0; ///< This means that there is only one
	ns->flbas = xnvme_bits_set(ns->flbas, XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_FLBAS_FIDXL,
				   0); ///< using the first one

	ns->lbaf[0] = xnvme_bits_set(0, XNVME_SPEC_NVM_LBA_FORMAT_LBADS, XNVME_ILOG2(nbytes));

	return 0;
}

static int
_idfy(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes)
{
	struct xnvme_spec_znd_idfy_ctrlr *zctrlr = dbuf;
	const int buf_len = 0x1000;
	char buf[buf_len];
	int is_zoned;

	// A device delivers the whole page, so what is not emulated reads as zero
	memset(dbuf, 0, dbuf_nbytes);

	xnvme_be_linux_sysfs_dev_attr_to_buf(ctx->dev, "queue/zoned", buf, buf_len);
	is_zoned = strncmp("host-managed", buf, 12) == 0;

	// TODO: convert sysfs size to idfy.ctrlr.ncap
	// TODO: convert sysfs queue/max_sectors_kb to idfy.ctrlr.mdts
	// TODO: convert sysfs queue/minimum_io_size to idfy.ns.lbad.ds

	switch (xnvme_bits_get(ctx->cmd.common.cdw10, XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS)) {
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_NAMESPACE_DATA_STRUCTURE:
		if (!((xnvme_bits_get(ctx->cmd.common.cdw11,
				      XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI) ==
		       XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI_ZONED_NAMESPACE_COMMAND_SET) &&
		      (is_zoned))) {
			XNVME_DEBUG("FAILED: device is not zoned");
			goto failed;
		}
		return _idfy_ns_iocs(ctx->dev, dbuf);

	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_NAMESPACE_IDENTIFICATION_DESCRIPTOR_LIST:
		if (!is_zoned) {
			XNVME_DEBUG("FAILED: device is not zoned");
			goto failed;
		}
		return xnvme_spec_idfy_ns_desc_fill(
			dbuf, dbuf_nbytes,
			XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI_ZONED_NAMESPACE_COMMAND_SET);

	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_NVM_COMMAND_SET_IDENTIFY_NAMESPACE_DATA_STRUCTURE:
		return _idfy_ns(ctx->dev, dbuf);

	///< TODO: check that device supports the given csi
	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_IO_COMMAND_SET_SPECIFIC_IDENTIFY_CONTROLLER_DATA_STRUCTURE:
		// TODO: setup command-set specific stuff for zoned
		if (!((xnvme_bits_get(ctx->cmd.common.cdw11,
				      XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI) ==
		       XNVME_SPEC_ADMIN_IDENTIFY_CDW11_CSI_ZONED_NAMESPACE_COMMAND_SET) &&
		      (is_zoned))) {
			XNVME_DEBUG("FAILED: device is not zoned");
			goto failed;
		}

		zctrlr->zasl = 7;
		break;

	case XNVME_SPEC_ADMIN_IDENTIFY_CDW10_CNS_IDENTIFY_CONTROLLER_DATA_STRUCTURE:
		return _idfy_ctrlr(ctx->dev, dbuf);

	default:
		goto failed;
	}

	ctx->cpl.status = 0;
	return 0;

failed:
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

static int
xnvme_be_linux_block_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			       void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	// NOTE: opcode-dispatch (admin)
	switch (xnvme_cmd_sqe_get_opc(&ctx->cmd)) {
	case XNVME_SPEC_ADMIN_IDENTIFY_OPCODE:
		return _idfy(ctx, dbuf, dbuf_nbytes);

	case XNVME_SPEC_ADMIN_GET_FEATURES_OPCODE:
		return _gfeat(ctx, dbuf);

	case XNVME_SPEC_ADMIN_GET_LOG_PAGE_OPCODE:
		XNVME_DEBUG("FAILED: not implemented yet.");
		return -ENOSYS;

	default:
		XNVME_DEBUG("FAILED: ENOSYS opcode: %d", xnvme_cmd_sqe_get_opc(&ctx->cmd));
		return -ENOSYS;
	}
}
#endif

struct xnvme_be_admin g_xnvme_be_linux_admin_block = {
	.id = "block",
#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
	.cmd_admin = xnvme_be_linux_block_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#endif
};
