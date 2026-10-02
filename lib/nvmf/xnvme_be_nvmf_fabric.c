#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <libxnvme.h>
#include <xnvme_be.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_qpair.h>
#include <xnvme_be_nvmf_req.h>
#include <xnvme_be_nvmf_fabric.h>
#include <xnvme_be_nvmf_spec_base.h>
#include <xnvme_be_nvmf_debug.h>

#define NVMF_DEBUG_CATEGORY NVMF_DEBUG_CATEGORY_FABRICS
#define _PROP_GET(qpair, prop, retvar, val, label)                                          \
	do {                                                                                \
		int sz = _property_size(prop);                                              \
		if (sz <= 0) {                                                              \
			NVMF_ERROR("FAILED: property_get " #prop ", invalid size: %d", sz); \
			goto label;                                                         \
		}                                                                           \
		retvar = _nvmf_fabric_prop_get(qpair, prop, (void *)val, sz);               \
		if (retvar) {                                                               \
			NVMF_ERROR("FAILED: property_get " #prop ", err: %d", retvar);      \
			goto label;                                                         \
		}                                                                           \
	} while (0)

#define _PROP_SET(qpair, prop, retvar, val, label)                                          \
	do {                                                                                \
		int sz = _property_size(prop);                                              \
		if (sz <= 0) {                                                              \
			NVMF_ERROR("FAILED: property_set " #prop ", invalid size: %d", sz); \
			goto label;                                                         \
		}                                                                           \
		retvar = _nvmf_fabric_prop_set(qpair, prop, (void *)val, sz);               \
		if (retvar) {                                                               \
			NVMF_ERROR("FAILED: property_set " #prop ", err: %d", retvar);      \
			goto label;                                                         \
		}                                                                           \
	} while (0)

// return value of 0 indicates that the property is reserved
static inline int
_property_size(uint32_t offset)
{
	switch (offset) {
	case XNVME_SPEC_FABRIC_PROP_CAP:
		return 8;
	case XNVME_SPEC_FABRIC_PROP_VS:
	case XNVME_SPEC_FABRIC_PROP_CC:
	case XNVME_SPEC_FABRIC_PROP_CSTS:
	case XNVME_SPEC_FABRIC_PROP_NSSR:
	case XNVME_SPEC_FABRIC_PROP_NSSD:
	case XNVME_SPEC_FABRIC_PROP_CRTO:
		return 4;
	case XNVME_SPEC_FABRIC_PROP_TRANSPORT:
		return 300;
	case XNVME_SPEC_FABRIC_PROP_FABRIC_VENDOR:
		// TODO: Not supported yet -- if ever
	default:
		return -EINVAL; /* reserved */
	}
}

static inline void
_encode_keyed_sgl(struct xnvme_spec_sgl_descriptor *sgl, void *buf, size_t len, uint64_t key)
{
	assert(sgl != NULL);
	assert(buf != NULL);
	assert(len < (1UL << 24)); // Ensure length fits within 24 bits for the SGL descriptor
	assert(key < (1UL << 32)); // Ensure key fits within 64 bits for the SGL descriptor

	// Set the SGL descriptor type and subtype
	sgl->keyed.type = XNVME_SPEC_SGL_DESCR_TYPE_KEYED_DATA_BLOCK;
	sgl->keyed.subtype = XNVME_SPEC_SGL_DESCR_SUBTYPE_ADDRESS;

	// set the SGL descriptor to point to the internal buffer
	sgl->addr = (uintptr_t)buf;
	sgl->keyed.len = len;
	sgl->keyed.key = key;
}

static inline void
_handle_fabric_connect_error(struct xnvme_spec_cpl *cpl,
			     struct xnvme_spec_fabric_connect_resp_cpl *connect_cpl)
{
	NVMF_ERROR("FAILED: Fabric Connect rejected, sc: %u sct: %u", cpl->status.sc,
		    cpl->status.sct);
	_xnvme_print_error_code(cpl);
	if (cpl->status.sc == 0x02) {
		NVMF_DEBUG(
			"INFO: Fabric Connect rejected due to invalid parameter, ipo: %u ips: %u",
			connect_cpl->connect_invalid.ipo, connect_cpl->connect_invalid.iattr.ips);

		if (connect_cpl->connect_invalid.iattr.ips == 0) {
			NVMF_DEBUG("INFO: Invalid parameter in submission queue entry: %u",
				    connect_cpl->connect_invalid.ipo);
		} else {
			NVMF_DEBUG("INFO: Invalid parameter in data: %u",
				    connect_cpl->connect_invalid.ipo);
			if (connect_cpl->connect_invalid.ipo < 16) {
				NVMF_DEBUG("INFO: Invalid host identifier");
			} else if (connect_cpl->connect_invalid.ipo >= 16 &&
				   connect_cpl->connect_invalid.ipo < 18) {
				NVMF_DEBUG("INFO: Invalid controller id");
			} else if (connect_cpl->connect_invalid.ipo >= 256 &&
				   connect_cpl->connect_invalid.ipo < 512) {
				NVMF_DEBUG("INFO: Invalid subsystem NQN");
			} else if (connect_cpl->connect_invalid.ipo >= 512 &&
				   connect_cpl->connect_invalid.ipo < 768) {
				NVMF_DEBUG("INFO: Invalid host NQN");
			} else {
				NVMF_DEBUG("INFO: Invalid parameter offset: %u",
					    connect_cpl->connect_invalid.ipo);
			}
		}
	}
}

static inline void
_handle_fabric_connect(struct xnvme_be_nvmf_qpair *qpair, void *buf)
{
	struct xnvme_spec_cpl *cpl = buf;
	struct xnvme_spec_fabric_connect_resp_cpl *connect_cpl =
		(struct xnvme_spec_fabric_connect_resp_cpl *)cpl;

	if (cpl->status.sc != 0) {
		_handle_fabric_connect_error(cpl, connect_cpl);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return;
	}

	qpair->cntlid = ((struct xnvme_spec_fabric_connect_resp_cpl *)cpl)->success.cntlid;

	/* TODO: Deal with authentication requirements later*/
	if (connect_cpl->success.authreq.ascr) {
		NVMF_ERROR("ERROR: Fabric Connect accepted, but authentication is "
			    "required (ascr=1)");
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
	} else if (connect_cpl->success.authreq.atr) {
		NVMF_ERROR("ERROR: Fabric Connect accepted, but authentication is "
			    "required (atr=1)");
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
	} else {
		NVMF_DEBUG("INFO: Fabric Connect accepted, cntlid: %u", qpair->cntlid);
		qpair->state = XNVME_NVMF_QPAIR_STATE_READY;
	}

	return;
}

int
_nvmf_fabric_prop_get(struct xnvme_be_nvmf_qpair *admin_qpair, uint32_t property, void *value,
		      uint32_t len)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(admin_qpair->dev);
	struct xnvme_spec_fabric_cmd *fcmd = (struct xnvme_spec_fabric_cmd *)&ctx.cmd;
	struct xnvme_spec_fabric_property_get_cmd *prop_get_cmd = &fcmd->property_get;
	struct xnvme_spec_fabric_generic_cpl *cpl;
	int err;

	ctx.cmd.common.opcode = XNVME_SPEC_FABRIC_OPC; /* Fabric command opcode */
	ctx.cmd.common.fuse = 0;                       /* There are no fused fabrics commands */

	prop_get_cmd->fctype = XNVME_SPEC_FABRIC_COMMAND_PROPERTY_GET; // property-get code
	prop_get_cmd->attrib.prs = len == 8 ? 0b001 : 0b000;           // property size
	prop_get_cmd->ofst = property;

	err = xnvme_be_nvmf_qpair_submit_internal_sync(
		admin_qpair, &ctx.cmd, sizeof(*prop_get_cmd), NULL, 0, NULL, 0, &ctx);
	if (err) {
		NVMF_ERROR("ERROR: Failed to send property get command");
		return err;
	}

	NVMF_DEBUG("INFO: Completion received for property get command");
	_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, &ctx.cpl, sizeof(ctx.cpl));
	cpl = (struct xnvme_spec_fabric_generic_cpl *)&ctx.cpl;

	if (cpl->prop_get.status.sc) {
		NVMF_ERROR("ERROR: Property get command failed with status code 0x%x",
			   cpl->prop_get.status.sc);
		return cpl->prop_get.status.sc;
	}

	if (len == 4)
		*(uint32_t *)value = cpl->prop_get.ddw;
	else if (len == 8)
		*(uint64_t *)value = cpl->prop_get.value;

	return 0;
}

int
_nvmf_fabric_prop_set(struct xnvme_be_nvmf_qpair *admin_qpair, uint32_t property, void *value,
		      uint32_t len)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(admin_qpair->dev);
	struct xnvme_spec_fabric_cmd *fcmd = (struct xnvme_spec_fabric_cmd *)&ctx.cmd;
	struct xnvme_spec_fabric_property_set_cmd *prop_set_cmd = &fcmd->property_set;
	struct xnvme_spec_fabric_generic_cpl *cpl;
	int err;

	ctx.cmd.common.opcode = XNVME_SPEC_FABRIC_OPC; /* Fabric command opcode */
	ctx.cmd.common.fuse = 0;                       /* There are no fused fabrics commands */

	prop_set_cmd->fctype = XNVME_SPEC_FABRIC_COMMAND_PROPERTY_SET; // property-set code
	prop_set_cmd->ofst = property;
	prop_set_cmd->attrib.pus = len == 8 ? 0b001 : 0b000; // 4 bytes
	if (len == 4)
		prop_set_cmd->ddw = *(uint32_t *)value;
	else if (len == 8)
		prop_set_cmd->value = *(uint64_t *)value;
	else
		return -EINVAL; // Unsupported property size

	err = xnvme_be_nvmf_qpair_submit_internal_sync(
		admin_qpair, &ctx.cmd, sizeof(*prop_set_cmd), NULL, 0, NULL, 0, &ctx);
	if (err) {
		NVMF_ERROR("ERROR: Failed to send property set command");
		return err;
	}

	cpl = (struct xnvme_spec_fabric_generic_cpl *)&ctx.cpl;
	if (cpl->prop_set.status.sc) {
		NVMF_ERROR("ERROR: Property set command failed with status code 0x%x",
			   cpl->prop_set.status.sc);
		_print_nvme_completion(&ctx.cpl);
		return cpl->prop_set.status.sc;
	}

	NVMF_DEBUG("INFO: Property set command completed successfully");
	return 0;
}

static inline void
_encode_fabric_connect_data(struct xnvme_be_nvmf_qpair *qpair, void *buf)
{
	struct xnvme_spec_fabric_connect_data *data = buf;

	memset(data, 0, sizeof(*data));

	/* TODO: This is not fully populated */
	data->cntlid = 0xffff; /* assume dynamic controller model for now */

	if (strlen(qpair->dev->ident.subnqn) == 0) {
		NVMF_DEBUG("INFO: Setting subnqn to discovery NQN: %s", XNVME_NVMF_DISCOVERY_NQN);
		strncpy((char *)data->subnqn, XNVME_NVMF_DISCOVERY_NQN, sizeof(data->subnqn));
	} else {
		strncpy((char *)data->subnqn, qpair->dev->ident.subnqn, sizeof(data->subnqn));
	}
}

int
xnvme_be_nvmf_fabric_connect(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_spec_fabric_connect_data *connect_data;
	struct xnvme_be_nvmf_req *req = NULL;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(qpair->dev);
	struct xnvme_spec_cmd *cmd = &ctx.cmd;
	struct xnvme_spec_fabric_cmd *fcmd = (struct xnvme_spec_fabric_cmd *)&ctx.cmd;
	struct xnvme_spec_sgl_descriptor *sgl = &fcmd->connect.sgl1;
	void *handle = NULL;
	void *buffer = NULL;
	uint32_t rkey = 0;
	int err;

	buffer = xnvme_buf_virt_alloc(0x1000, sizeof(*connect_data));
	if (!buffer) {
		NVMF_ERROR("FAILED: xnvme_buf_virt_alloc() for connect_data");
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return -ENOMEM;
	}

	err = xnvme_be_nvmf_ctrlr_reg(qpair->ctrlr, buffer, sizeof(*connect_data), &handle, &rkey);
	if (err) {
		NVMF_ERROR("FAILED: ctrlr_reg() for connect_data");
		err = -EIO;
		goto free_data_buffer;
	}
	NVMF_DEBUG("INFO: Fabric connect data buffer allocated at %p, size: %zu, rkey: %" PRIu32,
		   buffer, sizeof(*connect_data), rkey);

	cmd->common.opcode = XNVME_SPEC_FABRIC_OPC; /* Fabric command opcode */
	cmd->common.fuse = 0;       /* There are no fused fabrics commands */
	cmd->common.psdt = XNVME_SPEC_PSDT_SGL_MPTR_SGL;

	fcmd->connect.fctype = XNVME_SPEC_FABRIC_COMMAND_CONNECT;
	fcmd->connect.recfmt = 0;
	fcmd->connect.qid = qpair->attr.qid;
	fcmd->connect.sqsize = qpair->attr.qsize;
	fcmd->connect.kato = 0;     /* No keep-alive timeout */
	fcmd->connect.nvmsetid = 0; /* Default NVM set */
	fcmd->connect.cattr.connent = 0;
	fcmd->connect.cattr.indivioqdels = 1;
	fcmd->connect.cattr.dissqfc = 0;
	fcmd->connect.cattr.prioclass = 0;

	_encode_fabric_connect_data(qpair, buffer);

	_encode_keyed_sgl(sgl, buffer, sizeof(*connect_data), rkey);

	err = xnvme_be_nvmf_qpair_submit_internal_sync(qpair, cmd, sizeof(*cmd), NULL, 0, NULL, 0,
						       &ctx);
	if (err) {
		NVMF_ERROR("FAILED: qpair_submit_internal_sync() for Fabric Connect, err: %d",
			   err);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		goto dereg_data_buffer;
	}

	NVMF_DEBUG("INFO: Handling fabric connect completion");
	_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, &ctx.cpl, sizeof(ctx.cpl));
	_handle_fabric_connect(qpair, &ctx.cpl);

	if (qpair->state == XNVME_NVMF_QPAIR_STATE_ERROR) {
		NVMF_ERROR("FAILED: QPair in error state after fabric connect");
		err = -ECONNREFUSED;
		goto dereg_data_buffer;
	}

	xnvme_be_nvmf_ctrlr_dereg(qpair->ctrlr, handle);

	xnvme_buf_virt_free(buffer);
	return 0;

dereg_data_buffer:
	xnvme_be_nvmf_ctrlr_dereg(qpair->ctrlr, handle);
free_data_buffer:
	xnvme_buf_virt_free(buffer);
	qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
	return err;
}

