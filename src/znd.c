// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <libznd.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_spec.h>

/**
 * TODO: there should be a bunch of boundary checks here, e.g. slba + limit >
 * nzones * zcap... round down slba to zslba... all that jazz...
 *
 * Allocate struct 'znd_report' with memory allowing it to contain all
 * zone-entries, despite the intended 'limit'
 */
static inline struct znd_report *
znd_report_init(struct xnvme_dev *dev, uint64_t slba, size_t limit, uint8_t extended)
{
	const struct xnvme_spec_idfy_ns *nvm = (void *)xnvme_dev_get_ns(dev);
	const struct znd_idfy_ns *zns = (void *)xnvme_dev_get_ns_css(dev);
	const size_t zdext_nbytes = zns->lbafe[nvm->flbas.format].zdes * 64;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	const size_t zd_nbytes = sizeof(struct znd_descr);
	size_t nentries;
	size_t zrent_nbytes;
	size_t entries_nbytes;
	size_t report_nbytes;
	struct znd_report *report;

	if (extended && (!zdext_nbytes)) {
		XNVME_DEBUG("FAILED: device does not support extended report");
		errno = ENOSYS;
		return NULL;
	}

	// Determine number of entries
	nentries = limit ? limit : geo->nzone;
	if (nentries > geo->nzone) {
		XNVME_DEBUG("FAILED: nentries: %zu > geo->nzone: %u",
			    nentries, geo->nzone);
		errno = EINVAL;
		return NULL;
	}

	// Determine size of an entry
	zrent_nbytes = extended ? zd_nbytes + zdext_nbytes : zd_nbytes;

	entries_nbytes = nentries * zrent_nbytes;
	report_nbytes = sizeof(*report) + entries_nbytes;

	// Initialize report structure
	report = xnvme_buf_virt_alloc(0x1000, report_nbytes);
	if (!report) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc");
		return NULL;
	}
	memset(report, 0, report_nbytes);

	report->nzones = geo->nzone;
	report->extended = extended;
	report->nentries = nentries;

	report->zd_nbytes = zd_nbytes;
	report->zdext_nbytes = zdext_nbytes;
	report->zrent_nbytes = zrent_nbytes;

	report->entries_nbytes = entries_nbytes;
	report->report_nbytes = report_nbytes;

	// TODO: round to zslba, that is, do not assume that slba is a zslba
	report->zslba = slba;

	// TODO: check boundary
	report->zelba = slba + (report->nentries - 1) * geo->nsect;

	return report;
}

/**
 * At this point then dev->geo has been filled and we can rely on the derived
 * value for dev->geo.nzone instead of probing for it here.
 *
 * As such, we can choose to skip the header for the log, or defensively fetch
 * and verify that is matches dev->geo.nzone.
 *
 * zd_nbytes: Zone Descriptor Size in BYTES
 * zdext_nbytes: Zone Descriptor Extension Size in BYTES
 * zrent_nbytes: Size of an entry in the report, that is, descr + ext
 */
