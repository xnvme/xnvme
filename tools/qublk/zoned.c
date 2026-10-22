// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "zoned.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libxnvme.h>

#include "io.h"

static int
zoned_reject(const struct qublk_dev *dev, const char *reason)
{
	fprintf(stderr, "Failed: %s: namespace is zoned (ZNS); %s\n", dev->uri, reason);
	return -EINVAL;
}

#ifdef QUBLK_ZONED_ENABLED

#include <linux/blkzoned.h>

/*
 * Limits follow the kernel NVMe driver: open and active are MOR + 1 and MAR + 1
 * (0xFFFFFFFF wraps to 0, no limit), append is ZASL or, when 0, the transfer limit.
 * A zone report must fit one request, as ublk_report_zones() restarts the zone
 * numbering per request: tag buffers get the report header on top of max_io_buf,
 * which gives up a page where MDTS would not admit it.
 */
int
qublk_zoned_init(struct qublk_dev *dev, int passes_nvme)
{
	const struct xnvme_spec_znd_idfy_ns *zns = xnvme_znd_dev_get_ns(dev->xdev);
	const struct xnvme_spec_znd_idfy_ctrlr *zctrlr = xnvme_znd_dev_get_ctrlr(dev->xdev);
	uint64_t zone_sectors = dev->geo->nsect << (dev->lba_shift - XNVME_UNIVERSAL_SECT_SH);
	uint64_t dev_sectors = dev->geo->tbytes >> XNVME_UNIVERSAL_SECT_SH;
	uint64_t append_nbytes;

	if (!passes_nvme) {
		return zoned_reject(dev, "serving it needs a passthru backend");
	}

	if (!zns) {
		return zoned_reject(dev, "the zoned identify-namespace data is missing");
	}

	// The kernel keeps chunk_sectors in 32 bits and shifts by its log2
	if (!zone_sectors || zone_sectors > UINT32_MAX || (zone_sectors & (zone_sectors - 1))) {
		fprintf(stderr,
			"Failed: %s: namespace is zoned (ZNS); zone size of %" PRIu64
			" sectors is not a power of two\n",
			dev->uri, zone_sectors);
		return -EINVAL;
	}

	// A partial last zone would make START_DEV fail
	if (dev_sectors % zone_sectors) {
		fprintf(stderr,
			"Failed: %s: namespace is zoned (ZNS); size of %" PRIu64
			" sectors is not a multiple of the zone size of %" PRIu64 " sectors\n",
			dev->uri, dev_sectors, zone_sectors);
		return -EINVAL;
	}

	dev->zone_sectors = (uint32_t)zone_sectors;
	dev->nr_zones = (uint32_t)(dev_sectors / zone_sectors);
	dev->max_open = (uint32_t)XNVME_MIN_U64((uint32_t)(zns->mor + 1), dev->nr_zones);
	dev->max_active = (uint32_t)XNVME_MIN_U64((uint32_t)(zns->mar + 1), dev->nr_zones);

	// Open zones are active zones; the kernel (6.13+) refuses START_DEV otherwise
	if (dev->max_active && dev->max_open > dev->max_active) {
		fprintf(stderr,
			"Failed: %s: namespace is zoned (ZNS); max open zones %u exceeds max "
			"active zones %u\n",
			dev->uri, dev->max_open, dev->max_active);
		return -EINVAL;
	}

	if (dev->geo->mdts_nbytes &&
	    (uint64_t)dev->max_io_buf + sizeof(struct xnvme_spec_znd_report_hdr) >
		    dev->geo->mdts_nbytes) {
		uint32_t page = (uint32_t)sysconf(_SC_PAGESIZE);

		if (dev->max_io_buf < 2 * page) {
			fprintf(stderr,
				"Failed: %s: namespace is zoned (ZNS); its transfer size limit "
				"(MDTS %u B) leaves no room for a zone report\n",
				dev->uri, dev->geo->mdts_nbytes);
			return -EINVAL;
		}

		dev->max_io_buf -= page;
	}

	// max_io_buf is final: warn when a report of every zone cannot fit one request
	if (dev->nr_zones > dev->max_io_buf / sizeof(struct xnvme_spec_znd_descr)) {
		uint64_t page = (uint64_t)sysconf(_SC_PAGESIZE);
		uint64_t want = ((uint64_t)dev->nr_zones * sizeof(struct xnvme_spec_znd_descr) +
				 page - 1) &
				~(page - 1);
		char advice[96];

		if (!dev->geo->mdts_nbytes ||
		    want + sizeof(struct xnvme_spec_znd_report_hdr) <= dev->geo->mdts_nbytes) {
			snprintf(advice, sizeof(advice), "raise --max-io-bytes to %" PRIu64, want);
		} else {
			snprintf(advice, sizeof(advice), "MDTS %u B is too small",
				 dev->geo->mdts_nbytes);
		}

		fprintf(stderr,
			"qublk: warning: %s: a report holds %u of %u zones per request; larger "
			"reports can come back scrambled; %s\n",
			dev->uri,
			(uint32_t)(dev->max_io_buf / sizeof(struct xnvme_spec_znd_descr)),
			dev->nr_zones, advice);
	}

	// ZASL is a power-of-two multiple of 4 KiB; beyond 32 it exceeds any cap below
	append_nbytes = dev->max_io_buf;
	if (zctrlr && zctrlr->zasl) {
		append_nbytes = zctrlr->zasl < 32 ? (uint64_t)4096 << zctrlr->zasl : UINT64_MAX;
	}
	append_nbytes = XNVME_MIN_U64(append_nbytes, dev->max_io_buf);
	append_nbytes = XNVME_MIN_U64(append_nbytes, zone_sectors << XNVME_UNIVERSAL_SECT_SH);
	dev->max_append_sectors = (uint32_t)(append_nbytes >> XNVME_UNIVERSAL_SECT_SH);

	// The kernel ties UBLK_F_ZONED to UBLK_F_USER_COPY (see io_addr() in io.c)
	dev->zoned = 1;
	dev->user_copy = 1;
	dev->flags |= UBLK_F_ZONED | UBLK_F_USER_COPY;

	return 0;
}