#define PROP_GET(prop, val) _PROP_GET(admin_qpair, prop, err, val, shutdown_controller)
#define PROP_SET(prop, val) _PROP_SET(admin_qpair, prop, err, val, shutdown_controller)
int
xnvme_be_nvmf_fabric_enable(struct xnvme_be_nvmf_ctrlr *ctrlr,
			    struct xnvme_be_nvmf_qpair *admin_qpair)
{
	struct nvme_ctrlr_cap cap;
	struct nvme_ctrlr_cc cc;
	struct nvme_ctrlr_csts csts = {0};
	int err;

	// determine controller capabilities
	PROP_GET(XNVME_SPEC_FABRIC_PROP_CAP, &cap.raw);
	PROP_GET(XNVME_SPEC_FABRIC_PROP_CC, &cc.raw);

	NVMF_DEBUG("INFO: Controller CAP:");
	_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, &cap, sizeof(cap));

	NVMF_DEBUG("INFO: Controller CC:");
	_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, &cc, sizeof(cc));

	// determine the supposed IO set
	if (cap.noiocss)
		cc.css = 0b111;
	else if (cap.iocss)
		cc.css = 0b110;
	else if (!cap.iocss && cap.ncss)
		cc.css = 0b000;
	else {
		NVMF_ERROR("FAILED: unsupported IO command set configuration, cap.css: 0x%lx",
			    cap.noiocss | cap.iocss | cap.ncss);
		return -1;
	}

	// host configures controller settings
	if (ctrlr->discovery_ctrlr) {
		cc = (struct nvme_ctrlr_cc){0}; // Cleared for discovery controllers
	} else {
		cc.mps = 0b000; // memory page size, default to 4KiB
		cc.ams = 0b000; // round-robin
	}

	cc.en = 1; // enable controller
	PROP_SET(XNVME_SPEC_FABRIC_PROP_CC, &cc.raw);

	while (!csts.rdy) {
		PROP_GET(XNVME_SPEC_FABRIC_PROP_CSTS, &csts.raw);
	}

	NVMF_DEBUG("INFO: Controller CSTS:");
	_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, &csts, sizeof(csts));

	NVMF_DEBUG("INFO: Controller CSTS Ready: %d", csts.rdy);

	return 0;

