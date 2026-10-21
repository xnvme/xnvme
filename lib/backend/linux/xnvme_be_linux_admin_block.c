// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libxnvme.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <xnvme_be_posix.h>
#include <xnvme_be_linux.h>

#ifdef XNVME_BE_LINUX_BLOCK_ZONED_ENABLED
#include <linux/blkzoned.h>
#endif

static int
_idfy_ctrlr(struct xnvme_dev *dev, void *dbuf)
{
	struct xnvme_spec_idfy_ctrlr *ctrlr = dbuf;
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
	struct xnvme_spec_idfy_ns *ns = dbuf;
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

	ns->nlbaf = 0;        ///< This means that there is only one
	ns->flbas.format = 0; ///< using the first one

	ns->lbaf[0].ms = 0;
	ns->lbaf[0].ds = XNVME_ILOG2(nbytes);
	ns->lbaf[0].rp = 0;

	return 0;
}

static int
_idfy(struct xnvme_cmd_ctx *ctx, void *dbuf)
{
	struct xnvme_spec_znd_idfy_ctrlr *zctrlr = dbuf;
	const int buf_len = 0x1000;
	char buf[buf_len];
	int is_zoned;

	xnvme_be_linux_sysfs_dev_attr_to_buf(ctx->dev, "queue/zoned", buf, buf_len);
	is_zoned = strncmp("host-managed", buf, 12) == 0;

	// TODO: convert sysfs size to idfy.ctrlr.ncap
	// TODO: convert sysfs queue/max_sectors_kb to idfy.ctrlr.mdts
	// TODO: convert sysfs queue/minimum_io_size to idfy.ns.lbad.ds

	switch (ctx->cmd.idfy.cns) {
	case XNVME_SPEC_IDFY_NS_IOCS:
		if (!((ctx->cmd.idfy.csi == XNVME_SPEC_CSI_ZONED) && (is_zoned))) {
			XNVME_DEBUG("FAILED: device is not zoned");
			goto failed;
		}
		return _idfy_ns_iocs(ctx->dev, dbuf);

	case XNVME_SPEC_IDFY_NS:
		return _idfy_ns(ctx->dev, dbuf);

	///< TODO: check that device supports the given csi
	case XNVME_SPEC_IDFY_CTRLR_IOCS:
		// TODO: setup command-set specific stuff for zoned
		if (!((ctx->cmd.idfy.csi == XNVME_SPEC_CSI_ZONED) && (is_zoned))) {
			XNVME_DEBUG("FAILED: device is not zoned");
			goto failed;
		}

		zctrlr->zasl = 7;
		break;

	case XNVME_SPEC_IDFY_CTRLR:
		return _idfy_ctrlr(ctx->dev, dbuf);

	default:
		goto failed;
	}

	ctx->cpl.status.val = 0;
	return 0;

failed:
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

static int
xnvme_be_linux_block_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf,
			       size_t XNVME_UNUSED(dbuf_nbytes), void *XNVME_UNUSED(mbuf),
			       size_t XNVME_UNUSED(mbuf_nbytes))
{
	// NOTE: opcode-dispatch (admin)
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_ADM_OPC_IDFY:
		return _idfy(ctx, dbuf);

	case XNVME_SPEC_ADM_OPC_GFEAT:
		return _gfeat(ctx, dbuf);

	case XNVME_SPEC_ADM_OPC_LOG:
		XNVME_DEBUG("FAILED: not implemented yet.");
		return -ENOSYS;

	default:
		XNVME_DEBUG("FAILED: ENOSYS opcode: %d", ctx->cmd.common.opcode);
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