int
qublk_zoned_check_features(const struct qublk_dev *dev, uint64_t feat)
{
	if (dev->zoned &&
	    (feat & (UBLK_F_ZONED | UBLK_F_USER_COPY)) != (UBLK_F_ZONED | UBLK_F_USER_COPY)) {
		xnvme_cli_perr("Failed: zoned namespace: kernel lacks UBLK_F_ZONED with "
			       "UBLK_F_USER_COPY (6.6+)",
			       -ENOSYS);
		return -ENOSYS;
	}

	return 0;
}

void
qublk_zoned_add_failed(const struct qublk_dev *dev, int rc)
{
	// The flag is advertised, so the kernel refused the combination it requires
	if (rc == -EINVAL && dev->zoned) {
		xnvme_cli_perr("Failed: zoned namespace: kernel lacks zoned ublk "
			       "(CONFIG_BLK_DEV_ZONED, 6.6+)",
			       rc);
	}
}

void
qublk_zoned_print_info(const struct qublk_dev *dev)
{
	if (dev->zoned) {
		fprintf(stderr,
			"qublk: dev id=%d zoned: zone_sectors=%u nr_zones=%u max_open=%u "
			"max_active=%u max_append_sectors=%u\n",
			dev->dev_id, dev->zone_sectors, dev->nr_zones, dev->max_open,
			dev->max_active, dev->max_append_sectors);
	}
}

void
qublk_zoned_set_params(const struct qublk_dev *dev, struct ublk_params *params)
{
	// The kernel's zone count is dev_sectors >> log2(chunk_sectors); init made it exact
	if (dev->zoned) {
		params->types |= UBLK_PARAM_TYPE_ZONED;
		params->basic.chunk_sectors = dev->zone_sectors;
		params->zoned.max_open_zones = dev->max_open;
		params->zoned.max_active_zones = dev->max_active;
		params->zoned.max_zone_append_sectors = dev->max_append_sectors;
	}
}

size_t
qublk_zoned_buf_slack(const struct qublk_dev *dev)
{
	return dev->zoned ? sizeof(struct xnvme_spec_znd_report_hdr) : 0;
}

int
qublk_zoned_op_is_append(uint8_t op)
{
	return op == UBLK_IO_OP_ZONE_APPEND;
}

// The Zone Management Send action of a single-zone op, or -1 for any other op
static int
zone_mgmt_action(uint8_t op)
{
	switch (op) {
	case UBLK_IO_OP_ZONE_OPEN:
		return XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN;
	case UBLK_IO_OP_ZONE_CLOSE:
		return XNVME_SPEC_ZND_CMD_MGMT_SEND_CLOSE;
	case UBLK_IO_OP_ZONE_FINISH:
		return XNVME_SPEC_ZND_CMD_MGMT_SEND_FINISH;
	case UBLK_IO_OP_ZONE_RESET:
		return XNVME_SPEC_ZND_CMD_MGMT_SEND_RESET;
	default:
		return -1;
	}
}

