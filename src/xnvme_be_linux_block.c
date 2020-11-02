// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifdef XNVME_BE_LINUX_BLOCK_ZONED_ENABLED
#include <linux/blkzoned.h>
#endif
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <xnvme_be_linux.h>
#include <libxnvme_znd.h>

#ifdef BLK_ZONE_REP_CAPACITY
static uint64_t
_lzbd_zone_capacity(struct blk_zone_report *hdr, struct blk_zone *blkz)
{
	if (hdr->flags & BLK_ZONE_REP_CAPACITY) {
		return blkz->capacity;
	}

	return blkz->len;
}
#else
static uint64_t
_lzbd_zone_capacity(struct blk_zone_report *XNVME_UNUSED(hdr),
		    struct blk_zone *blkz)
{
	XNVME_DEBUG("FAILED: nosys ioctl(BLK_ZONE_REP_CAPACITY)");
	return blkz->len;
}
#endif

static int
_lzbd_zone_mgmt_send(struct xnvme_dev *dev, struct xnvme_spec_znd_cmd *cmd)
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	const uint64_t zone_nbytes = geo->nsect * geo->nbytes;
	struct blk_zone_range zr = { 0 };
	int err;

	zr.sector = cmd->mgmt_send.slba << (dev->ssw - LINUX_BLOCK_SSW);
	zr.nr_sectors = zone_nbytes >> LINUX_BLOCK_SSW;

	if (cmd->mgmt_send.zsasf) {
		zr.nr_sectors = zr.nr_sectors * geo->nzone;
	}

	switch (cmd->mgmt_send.zsa) {
#ifdef BLKCLOSEZONE
	case XNVME_SPEC_ZND_CMD_MGMT_SEND_CLOSE:
		err = ioctl(state->fd, BLKCLOSEZONE, &zr);
		break;
#endif
#ifdef BLKFINISHZONE
	case XNVME_SPEC_ZND_CMD_MGMT_SEND_FINISH:
		err = ioctl(state->fd, BLKFINISHZONE, &zr);
		break;
#endif
#ifdef BLKOPENZONE
	case XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN:
		err = ioctl(state->fd, BLKOPENZONE, &zr);
		break;
#endif
#ifdef BLKRESETZONE
	case XNVME_SPEC_ZND_CMD_MGMT_SEND_RESET:
		err = ioctl(state->fd, BLKRESETZONE, &zr);
		break;
#endif
	default:
		XNVME_DEBUG("FAILED: no ioctl for zsa: 0x%x",
			    cmd->mgmt_send.zsa);
		errno = ENOSYS;
		err = -errno;
		break;
	}

	return err;
}

#ifdef BLKGETNRZONES
static int
_lzbd_ioctl_nrzones(struct xnvme_dev *dev, uint64_t *nr_zones)
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;
	int err;

	err = ioctl(state->fd, BLKGETNRZONES, nr_zones);
	if (err) {
		XNVME_DEBUG("FAILED: ioctl(BLKGETNRZONES), err: %d, errno: %d",
			    err, errno);
		return err;
	}
	XNVME_DEBUG("INFO: got nr_zones: %zu", *nr_zones);

	return 0;
}
#else
static int
_lzbd_ioctl_nrzones(struct xnvme_dev *XNVME_UNUSED(dev),
		    uint64_t *XNVME_UNUSED(nr_zones))
{
	XNVME_DEBUG("FAILED: nosys ioctl(BLKGETNRZONES)");
	return -ENOSYS;
}
#endif

#ifdef BLKREPORTZONE
static int
_lzbd_ioctl_rprt_alloc(uint32_t nr_zones, struct blk_zone_report **report)
{
	size_t nbytes = sizeof(**report) + nr_zones * sizeof(struct blk_zone);

	(*report) = calloc(1, nbytes);
	if (!(*report)) {
		XNVME_DEBUG("FAILED: calloc(), errno(%d)", errno);
		return -errno;
	}

	return 0;
}
#else
static int
_lzbd_ioctl_rprt_alloc(uint32_t XNVME_UNUSED(nr_zones),
		       struct blk_zone_report **report)
{
	errno = ENOSYS;
	return -1;
}
#endif