struct znd_report *
znd_report_from_dev(struct xnvme_dev *dev, uint64_t slba, size_t limit, uint8_t extended)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	const uint32_t nsid = xnvme_dev_get_nsid(dev);
	size_t dbuf_nentries_max;
	size_t dbuf_nbytes;
	void *dbuf;

	struct znd_report *report = NULL;
	enum znd_recv_action action;

	struct xnvme_req req = { 0 };
	int err;

	report = znd_report_init(dev, slba, limit, extended);
	if (!report) {
		XNVME_DEBUG("FAILED: znd_report_init()");
		return NULL;
	}

	dbuf_nentries_max = (geo->mdts_nbytes / report->zrent_nbytes) + 2;
	do {
		--dbuf_nentries_max;

		dbuf_nbytes = report->zrent_nbytes * dbuf_nentries_max;
		dbuf_nbytes += sizeof(*report);
	} while ((dbuf_nbytes > geo->mdts_nbytes) && dbuf_nentries_max);

	XNVME_DEBUG("INFO: dbuf_nentries_max: %zu", dbuf_nentries_max);
	XNVME_DEBUG("INFO: dbuf_nbytes: %zu", dbuf_nbytes);

	if (!dbuf_nentries_max) {
		xnvme_buf_virt_free(report);
		return NULL;
	}

	// Allocate device buffer for mgmt-recv commands
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes, NULL);
	if (!dbuf) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		xnvme_buf_virt_free(report);
		return NULL;
	}

	// Determine Management Receive Action
	action = report->extended ? ZND_RECV_EREPORT : ZND_RECV_REPORT;

	for (uint64_t zslba = report->zslba; zslba <= report->zelba;) {
		struct znd_rprt_hdr *hdr = (void *)dbuf;
		size_t nentries = XNVME_MIN(dbuf_nentries_max,
					    ((report->zelba - zslba) / geo->nsect) + 1);

		XNVME_DEBUG("INFO: nentries: %zu", nentries);
		if ((!nentries) || (nentries > dbuf_nentries_max)) {
			XNVME_DEBUG("ERR: invalid nentries");
			break;
		}

		err = znd_cmd_mgmt_recv(dev, nsid, zslba, action,
					ZND_RECV_SF_ALL, 0x0, dbuf, dbuf_nbytes,
					XNVME_CMD_SYNC, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			XNVME_DEBUG("FAILED: znd_cmd_mgmt_recv()");
			xnvme_buf_virt_free(report);
			xnvme_buf_free(dev, dbuf);
			errno = err ? -err : EIO;
			return NULL;
		}
		XNVME_DEBUG("INFO: hdr->nzones: %zu", hdr->nzones);

		if (nentries > hdr->nzones) {
			XNVME_DEBUG("ERR: invalid nentries");
			break;
		}

		// Skip the header and copy the remainder
		memcpy(report->storage + \
		       ((zslba - slba) / geo->nsect) * report->zrent_nbytes,
		       ((uint8_t *)dbuf) + sizeof(struct znd_rprt_hdr),
		       report->zrent_nbytes * nentries);

		zslba += nentries * geo->nsect;
	}

	xnvme_buf_free(dev, dbuf);

	return report;
}

int
znd_report_find_arbitrary(const struct znd_report *report, enum znd_state state,
			  uint64_t *zlba, int opts)
{
	uint64_t arb;

	srand(opts ? opts : time(NULL));
	arb = rand() % UINT32_MAX;

	for (uint32_t ci = 0; ci < report->nentries; ++ci) {
		const uint64_t idx = (arb++) % ((uint64_t)report->nentries);
		struct znd_descr *zdescr = NULL;

		zdescr = ZND_REPORT_DESCR(report, idx);

		if ((zdescr->zs == state) && \
		    (zdescr->zt == ZND_TYPE_SEQWR) && \
		    (zdescr->zcap)) {
			*zlba = zdescr->zslba;
			return 0;
		}
	}

	return -ENXIO;
}

int
znd_descr_from_dev(struct xnvme_dev *dev, uint64_t slba,
		   struct znd_descr *zdescr)
{
	const uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct xnvme_req req = { 0 };
	struct znd_rprt_hdr *hdr;
	size_t dbuf_nbytes;
	void *dbuf;
	int err;

	dbuf_nbytes = sizeof(*hdr) + sizeof(*zdescr);
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes, NULL);
	if (!dbuf) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		return -errno;
	}
	memset(dbuf, 0, dbuf_nbytes);

	err = znd_cmd_mgmt_recv(dev, nsid, slba, ZND_RECV_REPORT,
				ZND_RECV_SF_ALL, 0x0, dbuf, dbuf_nbytes,
				XNVME_CMD_SYNC, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		XNVME_DEBUG("FAILED: znd_cmd_mgmt_recv()");
		err = err ? err : -EIO;
		goto exit;
	}

	hdr = (void *)dbuf;
	if (!hdr->nzones) {
		XNVME_DEBUG("FAILED: hdr->nzones: %zu", hdr->nzones);
		err = -EIO;
		goto exit;
	}
	memcpy(zdescr, dbuf + sizeof(*hdr), sizeof(*zdescr));

exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

int
znd_descr_from_dev_in_state(struct xnvme_dev *dev, enum znd_state state,
			    struct znd_descr *zdescr)
{
	const uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct xnvme_req req = { 0 };
	struct znd_rprt_hdr *hdr;
	size_t dbuf_nbytes;
	void *dbuf;
	uint8_t sfield;
	int err;

	switch (state) {
	case ZND_STATE_EMPTY:
		sfield = ZND_RECV_SF_EMPTY;
		break;
	case ZND_STATE_IOPEN:
		sfield = ZND_RECV_SF_IOPEN;
		break;
	case ZND_STATE_EOPEN:
		sfield = ZND_RECV_SF_EOPEN;
		break;
	case ZND_STATE_CLOSED:
		sfield = ZND_RECV_SF_CLOSED;
		break;
	case ZND_STATE_FULL:
		sfield = ZND_RECV_SF_FULL;
		break;
	case ZND_STATE_RONLY:
		sfield = ZND_RECV_SF_RONLY;
		break;
	case ZND_STATE_OFFLINE:
		sfield = ZND_RECV_SF_OFFLINE;
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported state: %d", state);
		return -EINVAL;
	}

	dbuf_nbytes = sizeof(*hdr) + sizeof(*zdescr);
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes, NULL);
	if (!dbuf) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc(%zu)", dbuf_nbytes);
		return -errno;
	}
	memset(dbuf, 0, dbuf_nbytes);

	err = znd_cmd_mgmt_recv(dev, nsid, 0x0, ZND_RECV_REPORT, sfield, 0x1,
				dbuf, dbuf_nbytes, XNVME_CMD_SYNC, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		XNVME_DEBUG("FAILED: znd_cmd_mgmt_recv()");
		err = err ? err : -EIO;
		goto exit;
	}

	hdr = (void *)dbuf;
	if (!hdr->nzones) {
		XNVME_DEBUG("FAILED: hdr->nzones: %zu", hdr->nzones);
		err = -EIO;
		goto exit;
	}
	memcpy(zdescr, dbuf + sizeof(*hdr), sizeof(*zdescr));

exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

int
znd_stat_dev(struct xnvme_dev *dev, enum znd_recv_action_sf sfield,
	     uint64_t *nzones)
{
	const uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct xnvme_req req = { 0 };
	struct znd_rprt_hdr *hdr;
	const size_t hdr_nbytes = sizeof(*hdr);
	;
	int err;

	hdr = xnvme_buf_alloc(dev, hdr_nbytes, NULL);
	if (!hdr) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		return -errno;
	}
	memset(hdr, 0, hdr_nbytes);

	err = znd_cmd_mgmt_recv(dev, nsid, 0x0, ZND_RECV_REPORT, sfield, 0x0,
				hdr, hdr_nbytes, XNVME_CMD_SYNC, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		XNVME_DEBUG("FAILED: znd_cmd_mgmt_recv()");
		err = err ? err : -EIO;
		goto exit;
	}

	*nzones = hdr->nzones;

exit:
	xnvme_buf_free(dev, hdr);

	return err;
}

struct znd_changes *znd_changes_from_dev(struct xnvme_dev *dev)
{
	struct znd_changes *log = NULL;
	const size_t log_nbytes = sizeof(*log);
	struct xnvme_req req = { 0 };
	int err;

