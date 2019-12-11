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
 */
static inline struct znd_report *
znd_report_init(struct xnvme_dev *dev, uint64_t slba, size_t limit)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	struct znd_idfy_ns *zns = (void *)xnvme_dev_get_ns(dev);
	const size_t zd_nbytes = sizeof(struct znd_descr);
	const size_t zdext_nbytes = zns->zonef[zns->fzsze].zdes * 64;
	const size_t zrent_nbytes = zd_nbytes + zdext_nbytes;

	uint64_t nzones = geo->nzone;
	size_t rprt_nbytes = (nzones + 1) * zrent_nbytes;
	struct znd_report *rprt = NULL;

	// Initialize report structure
	rprt = xnvme_buf_virt_alloc(0x1000, rprt_nbytes);
	if (!rprt) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc");
		return NULL;
	}
	memset(rprt, 0, rprt_nbytes);

	rprt->nzones = nzones;
	rprt->nentries = limit ? limit : nzones;

	// TODO: round to zslba
	rprt->zslba = slba;

	// TODO: check boundary
	rprt->zelba = slba + (rprt->nentries - 1) * geo->nsect;

	rprt->zd_nbytes = zd_nbytes;
	rprt->zdext_nbytes = zdext_nbytes;
	rprt->zrent_nbytes = zrent_nbytes;

	rprt->entries_nbytes = rprt->nentries * zrent_nbytes;
	rprt->report_nbytes = rprt->entries_nbytes + sizeof(*rprt);

	return rprt;
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
znd_report_from_dev(struct xnvme_dev *dev, uint64_t slba, size_t limit)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	const uint32_t nsid = xnvme_dev_get_nsid(dev);
	size_t dbuf_nbytes = geo->mdts_nbytes;
	void *dbuf = NULL;

	size_t nentries_dbuf_max = 0;

	struct znd_report *rprt = NULL;
	enum znd_recv_action action;

	struct xnvme_req req = { 0 };
	int err;

	dbuf_nbytes = 1920;	// NOTE: Harcoded to test iterations

	// Allocate device buffer for mgmt-recv commands
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes, NULL);
	if (!dbuf) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc");
		return NULL;
	}

	rprt = znd_report_init(dev, slba, limit);
	if (!rprt) {
		xnvme_buf_free(dev, dbuf);
		XNVME_DEBUG("FAILED: znd_report_init()");
		return NULL;
	}

	// Determine maximum number of entries in data-buffer
	nentries_dbuf_max = (dbuf_nbytes / rprt->zrent_nbytes) - 1;
	// Determine Management Receive Action
	action = rprt->zdext_nbytes ? ZND_RECV_EREPORT : ZND_RECV_REPORT;

	for (uint64_t zslba = rprt->zslba; zslba <= rprt->zelba;) {
		struct znd_rprt_hdr *hdr = (void *)dbuf;
		size_t nentries_expected;

		err = znd_cmd_mgmt_recv(dev, nsid, zslba, action,
					ZND_RECV_SF_ALL, 0x0, dbuf, dbuf_nbytes,
					XNVME_CMD_SYNC, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			XNVME_DEBUG("FAILED: znd_cmd_mgmt_recv()");
			xnvme_buf_virt_free(rprt);
			xnvme_buf_free(dev, dbuf);
			return NULL;
		}

		// Determine expected number of entries reported
		nentries_expected = (hdr->rmlba + 1 - zslba) / geo->nsect;

		if (!hdr->rmlba) {
			XNVME_DEBUG("FAILED: nothing reported");
			xnvme_buf_virt_free(rprt);
			xnvme_buf_free(dev, dbuf);
			return NULL;
		}
		if (hdr->rmlba < zslba) {
			XNVME_DEBUG("FAILED: boundary error");
			xnvme_buf_virt_free(rprt);
			xnvme_buf_free(dev, dbuf);
			return NULL;
		}
		if (nentries_expected > nentries_dbuf_max) {
			XNVME_DEBUG("FAILED: impossibru!");
			xnvme_buf_virt_free(rprt);
			xnvme_buf_free(dev, dbuf);
			return NULL;
		}

		// Skip the header and copy the remainder
		memcpy(rprt->storage + \
		       ((zslba - slba) / geo->nsect) * rprt->zrent_nbytes,
		       ((uint8_t *)dbuf) + rprt->zrent_nbytes,
		       rprt->zrent_nbytes * nentries_expected);

		zslba = hdr->rmlba + 1;
	}

	return rprt;
}