// Descriptors to request: the kernel's count bounded by the buffer, and at least
// one, as the block layer issues one-zone reports (6.10+)
static uint32_t
report_zones_count(const struct qublk_dev *dev, uint32_t nr_zones)
{
	uint32_t room = dev->max_io_buf / sizeof(struct xnvme_spec_znd_descr);

	return nr_zones < room ? (nr_zones ? nr_zones : 1) : room;
}

/*
 * Convert the report in io->buf, in place, to the array of struct blk_zone the
 * kernel expects and copy it out. Zone i overlaps only descriptor i - 1, which
 * is already consumed. The kernel zeroed its buffer and stops at the first
 * zero-length zone.
 *
 * The result is max_io_buf, not the bytes written: the kernel maps its whole
 * report buffer into every request of a report, and a shorter result is
 * requeued forever. With user copy the kernel clamps the result to the mapped
 * length, so max_io_buf is exact.
 */
static int
report_zones_complete(struct qublk_queue *q, struct qublk_io *io)
{
	struct qublk_dev *dev = q->dev;
	uint8_t shift = dev->lba_shift - XNVME_UNIVERSAL_SECT_SH;
	uint32_t n = report_zones_count(dev, io->iod->nr_zones);
	struct xnvme_spec_znd_report_hdr hdr;
	uint8_t *buf = io->buf;
	uint64_t nzones;

	memcpy(&hdr, buf, sizeof(hdr));
	nzones = XNVME_MIN_U64(hdr.nzones, n);

	for (uint64_t i = 0; i < nzones; i++) {
		struct xnvme_spec_znd_descr descr;
		struct blk_zone zone = {0};

		memcpy(&descr, buf + sizeof(hdr) + i * sizeof(descr), sizeof(descr));

		zone.start = descr.zslba << shift;
		zone.len = dev->zone_sectors;
		zone.capacity = descr.zcap << shift;
		zone.wp = (descr.zs == XNVME_SPEC_ZND_STATE_FULL) ? zone.start + zone.len
								  : descr.wp << shift;
		zone.type = descr.zt;
		zone.cond = descr.zs;

		memcpy(buf + i * sizeof(zone), &zone, sizeof(zone));
	}

	if (nzones && qublk_io_user_copy_out(q, io, nzones * sizeof(struct blk_zone))) {
		return -EIO;
	}

	return (int)dev->max_io_buf;
}