	// Allocate buffer for the maximum possible amount of log entries
	log = xnvme_buf_alloc(dev, log_nbytes, NULL);
	if (!log) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc");
		return NULL;
	}

	// Retrieve Changed Zone Information List for namespace with nsid=1
	err = xnvme_cmd_log(dev, ZND_CMD_LOG_CHANGES, 0, 0,
			    xnvme_dev_get_nsid(dev), 0, log, log_nbytes, &req);
	if (err || xnvme_req_cpl_status(&req)) {
		XNVME_DEBUG("FAILED: xnvme_cmd_log(XNVME_SPEC_LOG_TBD_ZCHG)");
		xnvme_buf_free(dev, log);
		return NULL;
	}

	return log;
}

int
znd_cmd_mgmt_send(struct xnvme_dev *dev, uint32_t nsid, uint64_t zslba,
		  enum znd_send_action action, enum znd_send_action_sf sf,
		  void *dbuf, int opts, struct xnvme_req *req)
{
	struct znd_cmd cmd = { 0 };

	uint32_t dbuf_nbytes = 0;

	cmd.common.opcode = ZND_CMD_OPC_MGMT_SEND;
	cmd.common.nsid = nsid;
	cmd.mgmt_send.slba = zslba;
	cmd.mgmt_send.zsa = action;
	cmd.mgmt_send.zsasf = sf;

	if (dbuf) {
		struct xnvme_spec_idfy_ns *nvm = (void *)xnvme_dev_get_ns(dev);
		struct znd_idfy_ns *zns = (void *)xnvme_dev_get_ns_css(dev);

		dbuf_nbytes = zns->lbafe[nvm->flbas.format].zdes * 64;
	}

	return xnvme_cmd_pass(dev, &cmd.base, dbuf, dbuf_nbytes, NULL, 0, opts,
			      req);
}

int
znd_cmd_mgmt_recv(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba,
		  enum znd_recv_action action, enum znd_recv_action_sf sf,
		  uint8_t partial, void *dbuf, uint32_t dbuf_nbytes,
		  int opts, struct xnvme_req *req)
{
	struct znd_cmd cmd = { 0 };

	cmd.common.opcode = ZND_CMD_OPC_MGMT_RECV;
	cmd.common.nsid = nsid;
	cmd.mgmt_recv.slba = slba;
	cmd.mgmt_recv.ndwords = (dbuf_nbytes >> 2) - 1;
	cmd.mgmt_recv.zra = action;
	cmd.mgmt_recv.zrasf = sf;
	cmd.mgmt_recv.partial = partial;

	return xnvme_cmd_pass(dev, &cmd.base, dbuf, dbuf_nbytes, NULL, 0, opts,
			      req);
}

int
znd_cmd_append(struct xnvme_dev *dev, uint32_t nsid, uint64_t zslba,
	       uint16_t nlb, const void *dbuf, const void *mbuf,
	       int opts, struct xnvme_req *req)
{
	void *cdbuf = (void *)dbuf;
	void *cmbuf = (void *)mbuf;

	size_t dbuf_nbytes = cdbuf ? dev->geo.lba_nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = cmbuf ? dev->geo.nbytes_oob * (nlb + 1) : 0;
	struct znd_cmd cmd = { 0 };

	// TODO: consider returning -EINVAL when mbuf is provided and namespace
	// have extended-lba in effect

	cmd.common.opcode = ZND_CMD_OPC_APPEND;
	cmd.common.nsid = nsid;
	cmd.append.zslba = zslba;
	cmd.append.nlb = nlb;

	return xnvme_cmd_pass(dev, &cmd.base, cdbuf, dbuf_nbytes, cmbuf,
			      mbuf_nbytes, opts, req);
}