int
znd_report_find_arbitrary(const struct znd_report *report, enum znd_state state,
			  uint64_t *zlba, int opts)
{
	int arb = 0;

	srand(opts ? opts : time(NULL));
	arb = rand();

	for (uint64_t ci = 0; ci < report->nentries; ++ci) {
		const uint64_t idx = (ci + arb) % report->nentries;
		struct znd_descr *zdescr = NULL;

		zdescr = ZND_REPORT_DESCR(report, idx);

		if ((zdescr->zs == state) && \
		    (zdescr->zt == ZND_TYPE_SEQWR) && \
		    (zdescr->zcap)) {
			*zlba = zdescr->zslba;
			return 0;
		}
	}

	return -1;
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
		struct znd_idfy_ns *zns = (void *)xnvme_dev_get_ns(dev);
		dbuf_nbytes = zns->zonef[zns->fzsze].zdes * 64;
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
	cmd.mgmt_recv.nbytes = dbuf_nbytes;
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

	size_t dbuf_nbytes = cdbuf ? dev->geo.nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = cmbuf ? dev->geo.nbytes_oob * (nlb + 1) : 0;
	struct znd_cmd cmd = { 0 };

	cmd.common.opcode = ZND_CMD_OPC_APPEND;
	cmd.common.nsid = nsid;
	cmd.append.zslba = zslba;
	cmd.append.nlb = nlb;

	return dev->be->cmd_pass(dev, &cmd.base, cdbuf, dbuf_nbytes, cmbuf,
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

	wrtn += fprintf(stream, "%*szt: %#x%s", indent, "",
			descr->zt, sep);

	wrtn += fprintf(stream, "%*szs: %*s%s", indent, "",
			17, znd_state_str(descr->zs), sep);

	wrtn += fprintf(stream, "%*sza: '0b"XNVME_I8_FMT"'%s", indent, "",
			XNVME_I8_TO_STR(descr->za.val), sep);

	wrtn += fprintf(stream, "%*szslba: 0x%016lx%s", indent, "",
			descr->zslba, sep);

	wrtn += fprintf(stream, "%*swp: 0x%016lx%s", indent, "",
			descr->wp, sep);

	wrtn += fprintf(stream, "%*szcap: %zu", indent, "",
			descr->zcap);

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
	wrtn += fprintf(stream, "  rmlba: 0x%016lx", hdr->rmlba);
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
	wrtn += fprintf(stream, "  zrent_nbytes: %d\n", report->zrent_nbytes);

	wrtn += fprintf(stream, "  zslba: 0x%016lx\n", report->zslba);
	wrtn += fprintf(stream, "  zelba: 0x%016lx\n", report->zelba);

	wrtn += fprintf(stream, "  nzones: %zu\n", report->nzones);
	wrtn += fprintf(stream, "  nentries: %zu\n", report->nentries);

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
	wrtn += fprintf(stream, "  zamds: %d\n", zctrlr->zamds);

	return wrtn;
}

int
znd_idfy_ctrlr_pr(struct znd_idfy_ctrlr *zctrlr, int opts)
{
	return znd_idfy_ctrlr_fpr(stdout, zctrlr, opts);
}

int
znd_idfy_zonef_fpr(FILE *stream, struct znd_idfy_zonef *zonef, int opts)
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

	if (!zonef) {
		wrtn += fprintf(stream, "~");
	}

	wrtn += fprintf(stream, "{");
	wrtn += fprintf(stream, " zs: %zu, lbafs: 0x%x, zdes: %d ",
			zonef->zs, zonef->lbafs, zonef->zdes);

	wrtn += fprintf(stream, "}");

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
	wrtn += fprintf(stream, "  fzsze: %d\n", zns->fzsze);
	wrtn += fprintf(stream, "  nzonef: %d\n", zns->nzonef);
	wrtn += fprintf(stream, "  nar: %u\n", zns->nar);
	wrtn += fprintf(stream, "  nor: %u\n", zns->nor);
	wrtn += fprintf(stream, "  zal: %u\n", zns->zal);
	wrtn += fprintf(stream, "  rrl: %u\n", zns->rrl);
	wrtn += fprintf(stream, "  zoc: {vzcap: %d, ze: %d, razb: %d}\n",
			zns->zoc.vzcap, zns->zoc.ze, zns->zoc.razb);

	wrtn += fprintf(stream, "  zonef:\n");
	for (int zfi = 0; zfi < 4; ++zfi) {
		wrtn += fprintf(stream, "  - ");
		wrtn += znd_idfy_zonef_fpr(stream, &zns->zonef[zfi], opts);
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
