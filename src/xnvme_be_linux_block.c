// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <linux/nvme_ioctl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <xnvme_be_linux.h>
#include <libznd.h>

static int
_sysfs_info_example(struct xnvme_dev *dev)
{
	int buf_len = 0x1000;
	char buf[buf_len];
	uint64_t num;

	xnvme_be_linux_sysfs_dev_attr_to_num(dev, "size", &num);
	XNVME_DEBUG("size: %zu", num);

	xnvme_be_linux_sysfs_dev_attr_to_num(dev, "queue/minimum_io_size", &num);
	XNVME_DEBUG("minimum_io_size: %zu", num);

	xnvme_be_linux_sysfs_dev_attr_to_num(dev, "queue/max_sectors_kb", &num);
	XNVME_DEBUG("max_sectors_kb: %zu", num);

	xnvme_be_linux_sysfs_dev_attr_to_buf(dev, "queue/zoned", buf, buf_len);
	XNVME_DEBUG("zoned: %s", buf);

	return 0;
}

int
xnvme_be_linux_block_cmd_io(struct xnvme_dev *XNVME_UNUSED(dev),
			    struct xnvme_spec_cmd *cmd,
			    void *XNVME_UNUSED(dbuf),
			    size_t XNVME_UNUSED(dbuf_nbytes),
			    void *XNVME_UNUSED(mbuf),
			    size_t XNVME_UNUSED(mbuf_nbytes),
			    int XNVME_UNUSED(opts),
			    struct xnvme_req *XNVME_UNUSED(req))
{
	switch (cmd->common.opcode) {
	case ZND_CMD_OPC_MGMT_SEND:
		XNVME_DEBUG("FAILED: not implemented yet.");
		return -ENOSYS;

	case ZND_CMD_OPC_MGMT_RECV:
		XNVME_DEBUG("FAILED: not implemented yet.");
		return -ENOSYS;

	default:
		XNVME_DEBUG("FAILED: unsupported opcode");
		return -ENOSYS;
	}
}

int
_idfy(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *dbuf,
      struct xnvme_req *req)
{
	struct xnvme_spec_idfy_ctrlr *ctrlr = dbuf;
	struct xnvme_spec_idfy_ns *ns = dbuf;
	struct znd_idfy_ctrlr *zctrlr = dbuf;
	struct znd_idfy_ns *zns = dbuf;
	const int buf_len = 0x1000;
	char buf[buf_len];
	int is_zoned;

	xnvme_be_linux_sysfs_dev_attr_to_buf(dev, "queue/zoned", buf, buf_len);
	is_zoned = strncmp("host-managed", buf, 12) == 0;

	// TODO: convert sysfs size to idfy.ctrlr.ncap
	// TODO: convert sysfs queue/max_sectors_kb to idfy.ctrlr.mdts
	// TODO: convert sysfs queue/minimum_io_size to idfy.ns.lbad.ds

	switch (cmd->idfy.cns) {
	case XNVME_SPEC_IDFY_NS_IOCS:
		if (!((cmd->idfy.csi == XNVME_SPEC_CSI_ZONED) && (is_zoned))) {
			goto failed;
		}

		// Check if the block-layer has something for this
		zns->mar = 383;
		zns->mor = 383;
		zns->lbafe[0].zsze = 4096;
		zns->lbafe[0].zdes = 0;

		break;

	case XNVME_SPEC_IDFY_NS:
		ns->nsze = 2097152;
		ns->ncap = 2097152;
		ns->nuse = 2097152;

		ns->nlbaf = 0;		///< This means that there is only one
		ns->flbas.format = 0;	///< using the first one

		ns->lbaf[0].ms = 0;
		ns->lbaf[0].ds = 9;
		ns->lbaf[0].rp = 0;
		break;

	///< TODO: check that device supports the given csi
	case XNVME_SPEC_IDFY_CTRLR_IOCS:
		// TODO: setup command-set specific stuff for zoned
		if (!((cmd->idfy.csi == XNVME_SPEC_CSI_ZONED) && (is_zoned))) {
			goto failed;
		}

		zctrlr->zasl = 7;
		break;

	case XNVME_SPEC_IDFY_CTRLR:
		ctrlr->mdts = 7;	///< TODO: read this from sysfs
		break;

	default:
		goto failed;
	}

	req->cpl.status.val = 0;
	return 0;

failed:
	///< TODO: set some appropriate status-code for other idfy-cmds
	req->cpl.status.sc = 0x3;
	req->cpl.status.sct = 0x3;
	return 1;
}

int
xnvme_be_linux_block_cmd_admin(struct xnvme_dev *dev,
			       struct xnvme_spec_cmd *cmd, void *dbuf,
			       size_t XNVME_UNUSED(dbuf_nbytes),
			       void *XNVME_UNUSED(mbuf),
			       size_t XNVME_UNUSED(mbuf_nbytes),
			       int XNVME_UNUSED(opts), struct xnvme_req *req)
{
	switch (cmd->common.opcode) {
	case XNVME_SPEC_OPC_IDFY:
		return _idfy(dev, cmd, dbuf, req);

	case XNVME_SPEC_OPC_LOG:
		XNVME_DEBUG("FAILED: not implemented yet.");
		return -ENOSYS;

	default:
		XNVME_DEBUG("FAILED: unsupported opcode");
		return -ENOSYS;
	}
}

int
xnvme_be_linux_block_supported(struct xnvme_dev *dev,
			       uint32_t XNVME_UNUSED(opts))
{
	uint64_t num;
	int err;

	err = xnvme_be_linux_sysfs_dev_attr_to_num(dev, "size", &num);
	if (err) {
		XNVME_DEBUG("FAILED: failed retrieving: 'size' from sysfs");
		return 0;
	}

	XNVME_DEBUG("size: %zu", num);

	_sysfs_info_example(dev);

	return 1;
}

struct xnvme_be_sync g_linux_block = {
	.cmd_io = xnvme_be_linux_block_cmd_io,
	.cmd_admin = xnvme_be_linux_block_cmd_admin,
	.id = "block_ioctl",
	.enabled = 1,
	.supported = xnvme_be_linux_block_supported,
};
#else
#include <xnvme_be_linux.h>
#include <xnvme_be_nosys.h>
struct xnvme_be_sync g_linux_block = {
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.id = "block_ioctl",
	.enabled = 0,
	.supported = xnvme_be_nosys_sync_supported,
};
#endif