const char *
znd_status_code_str(enum znd_status_code sc)
{
	switch (sc) {
	case ZND_SC_INVALID_FORMAT:
		return "ZND_SC_INVALID_FORMAT";
	case ZND_SC_BOUNDARY_ERROR:
		return "ZND_SC_BOUNDARY_ERROR";
	case ZND_SC_IS_FULL:
		return "ZND_SC_IS_FULL";
	case ZND_SC_IS_READONLY:
		return "ZND_SC_IS_READONLY";
	case ZND_SC_IS_OFFLINE:
		return "ZND_SC_IS_OFFLINE";
	case ZND_SC_INVALID_WRITE:
		return "ZND_SC_INVALID_WRITE";
	case ZND_SC_TOO_MANY_ACTIVE:
		return "ZND_SC_TOO_MANY_ACTIVE";
	case ZND_SC_TOO_MANY_OPEN:
		return "ZND_SC_TOO_MANY_OPEN";
	case ZND_SC_INVALID_TRANS:
		return "ZND_SC_INVALID_TRANS";
	}

	return "ZND_SC_ENOSYS";
}

const char *
znd_send_action_sf_str(enum znd_send_action_sf sf)
{
	switch (sf) {
	case ZND_SEND_SF_SALL:
		return "ZND_SEND_SF_SALL";
	}

	return "ZND_SEND_SF_ENOSYS";
}

const char *
znd_send_action_str(enum znd_send_action action)
{
	switch (action) {
	case ZND_SEND_CLOSE:
		return "ZND_SEND_CLOSE";
	case ZND_SEND_FINISH:
		return "ZND_SEND_FINISH";
	case ZND_SEND_OPEN:
		return "ZND_SEND_OPEN";
	case ZND_SEND_RESET:
		return "ZND_SEND_RESET";
	case ZND_SEND_OFFLINE:
		return "ZND_SEND_OFFLINE";
	case ZND_SEND_DESCRIPTOR:
		return "ZND_SEND_DESCRIPTOR";
	}

	return "ZND_SEND_ENOSYS";
}

const char *
znd_recv_action_sf_str(enum znd_recv_action_sf sf)
{
	switch (sf) {
	case ZND_RECV_SF_ALL:
		return "ZND_RECV_SF_ALL";
	case ZND_RECV_SF_EMPTY:
		return "ZND_RECV_SF_EMPTY";
	case ZND_RECV_SF_IOPEN:
		return "ZND_RECV_SF_IOPEN";
	case ZND_RECV_SF_EOPEN:
		return "ZND_RECV_SF_EOPEN";
	case ZND_RECV_SF_CLOSED:
		return "ZND_RECV_SF_CLOSED";
	case ZND_RECV_SF_FULL:
		return "ZND_RECV_SF_FULL";
	case ZND_RECV_SF_RONLY:
		return "ZND_RECV_SF_RONLY";
	case ZND_RECV_SF_OFFLINE:
		return "ZND_RECV_SF_OFFLINE";
	}

	return "ZND_RECV_SF_ENOSYS";
}

const char *
znd_recv_action_str(enum znd_recv_action action)
{
	switch (action) {
	case ZND_RECV_REPORT:
		return "ZND_RECV_REPORT";
	case ZND_RECV_EREPORT:
		return "ZND_RECV_EREPORT";
	}

	return "ZND_RECV_ENOSYS";
}

const char *
znd_state_str(enum znd_state state)
{
	switch (state) {
	case ZND_STATE_EMPTY:
		return "ZND_STATE_EMPTY";
	case ZND_STATE_IOPEN:
		return "ZND_STATE_IOPEN";
	case ZND_STATE_EOPEN:
		return "ZND_STATE_EOPEN";
	case ZND_STATE_CLOSED:
		return "ZND_STATE_CLOSED";
	case ZND_STATE_RONLY:
		return "ZND_STATE_RONLY";
	case ZND_STATE_FULL:
		return "ZND_STATE_FULL";
	case ZND_STATE_OFFLINE:
		return "ZND_STATE_OFFLINE";
	}

	return "ZND_STATE_ENOSYS";
}