int
qublk_zoned_dispatch(struct qublk_queue *q, struct qublk_io *io, struct xnvme_cmd_ctx *ctx)
{
	struct qublk_dev *dev = q->dev;
	const struct ublksrv_io_desc *iod = io->iod;
	uint32_t nsid = xnvme_dev_get_nsid(dev->xdev);
	uint8_t lba_shift = dev->lba_shift;
	uint8_t op = ublksrv_get_op(iod);
	uint64_t bytes = (uint64_t)iod->nr_sectors << XNVME_UNIVERSAL_SECT_SH;
	uint64_t slba;
	int rc;

	switch (op) {
	case UBLK_IO_OP_REPORT_ZONES: {
		// nr_zones aliases nr_sectors; the start sector is a zone start
		uint32_t n = report_zones_count(dev, iod->nr_zones);

		slba = iod->start_sector >> (lba_shift - XNVME_UNIVERSAL_SECT_SH);
		return xnvme_znd_mgmt_recv(ctx, nsid, slba,
					   XNVME_SPEC_ZND_CMD_MGMT_RECV_ACTION_REPORT,
					   XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_ALL, 1, io->buf,
					   (uint32_t)(sizeof(struct xnvme_spec_znd_report_hdr) +
						      n * sizeof(struct xnvme_spec_znd_descr)));
	}
	case UBLK_IO_OP_ZONE_APPEND:
	case UBLK_IO_OP_ZONE_OPEN:
	case UBLK_IO_OP_ZONE_CLOSE:
	case UBLK_IO_OP_ZONE_FINISH:
	case UBLK_IO_OP_ZONE_RESET:
		if (!dev->zoned) {
			return -EOPNOTSUPP;
		}

		// The block layer only issues these at zone starts
		if (iod->start_sector & (dev->zone_sectors - 1)) {
			XNVME_DEBUG("FAILED: dev%d q%d tag %u: op %u sector %llu not a zone start",
				    dev->dev_id, q->q_id, io->tag, op,
				    (unsigned long long)iod->start_sector);
			return -EINVAL;
		}

		slba = iod->start_sector >> (lba_shift - XNVME_UNIVERSAL_SECT_SH);

		if (op == UBLK_IO_OP_ZONE_APPEND) {
			uint16_t nlb;

			// The block layer does not split an append to the limit given to
			// the kernel, so a larger one is a bug upstream: fail it
			if (bytes >
			    ((uint64_t)dev->max_append_sectors << XNVME_UNIVERSAL_SECT_SH)) {
				XNVME_DEBUG(
					"FAILED: dev%d q%d tag %u: append %lu B exceeds limit %u "
					"sectors",
					dev->dev_id, q->q_id, io->tag, (unsigned long)bytes,
					dev->max_append_sectors);
				return -EINVAL;
			}

			nlb = (uint16_t)((bytes >> lba_shift) - 1);
			if (iod->op_flags & UBLK_IO_F_FUA) {
				ctx->cmd.znd.append.fua = 1;
			}

			rc = qublk_io_user_copy_in(q, io, (size_t)bytes);
			if (rc) {
				return rc;
			}

			return xnvme_znd_append(ctx, nsid, slba, nlb, io->buf, NULL);
		}

		rc = zone_mgmt_action(op);
		if (rc < 0) {
			return -EOPNOTSUPP;
		}

		return xnvme_znd_mgmt_send(ctx, nsid, slba, false, rc, 0, NULL);
	case UBLK_IO_OP_ZONE_RESET_ALL:
		if (!dev->zoned) {
			return -EOPNOTSUPP;
		}

		// Select all: the ZSLBA is ignored by the device
		return xnvme_znd_mgmt_send(ctx, nsid, 0, true, XNVME_SPEC_ZND_CMD_MGMT_SEND_RESET,
					   0, NULL);
	default:
		return -EOPNOTSUPP;
	}
}

int
qublk_zoned_complete(struct qublk_queue *q, struct qublk_io *io, struct xnvme_cmd_ctx *ctx)
{
	uint8_t op = ublksrv_get_op(io->iod);

	if (op == UBLK_IO_OP_ZONE_APPEND) {
		// The device chose the LBA; the driver wants it in 512 B sectors, as
		// the request's sector (see io_addr() in io.c)
		io->zone_append_lba = ctx->cpl.result
				      << (q->dev->lba_shift - XNVME_UNIVERSAL_SECT_SH);
		return (int)((size_t)io->iod->nr_sectors << XNVME_UNIVERSAL_SECT_SH);
	}

	if (op == UBLK_IO_OP_REPORT_ZONES) {
		return report_zones_complete(q, io);
	}

	return 0;
}

#else

int
qublk_zoned_init(struct qublk_dev *dev, int passes_nvme)
{
	if (!passes_nvme) {
		return zoned_reject(dev, "serving it needs a passthru backend");
	}

	return zoned_reject(
		dev, "this qublk was built without zoned ublk support (kernel headers < 6.6)");
}

int
qublk_zoned_check_features(const struct qublk_dev *dev, uint64_t feat)
{
	(void)dev;
	(void)feat;
	return 0;
}

void
qublk_zoned_add_failed(const struct qublk_dev *dev, int rc)
{
	(void)dev;
	(void)rc;
}

void
qublk_zoned_print_info(const struct qublk_dev *dev)
{
	(void)dev;
}

void
qublk_zoned_set_params(const struct qublk_dev *dev, struct ublk_params *params)
{
	(void)dev;
	(void)params;
}

size_t
qublk_zoned_buf_slack(const struct qublk_dev *dev)
{
	(void)dev;
	return 0;
}

int
qublk_zoned_op_is_append(uint8_t op)
{
	(void)op;
	return 0;
}

int
qublk_zoned_dispatch(struct qublk_queue *q, struct qublk_io *io, struct xnvme_cmd_ctx *ctx)
{
	(void)q;
	(void)io;
	(void)ctx;
	return -EOPNOTSUPP;
}

int
qublk_zoned_complete(struct qublk_queue *q, struct qublk_io *io, struct xnvme_cmd_ctx *ctx)
{
	(void)q;
	(void)io;
	(void)ctx;
	return 0;
}

#endif