#ifdef BLKREPORTZONE
static int
_lzbd_ioctl_rprt(struct xnvme_dev *dev, uint64_t zslba, uint32_t nzones,
		 struct blk_zone_report *report)
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;

	report->sector = zslba << (dev->ssw - LINUX_BLOCK_SSW);
	report->nr_zones = nzones;

	XNVME_DEBUG("sector: 0x%llx, nr_zones: %u", report->sector,
		    report->nr_zones);

	return ioctl(state->fd, BLKREPORTZONE, report);
}
#else
static int
_lzbd_ioctl_rprt(struct xnvme_dev *XNVME_UNUSED(dev),
		 uint64_t XNVME_UNUSED(zslba), uint32_t XNVME_UNUSED(nzones),
		 struct blk_zone_report *XNVME_UNUSED(report))
{
	XNVME_DEBUG("FAILED: nosys ioctl(BLKREPORTZONE)");
	return -ENOSYS;
}
#endif

// Convert the Zoned Block Report zones to NVMe Report Descriptors
static int
_lzbd_ioctl_zblk_to_descr(struct xnvme_dev *dev,
			  struct blk_zone_report *lzbd_rprt,
			  struct blk_zone *blkz, struct xnvme_spec_znd_descr *zdescr)
{
	zdescr->zslba = blkz->start >> (dev->ssw - LINUX_BLOCK_SSW);
	zdescr->wp = blkz->wp >> (dev->ssw - LINUX_BLOCK_SSW);
	zdescr->zcap = _lzbd_zone_capacity(lzbd_rprt, blkz);
	zdescr->zcap = zdescr->zcap >> (dev->ssw - LINUX_BLOCK_SSW);

	// TODO: convert zone-cap correctly

	// When the type is not defined in the NVMe-spec, then treat
	// them as offline
	zdescr->zt = XNVME_SPEC_ZND_TYPE_SEQWR;
	zdescr->zs = (blkz->type == XNVME_SPEC_ZND_TYPE_SEQWR) ? \
		     blkz->cond : \
		     XNVME_SPEC_ZND_STATE_OFFLINE;

	return 0;
}

static int
_zrasf_to_state(enum xnvme_spec_znd_cmd_mgmt_recv_action_sf zrasf)
{
	switch (zrasf) {
	case XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_ALL:
		return 0;
	case XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_EMPTY:
		return XNVME_SPEC_ZND_STATE_EMPTY;
	case XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_IOPEN:
		return XNVME_SPEC_ZND_STATE_IOPEN;
	case XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_EOPEN:
		return XNVME_SPEC_ZND_STATE_EOPEN;
	case XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_CLOSED:
		return XNVME_SPEC_ZND_STATE_CLOSED;
	case XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_FULL:
		return XNVME_SPEC_ZND_STATE_FULL;
	case XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_RONLY:
		return XNVME_SPEC_ZND_STATE_RONLY;
	case XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_OFFLINE:
		return XNVME_SPEC_ZND_STATE_OFFLINE;
	}

	return -1;
}

/**
 * The size of the buffer dictates how many entries that are returned. The
 * ndwords field provides this information, however, we will use the
 * ``dbuf_nbytes``.
 *
 * The value of partial dictates the representation of hdr->nzones
 * partial=0: number of zones from and including slba matching zrasf on device
 * partial=1: number of zones from and including slba matching zrasf in dbuf
 *
 * When partial=0: then NVMe does something that Linux Zoned does not, so we
 * need to receive the entire report and filter it.
 * When partial=1, then NVMe has the same behavior as Linux Zoned.
 */