static int
znd_descr_fpr_yaml(FILE *stream, const struct znd_descr *descr, int indent,
		   const char *sep)
{
	int wrtn = 0;

	wrtn += fprintf(stream, "%*szslba: 0x%016lx%s", indent, "",
			descr->zslba, sep);

	wrtn += fprintf(stream, "%*swp: 0x%016lx%s", indent, "",
			descr->wp, sep);

	wrtn += fprintf(stream, "%*szcap: %zu%s", indent, "",
			descr->zcap, sep);

	wrtn += fprintf(stream, "%*szt: %#x%s", indent, "",
			descr->zt, sep);

	wrtn += fprintf(stream, "%*szs: %*s%s", indent, "",
			17, znd_state_str(descr->zs), sep);

	wrtn += fprintf(stream, "%*sza: '0b"XNVME_I8_FMT"'", indent, "",
			XNVME_I8_TO_STR(descr->za.val));

	return wrtn;
}

int
znd_descr_fpr(FILE *stream, const struct znd_descr *descr, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "znd_descr:");
	if (!descr) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += znd_descr_fpr_yaml(stream, descr, 2, "\n");
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
znd_descr_pr(const struct znd_descr *descr, int opts)
{
	return znd_descr_fpr(stdout, descr, opts);
}

int
znd_changes_fpr(FILE *stream, const struct znd_changes *changes, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "znd_changes:");
	if (!changes) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  nidents: %u\n", changes->nidents);

	if (changes->nidents) {
		return wrtn;
	}

	wrtn += fprintf(stream, "  idents:");
	if (!changes->nidents) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	for (uint16_t idx = 0; idx < changes->nidents; ++idx) {
		wrtn += fprintf(stream, "    - 0x%016lx\n",
				changes->idents[idx]);
	}

	return wrtn;
}

int
znd_changes_pr(const struct znd_changes *changes, int opts)
{
	return znd_changes_fpr(stdout, changes, opts);
}

int
znd_rprt_hdr_fpr(FILE *stream, const struct znd_rprt_hdr *hdr, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "znd_rprt_hdr:");
	if (!hdr) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  nzones: %zu\n", hdr->nzones);
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
znd_rprt_hdr_pr(const struct znd_rprt_hdr *hdr, int opts)
{
	return znd_rprt_hdr_fpr(stdout, hdr, opts);
}

int
znd_report_fpr(FILE *stream, const struct znd_report *report, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "znd_report:\n");
	if (!report) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "  report_nbytes: %zu\n", report->report_nbytes);
	wrtn += fprintf(stream, "  entries_nbytes: %zu\n",
			report->entries_nbytes);

	wrtn += fprintf(stream, "  zd_nbytes: %d\n", report->zd_nbytes);
	wrtn += fprintf(stream, "  zdext_nbytes: %d\n", report->zdext_nbytes);
	wrtn += fprintf(stream, "  zrent_nbytes: %zu\n", report->zrent_nbytes);

	wrtn += fprintf(stream, "  zslba: 0x%016lx\n", report->zslba);
	wrtn += fprintf(stream, "  zelba: 0x%016lx\n", report->zelba);

	wrtn += fprintf(stream, "  nzones: %zu\n", report->nzones);
	wrtn += fprintf(stream, "  nentries: %u\n", report->nentries);
	wrtn += fprintf(stream, "  extended: %u\n", report->extended);

	wrtn += fprintf(stream, "  entries:");

	if (!report->nentries) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	for (uint32_t idx = 0; idx < report->nentries; ++idx) {
		struct znd_descr *descr = ZND_REPORT_DESCR(report, idx);

		wrtn += fprintf(stream, "    - {");
		wrtn += znd_descr_fpr_yaml(stream, descr, 0, ", ");
		wrtn += fprintf(stream, "}\n");
	}

	return wrtn;
}

int
znd_report_pr(const struct znd_report *report, int opts)
{
	return znd_report_fpr(stdout, report, opts);
}

int
znd_idfy_ctrlr_fpr(FILE *stream, struct znd_idfy_ctrlr *zctrlr, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "znd_idfy_ctrlr:");
	if (!zctrlr) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  zasl: %d\n", zctrlr->zasl);

	return wrtn;
}

