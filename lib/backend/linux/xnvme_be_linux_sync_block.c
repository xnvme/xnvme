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
#include <xnvme_be_state.h>

#ifdef XNVME_BE_LINUX_BLOCK_ZONED_ENABLED
#include <linux/blkzoned.h>
#endif

#ifdef XNVME_BE_LINUX_BLK_ZONE_CAPACITY_ENABLED
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
_lzbd_zone_capacity(struct blk_zone_report *XNVME_UNUSED(hdr), struct blk_zone *blkz)
{
	XNVME_DEBUG("INFO: struct blk_zone has no capacity member; using len");
	return blkz->len;
}
#endif

static int
_lzbd_zone_mgmt_send(struct xnvme_cmd_ctx *ctx)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(ctx->dev);
	struct xnvme_be_linux_state *state = &xnvme_be_state(ctx->dev)->linux_state;
	const uint64_t zone_nbytes = geo->nsect * geo->nbytes;
	struct blk_zone_range zr = {0};
	int err;

	if (geo->type != XNVME_GEO_ZONED) {
		XNVME_DEBUG("FAILED: device is not zoned, got; %d", geo->type);
		return -EINVAL;
	}

	zr.sector = ctx->cmd.znd.mgmt_send.slba << (ctx->dev->geo.ssw - LINUX_BLOCK_SSW);
	zr.nr_sectors = zone_nbytes >> LINUX_BLOCK_SSW;

	if (ctx->cmd.znd.mgmt_send.select_all) {
		zr.nr_sectors = zr.nr_sectors * geo->nzone;
	}
	if (ctx->cmd.znd.mgmt_send.zsaso) {
		XNVME_DEBUG("FAILED: block-interface does not support ZRWA");
		errno = ENOSYS;
		return -errno;
	}

	switch (ctx->cmd.znd.mgmt_send.zsa) {
#ifdef BLKCLOSEZONE
	case XNVME_SPEC_ZND_CMD_MGMT_SEND_CLOSE:
		err = ioctl(state->posix.fd, BLKCLOSEZONE, &zr);
		break;
#endif
#ifdef BLKFINISHZONE
	case XNVME_SPEC_ZND_CMD_MGMT_SEND_FINISH:
		err = ioctl(state->posix.fd, BLKFINISHZONE, &zr);
		break;
#endif
#ifdef BLKOPENZONE
	case XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN:
		err = ioctl(state->posix.fd, BLKOPENZONE, &zr);
		break;
#endif
#ifdef BLKRESETZONE
	case XNVME_SPEC_ZND_CMD_MGMT_SEND_RESET:
		err = ioctl(state->posix.fd, BLKRESETZONE, &zr);
		break;
#endif
	default:
		XNVME_DEBUG("FAILED: no ioctl for zsa: 0x%x", ctx->cmd.znd.mgmt_send.zsa);
		errno = ENOSYS;
		err = -errno;
		break;
	}

	return err;
}

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
_lzbd_ioctl_rprt_alloc(uint32_t XNVME_UNUSED(nr_zones), struct blk_zone_report **report)
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
	struct xnvme_be_linux_state *state = &xnvme_be_state(dev)->linux_state;

	report->sector = zslba << (dev->geo.ssw - LINUX_BLOCK_SSW);
	report->nr_zones = nzones;

	XNVME_DEBUG("sector: 0x%llx, nr_zones: %u", report->sector, report->nr_zones);

	return ioctl(state->posix.fd, BLKREPORTZONE, report);
}
#else
static int
_lzbd_ioctl_rprt(struct xnvme_dev *XNVME_UNUSED(dev), uint64_t XNVME_UNUSED(zslba),
		 uint32_t XNVME_UNUSED(nzones), struct blk_zone_report *XNVME_UNUSED(report))
{
	XNVME_DEBUG("FAILED: nosys ioctl(BLKREPORTZONE)");
	return -ENOSYS;
}
#endif

// Convert the Zoned Block Report zones to NVMe Report Descriptors
static int
_lzbd_ioctl_zblk_to_descr(struct xnvme_dev *dev, struct blk_zone_report *lzbd_rprt,
			  struct blk_zone *blkz, struct xnvme_spec_znd_descr *zdescr)
{
	zdescr->zslba = blkz->start >> (dev->geo.ssw - LINUX_BLOCK_SSW);
	zdescr->wp = blkz->wp >> (dev->geo.ssw - LINUX_BLOCK_SSW);
	zdescr->zcap = _lzbd_zone_capacity(lzbd_rprt, blkz);
	zdescr->zcap = zdescr->zcap >> (dev->geo.ssw - LINUX_BLOCK_SSW);