static int
_lzbd_zone_mgmt_recv(struct xnvme_dev *dev, struct xnvme_spec_znd_cmd *cmd, void *dbuf,
		     size_t dbuf_nbytes)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	struct xnvme_spec_znd_report_hdr *nvme_rprt = (void *)dbuf;
	struct xnvme_spec_znd_descr *nvme_descr = (void *)dbuf + sizeof(*nvme_rprt);
	struct blk_zone_report *lzbd_rprt = NULL;
	uint64_t nzones_per_cmd = 64;
	uint64_t nzones_dev = 0;
	uint32_t dbuf_nzones;
	int err;

	if (dbuf_nbytes < sizeof(*nvme_rprt)) {
		XNVME_DEBUG("FAILED: dbuf_nbytes: %zu < hdr", dbuf_nbytes);
		return -EINVAL;
	}
	// We cannot do extended report via the Linux Zoned Block Device
	if (cmd->mgmt_recv.zra != 0) {
		XNVME_DEBUG("FAILED: nosys zra: 0x%x", cmd->mgmt_recv.zra);
		return -ENOSYS;
	}

	err = _lzbd_ioctl_nrzones(dev, &nzones_dev);
	if (err) {
		XNVME_DEBUG("FAILED: _lzbd_ioctl_nrzones(), err: %d", err);
		return -EIO;
	}
	memset(dbuf, 0, dbuf_nbytes);

	dbuf_nzones = (dbuf_nbytes - sizeof(*nvme_rprt)) / sizeof(*nvme_descr);

	// Special-case; e.g. asking "how-many-zones" and no room for entries
	if ((cmd->mgmt_recv.zrasf == XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_ALL) && (dbuf_nzones == 0)) {
		nvme_rprt->nzones = nzones_dev;
		return 0;
	}

	// General-case: scan through the report, accumulate nzones and
	// populate descriptors
	err = _lzbd_ioctl_rprt_alloc(nzones_per_cmd, &lzbd_rprt);
	if (err) {
		XNVME_DEBUG("FAILED: _lzbd_ioctl_rprt_alloc(), err: %d", err);
		return -EIO;
	}

	nvme_rprt->nzones = 0;
	for (uint64_t zidx = 0; zidx < nzones_dev; zidx += nzones_per_cmd) {
		uint64_t zslba = cmd->mgmt_recv.slba + zidx * geo->nsect;

		err = _lzbd_ioctl_rprt(dev, zslba, nzones_per_cmd, lzbd_rprt);
		if (err) {
			XNVME_DEBUG("FAILED: _lzbd_ioctl_rprt(), err: %d", err);
			XNVME_DEBUG("INFO: zidx: %zu, zslba: 0x%lx", zidx,
				    zslba);
			err = -EIO;
			goto exit;
		}

		for (uint64_t i = 0; i < lzbd_rprt->nr_zones; ++i) {
			int zone_state = _zrasf_to_state(cmd->mgmt_recv.zrasf);
			uint64_t cur = nvme_rprt->nzones;

			///< Accumulate nzones
			if (!((!zone_state) || \
			      (zone_state == lzbd_rprt->zones[i].cond))) {
				XNVME_DEBUG("INFO: skipping i: %zu", i);
				continue;
			}
			++(nvme_rprt->nzones);

			///< Populate descriptor
			if (cur >= dbuf_nzones) {
				XNVME_DEBUG("INFO: no more room!");
				if (cmd->mgmt_recv.partial) {
					break;
				} else {
					continue;
				}
			}

			_lzbd_ioctl_zblk_to_descr(dev, lzbd_rprt,
						  &lzbd_rprt->zones[i],
						  &nvme_descr[cur]);
		}
	}

exit:
	free(lzbd_rprt);

	return err;
}

int
xnvme_be_linux_block_cmd_io(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
			    void *dbuf, size_t dbuf_nbytes,
			    void *XNVME_UNUSED(mbuf),
			    size_t XNVME_UNUSED(mbuf_nbytes),
			    int XNVME_UNUSED(opts), struct xnvme_req *XNVME_UNUSED(req))
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;
	ssize_t nbytes;

	switch (cmd->common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		nbytes = pwrite(state->fd, dbuf, dbuf_nbytes,
				cmd->nvm.slba << dev->ssw);
		if (nbytes != (ssize_t)dbuf_nbytes) {
			XNVME_DEBUG("FAILED: W nbytes: %ld != dbuf_nbytes: %zu, errno: %d",
				    nbytes, dbuf_nbytes, errno);
			return -errno;
		}
		return 0;

	case XNVME_SPEC_NVM_OPC_READ:
		nbytes = pread(state->fd, dbuf, dbuf_nbytes,
			       cmd->nvm.slba << dev->ssw);
		if (nbytes != (ssize_t)dbuf_nbytes) {
			XNVME_DEBUG("FAILED: R nbytes: %ld != dbuf_nbytes: %zu, errno: %d",
				    nbytes, dbuf_nbytes, errno);
			return -errno;
		}
		return 0;

	case XNVME_SPEC_ZND_OPC_MGMT_SEND:
		return _lzbd_zone_mgmt_send(dev, (void *)cmd);

	case XNVME_SPEC_ZND_OPC_MGMT_RECV:
		return _lzbd_zone_mgmt_recv(dev, (void *)cmd, dbuf, dbuf_nbytes);

	default:
		XNVME_DEBUG("FAILED: nosys opcode: %d", cmd->common.opcode);
		return -ENOSYS;
	}
}