int
znd_idfy_ctrlr_pr(struct znd_idfy_ctrlr *zctrlr, int opts)
{
	return znd_idfy_ctrlr_fpr(stdout, zctrlr, opts);
}

int
znd_idfy_lbafe_fpr(FILE *stream, struct znd_idfy_lbafe *lbafe, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	if (!lbafe) {
		wrtn += fprintf(stream, "~");
		return wrtn;
	}

	wrtn += fprintf(stream, "{ zsze: %zu, zdes: %d }", lbafe->zsze, lbafe->zdes);

	return wrtn;
}

int
znd_idfy_ns_fpr(FILE *stream, struct znd_idfy_ns *zns, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;

	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;
	}

	wrtn += fprintf(stream, "znd_idfy_ns:");
	if (!zns) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  zoc: { vzcap: %d, zae: %d }\n",
			zns->zoc.vzcap, zns->zoc.zae);
	wrtn += fprintf(stream, "  ozcs: { razb: %d }\n", zns->ozcs.razb);

	wrtn += fprintf(stream, "  mar: %u\n", zns->mar);
	wrtn += fprintf(stream, "  mor: %u\n", zns->mor);

	wrtn += fprintf(stream, "  rrl: %u\n", zns->rrl);
	wrtn += fprintf(stream, "  frl: %u\n", zns->frl);

	wrtn += fprintf(stream, "  lbafe:\n");
	for (int nfmt = 0; nfmt < 16; ++nfmt) {
		wrtn += fprintf(stream, "  - ");
		wrtn += znd_idfy_lbafe_fpr(stream, &zns->lbafe[nfmt], opts);
		wrtn += fprintf(stream, "\n");
	}

	return wrtn;
}

int
znd_idfy_ns_pr(struct znd_idfy_ns *zns, int opts)
{
	return znd_idfy_ns_fpr(stdout, zns, opts);
}

const char *
znd_type_str(enum znd_type zt)
{
	switch (zt) {
	case ZND_TYPE_SEQWR:
		return "ZND_TYPE_SEQR";
	}

	return "ZND_TYPE_ENOSYS";
}

const struct znd_idfy_ctrlr *
znd_get_ctrlr(struct xnvme_dev *dev)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	struct znd_idfy_ctrlr *zctrlr;

	if (geo->type != XNVME_GEO_ZONED) {
		XNVME_DEBUG("FAILED: device is not zoned");
		errno = EINVAL;
		return NULL;
	}

	zctrlr = (void *)xnvme_dev_get_ctrlr_css(dev);
	if (!zctrlr) {
		XNVME_DEBUG("FAILED: !xnvme_dev_get_ns()");
		return NULL;
	}

	return zctrlr;
}

const struct znd_idfy_ns *
znd_get_ns(struct xnvme_dev *dev)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	struct znd_idfy_ns *zns;

	if (geo->type != XNVME_GEO_ZONED) {
		XNVME_DEBUG("FAILED: device is not zoned");
		errno = EINVAL;
		return NULL;
	}

	zns = (void *)xnvme_dev_get_ns_css(dev);
	if (!zns) {
		XNVME_DEBUG("FAILED: !xnvme_dev_get_ns_css()");
		return NULL;
	}

	return zns;
}

const struct znd_idfy_lbafe *
znd_get_lbafe(struct xnvme_dev *dev)
{
	const struct xnvme_spec_idfy_ns *ns;
	const struct znd_idfy_ns *zns;
	const struct znd_idfy_lbafe *lbafe;

	ns = xnvme_dev_get_ns(dev);
	if (!ns) {
		XNVME_DEBUG("FAILED: !xnvme_de_get_ns()");
		return NULL;
	}

	zns = znd_get_ns(dev);
	if (!zns) {
		XNVME_DEBUG("FAILED: !zns_get_ns()");
		return NULL;
	}

	lbafe = &zns->lbafe[ns->flbas.format];

	return lbafe;
}
