// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <inttypes.h>
#include <string.h>

#include <xnvme_transport_base.h>
#include <xnvme_transport_debug.h>
#include <xnvme_transport_ctrlr.h>
#include <xnvme_transport_fabric.h>
#include <xnvme_transport_subsys.h>

// The log buffer holds the header and this many entries
#define TRANSPORT_DISCOVERY_MAX_ENTRIES 3

#define TRANSPORT_DEBUG_CATEGORY TRANSPORT_DEBUG_CATEGORY_DISCOVERY

static inline void
_transport_discovery_walk_log(struct xnvme_transport_ctrlr *ctrlr, void *buf)
{
	struct xnvme_spec_discovery_log_page *log_page =
		(struct xnvme_spec_discovery_log_page *)buf;
	struct xnvme_spec_discovery_log_page_entry *entry;
	struct xnvme_transport_subsys *subsys;
	uint64_t nentries;
	uint64_t i;
	int err;

	if (!log_page) {
		TRANSPORT_ERROR("FAILED: discovery log page is NULL");
		return;
	}

	TRANSPORT_DEBUG("INFO: Discovery Log Page:");
	xnvme_transport_hexdump_range(TRANSPORT_DEBUG_CATEGORY, log_page, sizeof(*log_page));

	TRANSPORT_DEBUG("INFO: Discovery Log Page Header: genctr=%" PRIu64
			", recfmt=%u, numrec=%" PRIu64 ", tdlpl=%u",
			log_page->genctr, log_page->recfmt, log_page->numrec, log_page->tdlpl);

	// NUMREC counts every record on the controller, only the requested ones were transferred
	nentries = log_page->numrec;
	if (nentries > TRANSPORT_DISCOVERY_MAX_ENTRIES) {
		TRANSPORT_ERROR("FAILED: discovery log has %" PRIu64 " entries, only %d retrieved",
				nentries, TRANSPORT_DISCOVERY_MAX_ENTRIES);
		nentries = TRANSPORT_DISCOVERY_MAX_ENTRIES;
	}

	for (i = 0; i < nentries; ++i) {
		entry = (struct xnvme_spec_discovery_log_page_entry
				 *)((char *)log_page->first +
				    i * sizeof(struct xnvme_spec_discovery_log_page_entry));
		TRANSPORT_DEBUG("INFO: Discovery Log Entry %" PRIu64 ":", i);
		xnvme_transport_hexdump_range(TRANSPORT_DEBUG_CATEGORY, entry, sizeof(*entry));

		TRANSPORT_DEBUG("INFO: Discovery Log Entry %" PRIu64
				" Details: cntlid=%u, asqsz=%u, portid=%u",
				i, entry->cntlid, entry->asqsz, entry->portid);

		// The target controls these bytes, so require a terminator before using them as a
		// string
		if (!memchr(entry->subnqn, '\0', sizeof(entry->subnqn))) {
			TRANSPORT_ERROR("FAILED: Discovery Log Entry %" PRIu64
					" has an unterminated subnqn",
					i);
			continue;
		}
		TRANSPORT_DEBUG("INFO: Discovery Log Entry %" PRIu64 " Details: subnqn=%s", i,
				entry->subnqn);

		subsys = xnvme_transport_ctrlr_find_first_subsys(ctrlr, entry->subnqn);
		if (subsys) {
			TRANSPORT_DEBUG("INFO: Found subsys for subnqn=%s", entry->subnqn);
		} else {
			TRANSPORT_DEBUG("INFO: No subsys found for subnqn=%s", entry->subnqn);

			err = xnvme_transport_subsys_create(ctrlr, entry->subnqn, &subsys);
			if (err) {
				TRANSPORT_ERROR("FAILED: xnvme_transport_subsys_create(), err: %d",
						err);
			}
		}
	}
}

int
xnvme_transport_discovery_get_log(struct xnvme_transport_ctrlr *ctrlr,
				  struct xnvme_transport_qpair *admin_qpair)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(admin_qpair->dev);
	union xnvme_spec_sqe *cmd = &ctx.cmd;
	struct xnvme_spec_cmd_log *log = &ctx.cmd.log;
	struct xnvme_spec_sgl_descriptor *sgl = &ctx.cmd.common.dptr.sgl;
	void *buf;
	void *handle;
	int err;
	uint64_t size;
	uint32_t key;
	uint32_t ndw;
	uint64_t offset = 0;

	size = sizeof(struct xnvme_spec_discovery_log_page) +
	       TRANSPORT_DISCOVERY_MAX_ENTRIES *
		       sizeof(struct xnvme_spec_discovery_log_page_entry);
	buf = calloc(1, size);
	if (!buf) {
		TRANSPORT_ERROR("FAILED: allocate buffer for discovery log");
		return -ENOMEM;
	}

	err = xnvme_transport_ctrlr_reg(ctrlr, buf, size, &handle, &key);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_ctrlr_reg(), err: %d", err);
		goto free_log_buffer;
	}
	cmd->common.opcode = XNVME_SPEC_ADM_OPC_LOG; // Get Log Page
	cmd->common.nsid = 0;                        // For discovery log, NSID is 0
	// No fused operation, SGL data transfer type
	cmd->common.flags = xnvme_bits_set(cmd->common.flags, XNVME_SPEC_CMD_COMMON_FLAGS_PSDT,
					   XNVME_SPEC_PSDT_SGL_MPTR_SGL);

	ndw = (size >> 2) - 1;
	log->cdw10 = xnvme_bits_set(log->cdw10, XNVME_SPEC_CMD_LOG_CDW10_NUMDL, ndw & 0xFFFF);
	log->numdu = (ndw >> 16) & 0xFFFF;
	TRANSPORT_DEBUG("INFO: size=%" PRIu64, size);
	TRANSPORT_DEBUG("INFO: ndw: %u", ndw);
	TRANSPORT_DEBUG("INFO: numdl: %" PRIu64 ", numdu: %u",
			xnvme_bits_get(log->cdw10, XNVME_SPEC_CMD_LOG_CDW10_NUMDL), log->numdu);
	log->cdw10 =
		xnvme_bits_set(log->cdw10, XNVME_SPEC_CMD_LOG_CDW10_LID, XNVME_SPEC_LOG_DISCOVERY);
	// Command Set Identifier
	log->cdw14 = xnvme_bits_set(log->cdw14, XNVME_SPEC_CMD_LOG_CDW14_CSI, 0);

	log->lpol = offset & 0xFFFFFFFF; // Log Page Offset Low
	log->lpou = offset >> 32;        // Log Page Offset Upper

	xnvme_transport_keyed_sgl_init(sgl, buf, size, key);
	cmd->common.flags = xnvme_bits_set(cmd->common.flags, XNVME_SPEC_CMD_COMMON_FLAGS_PSDT,
					   XNVME_SPEC_PSDT_SGL_MPTR_CONTIGUOUS);

	err = xnvme_transport_qpair_submit_sync(admin_qpair, &ctx);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_submit_sync(), err: %d", err);
		goto deregister_log_buffer_mr;
	}

	if (xnvme_cmd_ctx_cpl_status(&ctx)) {
		TRANSPORT_ERROR("FAILED: discovery log command completion status indicates error");
		err = -ECONNREFUSED;
		goto deregister_log_buffer_mr;
	}

	_transport_discovery_walk_log(ctrlr, buf);

deregister_log_buffer_mr:
	xnvme_transport_ctrlr_dereg(ctrlr, handle);
free_log_buffer:
	free(buf);

	return err;
}