int
_idfy_ctrlr(struct xnvme_dev *dev, void *dbuf)
{
	struct xnvme_spec_idfy_ctrlr *ctrlr = dbuf;
	uint64_t val;
	int err;

	err = xnvme_be_linux_sysfs_dev_attr_to_num(dev,
			"queue/max_hw_sectors_kb",
			&val);
	if (err) {
		XNVME_DEBUG("FAILED: reading 'max_hw_sectors_kb' from sysfs");
		return err;
	}

	XNVME_DEBUG("max_hw_sectors_kb: %zu", val);

	ctrlr->mdts = XNVME_ILOG2(val);

	return 0;
}

int
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

	err = _lzbd_ioctl_nrzones(dev, &nr_zones);
	if (err) {
		XNVME_DEBUG("FAILED blk_zones(), err(%d), errno(%d)",
			    err, errno);
		return err;
	}
	err = xnvme_be_linux_sysfs_dev_attr_to_num(dev, "queue/minimum_io_size",
			&nbytes);
	if (err) {
		XNVME_DEBUG("FAILED: reading 'minimum_io_size' from sysfs");
		return err;
	}

	nsect = tbytes / (nr_zones * nbytes);

	zns->lbafe[0].zsze = nsect;
	zns->mar = 0;	///< This means "no limit" but we have no of knowing
	zns->mor = 0;	///< This means "no limit" but we have no of knowing
	zns->lbafe[0].zdes = 0;

	return 0;
}

int
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
	err = xnvme_be_linux_sysfs_dev_attr_to_num(dev, "queue/minimum_io_size",
			&nbytes);
	if (err) {
		XNVME_DEBUG("FAILED: reading 'minimum_io_size' from sysfs");
		return err;
	}

	ns->nsze = val;
	ns->ncap = val;
	ns->nuse = val;

	ns->nlbaf = 0;          ///< This means that there is only one
	ns->flbas.format = 0;   ///< using the first one

	ns->lbaf[0].ms = 0;
	ns->lbaf[0].ds = XNVME_ILOG2(nbytes);
	ns->lbaf[0].rp = 0;

	return 0;
}

int
_idfy(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd, void *dbuf,
      struct xnvme_req *req)
{
	struct xnvme_spec_znd_idfy_ctrlr *zctrlr = dbuf;
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
			XNVME_DEBUG("FAILED: device is not zoned");
			goto failed;
		}
		return _idfy_ns_iocs(dev, dbuf);

	case XNVME_SPEC_IDFY_NS:
		return _idfy_ns(dev, dbuf);

	///< TODO: check that device supports the given csi
	case XNVME_SPEC_IDFY_CTRLR_IOCS:
		// TODO: setup command-set specific stuff for zoned
		if (!((cmd->idfy.csi == XNVME_SPEC_CSI_ZONED) && (is_zoned))) {
			goto failed;
		}

		zctrlr->zasl = 7;
		break;

	case XNVME_SPEC_IDFY_CTRLR:
		return _idfy_ctrlr(dev, dbuf);

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
	case XNVME_SPEC_ADM_OPC_IDFY:
		return _idfy(dev, cmd, dbuf, req);

	case XNVME_SPEC_ADM_OPC_LOG:
		XNVME_DEBUG("FAILED: not implemented yet.");
		return -ENOSYS;

	default:
		XNVME_DEBUG("FAILED: ENOSYS opcode: %d", cmd->common.opcode);
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

