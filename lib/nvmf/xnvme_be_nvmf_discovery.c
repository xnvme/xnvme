#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_debug.h>
#include <xnvme_be_nvmf_subsys.h>
#include <xnvme_be_nvmf_fabric.h>

#include <xnvme_be_nvmf_discovery.h>

#define NVMF_DEBUG_CATEGORY NVMF_DEBUG_CATEGORY_DISCOVERY

static inline void
_nvmf_discovery_walk_log(struct xnvme_be_nvmf_ctrlr *ctrlr, void *buf)
{
	struct xnvme_spec_discovery_log_page *log_page =
		(struct xnvme_spec_discovery_log_page *)buf;
	struct xnvme_spec_discovery_log_page_entry *entry;
	struct xnvme_be_nvmf_subsys *subsys;
	uint32_t i;
	int err;

	if (!log_page) {
		NVMF_ERROR("FAILED: discovery log page is NULL");
		return;
	}

	XNVME_DEBUG("INFO: Discovery Log Page:");
	_hexdump_range(NVMF_DEBUG_CATEGORY, log_page, sizeof(*log_page));

	XNVME_DEBUG("INFO: Discovery Log Page Header: genctr=%u, recfmt=%u, numrec=%u, tdlpl=%u",
		    log_page->genctr, log_page->recfmt, log_page->numrec, log_page->tdlpl);

	for (i = 0; i < log_page->numrec; ++i) {
		entry = (struct xnvme_spec_discovery_log_page_entry
				 *)((char *)log_page->first +
				    i * sizeof(struct xnvme_spec_discovery_log_page_entry));
		NVMF_DEBUG("INFO: Discovery Log Entry %d:", i);
		_hexdump_range(NVMF_DEBUG_CATEGORY, entry, sizeof(*entry));

		XNVME_DEBUG("INFO: Discovery Log Entry %d Details: cntlid=%u, asqsz=%u, portid=%u",
			    i, entry->cntlid, entry->asqsz, entry->portid);
		XNVME_DEBUG("INFO: Discovery Log Entry %d Details: subnqn=%s", i, entry->subnqn);

		subsys = xnvme_be_nvmf_ctrlr_find_first_subsys(ctrlr, entry->subnqn);
		if (subsys) {
			XNVME_DEBUG("INFO: Found subsys for subnqn=%s", entry->subnqn);
		} else {
			XNVME_DEBUG("INFO: No subsys found for subnqn=%s", entry->subnqn);

			err = xnvme_be_nvmf_subsys_create(ctrlr, entry->subnqn, &subsys);
			if (err) {
				NVMF_ERROR("FAILED: xnvme_be_nvmf_subsys_create(), err: %d", err);
			}
		}
	}
}

int
xnvme_be_nvmf_get_discovery_log(struct xnvme_be_nvmf_ctrlr *ctrlr,
				struct xnvme_be_nvmf_qpair *admin_qpair,
				struct xnvme_spec_discovery_log_page *log_page)
{
	int err;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(admin_qpair->dev);
	struct xnvme_spec_cmd *cmd = &ctx.cmd;
	struct xnvme_spec_cmd_log *log = &ctx.cmd.log;
	struct xnvme_spec_sgl_descriptor *sgl = &ctx.cmd.common.dptr.sgl;
	void *buf;
	uint64_t size;
	uint32_t key;
	void *handle;
	uint32_t ndw;
	uint64_t offset;

	size = sizeof(struct xnvme_spec_discovery_log_page) +
	       3 * sizeof(struct xnvme_spec_discovery_log_page_entry);
	buf = calloc(1, size);
	if (!buf) {
		NVMF_ERROR("FAILED: allocate buffer for discovery log");
		return -1;
	}

	err = xnvme_be_nvmf_ctrlr_reg(ctrlr, buf, size, &handle, &key);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_reg(), err: %d", err);
		free(buf);
		return err;
	}
	cmd->common.opcode = XNVME_SPEC_ADM_OPC_LOG; // Get Log Page
	cmd->common.nsid = 0;                        // For discovery log, NSID is 0
	cmd->common.fuse = 0;                        // No fused operation
	cmd->common.psdt = 0b10;                     // SGL data transfer type

	ndw = (size >> 2) - 1;
	log->numdl = ndw & 0xFFFF;
	log->numdu = (ndw >> 16) & 0xFFFF;
	XNVME_DEBUG("INFO: size=%lu", size);
	XNVME_DEBUG("INFO: ndw: %u", ndw);
	XNVME_DEBUG("INFO: log->numdl: %u, log->numdu: %u", log->numdl, log->numdu);
	log->lid = 0x70;
	log->lsp = 0x2000; // Log Specific Parameter
	log->csi = 0;      // Command Set Identifier

	xnvme_be_nvmf_keyed_sgl_init(sgl, buf, size, key);

	err = xnvme_be_nvmf_qpair_submit_internal_sync(admin_qpair, cmd, sizeof(*cmd), NULL, 0,
						       NULL, 0, &ctx);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_submit_internal_sync(), err: %d", err);
		return err;
	}

	log->lpol = offset & 0xFFFFFFFF; // Log Page Offset Low
	log->lpou = offset >> 32;        // Log Page Offset Upper

	if (xnvme_cmd_ctx_cpl_status(&ctx)) {
		NVMF_ERROR("FAILED: discovery log command completion status indicates error");
		return -ECONNREFUSED;
	}

	_nvmf_discovery_walk_log(ctrlr, buf);

	xnvme_be_nvmf_ctrlr_dereg(ctrlr, handle);
	free(buf);

	return 0;
}