	// TODO: convert zone-cap correctly

	// When the type is not defined in the NVMe-spec, then treat
	// them as offline
	zdescr->zt = XNVME_SPEC_ZND_TYPE_SEQWR;
	zdescr->zs = (blkz->type == XNVME_SPEC_ZND_TYPE_SEQWR) ? blkz->cond
							       : XNVME_SPEC_ZND_STATE_OFFLINE;

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
_lzbd_zone_mgmt_recv(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(ctx->dev);
	struct xnvme_spec_znd_report_hdr *nvme_rprt = (void *)dbuf;
	struct xnvme_spec_znd_descr *nvme_descr = (void *)dbuf + sizeof(*nvme_rprt);
	struct blk_zone_report *lzbd_rprt = NULL;
	uint64_t nzones_per_cmd = 64;
	uint64_t nzones_dev = 0;
	uint32_t dbuf_nzones;
	int err;

	if (geo->type != XNVME_GEO_ZONED) {
		XNVME_DEBUG("FAILED: device is not zoned, got; %d", geo->type);
		return -EINVAL;
	}
	if (dbuf_nbytes < sizeof(*nvme_rprt)) {
		XNVME_DEBUG("FAILED: dbuf_nbytes: %zu < hdr", dbuf_nbytes);
		return -EINVAL;
	}
	// We cannot do extended report via the Linux Zoned Block Device
	if (ctx->cmd.znd.mgmt_recv.zra != 0) {
		XNVME_DEBUG("FAILED: nosys zra: 0x%x", ctx->cmd.znd.mgmt_recv.zra);
		return -ENOSYS;
	}

	err = xnvme_be_linux_block_nrzones(ctx->dev, &nzones_dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_linux_block_nrzones(), err: %d", err);
		return -EIO;
	}
	memset(dbuf, 0, dbuf_nbytes);

	dbuf_nzones = (dbuf_nbytes - sizeof(*nvme_rprt)) / sizeof(*nvme_descr);

	// Special-case; e.g. asking "how-many-zones" and no room for entries
	if ((ctx->cmd.znd.mgmt_recv.zrasf == XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_ALL) &&
	    (dbuf_nzones == 0)) {
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
		uint64_t zslba = ctx->cmd.znd.mgmt_recv.slba + zidx * geo->nsect;

		err = _lzbd_ioctl_rprt(ctx->dev, zslba, nzones_per_cmd, lzbd_rprt);
		if (err) {
			XNVME_DEBUG("FAILED: _lzbd_ioctl_rprt(), err: %d", err);
			XNVME_DEBUG("INFO: zidx: %zu, zslba: 0x%lx", zidx, zslba);
			err = -EIO;
			goto exit;
		}

		for (uint64_t i = 0; i < lzbd_rprt->nr_zones; ++i) {
			int zone_state = _zrasf_to_state(ctx->cmd.znd.mgmt_recv.zrasf);
			uint64_t cur = nvme_rprt->nzones;

			///< Accumulate nzones
			if (!((!zone_state) || (zone_state == lzbd_rprt->zones[i].cond))) {
				XNVME_DEBUG("INFO: skipping i: %zu", i);
				continue;
			}
			++(nvme_rprt->nzones);

			///< Populate descriptor
			if (cur >= dbuf_nzones) {
				XNVME_DEBUG("INFO: no more room!");
				if (ctx->cmd.znd.mgmt_recv.partial) {
					break;
				} else {
					continue;
				}
			}

			_lzbd_ioctl_zblk_to_descr(ctx->dev, lzbd_rprt, &lzbd_rprt->zones[i],
						  &nvme_descr[cur]);
		}
	}

exit:
	free(lzbd_rprt);

	return err;
}

static int
xnvme_be_linux_block_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			    size_t mbuf_nbytes)
{
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
	case XNVME_SPEC_NVM_OPC_READ:
	case XNVME_SPEC_FS_OPC_WRITE:
	case XNVME_SPEC_FS_OPC_READ:
	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		return xnvme_be_posix_sync_cmd_io(ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes);

	case XNVME_SPEC_ZND_OPC_MGMT_SEND:
		return _lzbd_zone_mgmt_send(ctx);

	case XNVME_SPEC_ZND_OPC_MGMT_RECV:
		return _lzbd_zone_mgmt_recv(ctx, dbuf, dbuf_nbytes);

	default:
		XNVME_DEBUG("FAILED: nosys opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}
}
#endif

struct xnvme_be_sync g_xnvme_be_linux_sync_block = {
	.id = "block",
#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
	.cmd_io = xnvme_be_linux_block_cmd_io,
	.cmd_iov = xnvme_be_posix_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