shutdown_controller:
	return err;
}
#undef PROP_GET
#undef PROP_SET

#if 0
int 
xnvme_be_nvmf_get_discovery_log(struct xnvme_be_nvmf_ctrlr *ctrlr,
	struct xnvme_be_nvmf_qpair *admin_qpair, 
	struct xnvme_spec_discovery_log_page *log_page)
{
	int err;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(admin_qpair->dev);
	struct xnvme_spec_cmd *cmd = &ctx.cmd;
	struct xnvme_spec_cmd_log *log = &ctx.cmd.log;
	struct xnvme_be_nvmf_req *req = xnvme_be_nvmf_req_alloc(admin_qpair->req_pool, false, (void *)&ctx);
	void *buf;
	uint64_t size;
	uint64_t key;
	void *handle;

	size = sizeof(struct xnvme_spec_discovery_log_page) + 3 * sizeof(struct xnvme_spec_discovery_log_page_entry);
	buf = calloc(1, size);
	if (!buf) {
		NVMF_ERROR("FAILED: allocate buffer for discovery log");
		return -1;
	}

	xnvme_be_nvmf_ctrlr_reg(ctrlr, buf, size, &handle, &key);

	cmd->common.opcode = XNVME_SPEC_ADM_OPC_LOG; // Get Log Page
	cmd->common.nsid = 0; // For discovery log, NSID is 0
	cmd->common.cid = req->cid;
	cmd->common.fuse = 0; // No fused operation
	cmd->common.psdt = 0b10; // SGL data transfer type

	log->csi = XNVME_SPEC_CSI_NVM; // Command Set Identifier for NVM command set

	return 0;
}
#endif
