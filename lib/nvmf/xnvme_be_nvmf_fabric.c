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

// return value of 0 indicates that the property is reserved
static inline int
_fabric_property_size(uint32_t offset)
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
		return 0; // TODO: Not supported yet -- if ever
	default:
		return 0; /* reserved */
	}
}

static inline int
_discovery_controller_supported(uint32_t offset)
{
	switch (offset) {
	case XNVME_SPEC_FABRIC_PROP_CAP:
	case XNVME_SPEC_FABRIC_PROP_VS:
	case XNVME_SPEC_FABRIC_PROP_CC:
	case XNVME_SPEC_FABRIC_PROP_CSTS:
		return 1;
	default:
		return 0;
	}
}

static inline int
_io_ctrlr_supported(uint32_t offset)
{
	switch (offset) {
	case XNVME_SPEC_FABRIC_PROP_CAP:
	case XNVME_SPEC_FABRIC_PROP_VS:
	case XNVME_SPEC_FABRIC_PROP_CC:
	case XNVME_SPEC_FABRIC_PROP_CSTS:
	case XNVME_SPEC_FABRIC_PROP_NSSR:
	case XNVME_SPEC_FABRIC_PROP_NSSD:
	case XNVME_SPEC_FABRIC_PROP_CRTO:
		return 1;
	default:
		return 0;
	}
}

static inline int
_admin_ctrlr_supported(uint32_t offset)
{
	return _io_ctrlr_supported(offset); // no difference today
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
	if (cpl->status.sc = 0x02) {
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
_handle_fabric_connect(struct xnvme_be_nvmf_qpair *qpair, void *buf, size_t len)
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
xnvme_be_nvmf_fabric_prop_get(struct xnvme_be_nvmf_ctrlr *ctrlr,
			      struct xnvme_be_nvmf_qpair *admin_qpair, uint32_t property,
			      uint64_t *value)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(admin_qpair->dev);
	struct xnvme_spec_fabric_cmd *fcmd = (struct xnvme_spec_fabric_cmd *)&ctx.cmd;
	struct xnvme_spec_fabric_property_get_cmd *prop_get_cmd = &fcmd->property_get;
	struct xnvme_spec_fabric_generic_cpl *cpl;
	struct xnvme_be_nvmf_req *req;
	int prop_sz;
	int err;

	if (ctrlr->discovery_ctrlr) {
		if (!_discovery_controller_supported(property)) {
			NVMF_ERROR("ERROR: Property 0x%x not supported by discovery controller",
				    property);
			return -EINVAL;
		}
		prop_sz = _fabric_property_size(property);
	} else {
		if (!_admin_ctrlr_supported(property)) {
			NVMF_ERROR("ERROR: Property 0x%x not supported by admin controller",
				    property);
			return -EINVAL;
		}
		prop_sz = _fabric_property_size(property);
	}

	if (prop_sz == 0) {
		NVMF_DEBUG("INFO: Property 0x%x is reserved, doing nothing", property);
		*value = 0; // Set value to 0 for reserved entries
		return 0;   // Do nothing for reserved entries
	}

	req = xnvme_be_nvmf_req_internal_alloc(admin_qpair->req_pool, false, &ctx);
	if (!req) {
		NVMF_ERROR("ERROR: Failed to allocate internal request");
		return ENOSPC;
	}
	NVMF_DEBUG("INFO: Allocated internal request: cid: %u, req: %p, req->context: %p, "
		    "ctx.cmd: %p, ctx.cpl: %p",
		    req->cid, req, req->context, &ctx.cmd, &ctx.cpl);
	NVMF_DEBUG("INFO: Hexdump of request");
	_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, req, sizeof(*req));

	ctx.cmd.common.cid = req->cid;
	ctx.cmd.common.opcode = XNVME_SPEC_FABRIC_OPC; /* Fabric command opcode */
	ctx.cmd.common.fuse = 0;                       /* There are no fused fabrics commands */

	prop_get_cmd->fctype = XNVME_SPEC_FABRIC_COMMAND_PROPERTY_GET; // property-get code
	prop_get_cmd->attrib.prs = prop_sz == 8 ? 0b001 : 0b000;       // property size
	prop_get_cmd->ofst = property;

	err = xnvme_be_nvmf_qpair_submit(admin_qpair, &ctx.cmd, sizeof(*prop_get_cmd), req->cid);
	if (err) {
		NVMF_ERROR("ERROR: Failed to send property get command");
		return err;
	}

	NVMF_DEBUG("INFO: Waiting for completion of property get command");
	NVMF_DEBUG("INFO: cmd: %p, cpl: %p", &ctx.cmd, &ctx.cpl);

	xnvme_be_nvmf_wait_for_completion(admin_qpair, req);
	NVMF_DEBUG("INFO: Completion received for property get command");
	_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, &ctx.cpl, sizeof(ctx.cpl));
	cpl = (struct xnvme_spec_fabric_generic_cpl *)&ctx.cpl;
	NVMF_DEBUG("INFO: Hexdump of request");
	_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, req, sizeof(*req));

	xnvme_be_nvmf_req_free(admin_qpair->req_pool, req);

	if (cpl->prop_get.status.sc) {
		NVMF_ERROR("ERROR: Property get command failed with status code 0x%x",
			    cpl->prop_get.status.sc);
		return cpl->prop_get.status.sc;
	}

	if (prop_sz == 4)
		*value = cpl->prop_get.ddw;
	else
		*value = cpl->prop_get.value;

	return 0;
}

int
xnvme_be_nvmf_fabric_prop_set(struct xnvme_be_nvmf_ctrlr *ctrlr,
			      struct xnvme_be_nvmf_qpair *admin_qpair, uint32_t property,
			      uint64_t value)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(admin_qpair->dev);
	struct xnvme_spec_fabric_cmd *fcmd = (struct xnvme_spec_fabric_cmd *)&ctx.cmd;
	struct xnvme_spec_fabric_property_set_cmd *prop_set_cmd = &fcmd->property_set;
	struct xnvme_spec_fabric_generic_cpl *cpl;
	struct xnvme_be_nvmf_req *req;
	int prop_sz;
	int err;

	if (ctrlr->discovery_ctrlr) {
		if (!_discovery_controller_supported(property)) {
			NVMF_ERROR("ERROR: Property 0x%x not supported by discovery controller",
				    property);
			return -EINVAL;
		}
		prop_sz = _fabric_property_size(property);
	} else {
		if (!_admin_ctrlr_supported(property)) {
			NVMF_ERROR("ERROR: Property 0x%x not supported by admin controller",
				    property);
			return -EINVAL;
		}
		prop_sz = _fabric_property_size(property);
	}

	if (prop_sz == 0) {
		NVMF_DEBUG("INFO: Property 0x%x is reserved, doing nothing", property);
		return 0; // Do nothing for reserved entries
	}

	req = xnvme_be_nvmf_req_internal_alloc(admin_qpair->req_pool, false, &ctx);
	if (!req) {
		NVMF_ERROR("ERROR: Failed to allocate internal request");
		return ENOSPC;
	}

	ctx.cmd.common.cid = req->cid;
	ctx.cmd.common.opcode = XNVME_SPEC_FABRIC_OPC; /* Fabric command opcode */
	ctx.cmd.common.fuse = 0;                       /* There are no fused fabrics commands */

	prop_set_cmd->fctype = XNVME_SPEC_FABRIC_COMMAND_PROPERTY_SET; // property-set code
	prop_set_cmd->ofst = property;
	prop_set_cmd->attrib.pus = prop_sz == 8 ? 0b001 : 0b000; // 4 bytes
	if (prop_sz == 4)
		prop_set_cmd->ddw = value;
	else
		prop_set_cmd->value = value;

	err = xnvme_be_nvmf_qpair_submit(admin_qpair, &ctx.cmd, sizeof(*prop_set_cmd), req->cid);
	if (err) {
		NVMF_ERROR("ERROR: Failed to send property set command");
		return err;
	}

	xnvme_be_nvmf_wait_for_completion(admin_qpair, req);

	cpl = (struct xnvme_spec_fabric_generic_cpl *)&ctx.cpl;
	xnvme_be_nvmf_req_free(admin_qpair->req_pool, req);

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

	req = xnvme_be_nvmf_req_internal_alloc(qpair->req_pool, false, (void *)&ctx);
	if (!req) {
		NVMF_ERROR("FAILED: could not allocate request");
		err = -ENOMEM;
		goto dereg_data_buffer;
	}

	cmd->common.opcode = XNVME_SPEC_FABRIC_OPC; /* Fabric command opcode */
	cmd->common.cid = req->cid; /* TODO: This should be unique for each command */
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

	err = xnvme_be_nvmf_qpair_submit(qpair, cmd, sizeof(*cmd), req->cid);
	if (err) {
		NVMF_ERROR("FAILED: qpair_submit() for Fabric Connect, err: %d", err);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		goto dereg_data_buffer;
	}

	xnvme_be_nvmf_wait_for_completion(qpair, req);

	NVMF_DEBUG("INFO: Handling fabric connect completion");
	_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, &ctx.cpl, sizeof(ctx.cpl));
	_handle_fabric_connect(qpair, &ctx.cpl, sizeof(ctx.cpl));

	if (qpair->state == XNVME_NVMF_QPAIR_STATE_ERROR) {
		NVMF_ERROR("FAILED: QPair in error state after fabric connect");
		err = -ECONNREFUSED;
		goto dereg_data_buffer;
	}

	xnvme_be_nvmf_req_free(qpair->req_pool, req);

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

int
xnvme_be_nvmf_fabric_enable(struct xnvme_be_nvmf_ctrlr *ctrlr,
			    struct xnvme_be_nvmf_qpair *admin_qpair)
{
	struct nvme_ctrlr_cap cap;
	struct nvme_ctrlr_cc cc;
	struct nvme_ctrlr_csts csts = {0};
	uint64_t val;
	int err;

	// determine controller capabilities
	err = xnvme_be_nvmf_fabric_prop_get(ctrlr, admin_qpair, XNVME_SPEC_FABRIC_PROP_CAP, &val);
	if (err) {
		NVMF_ERROR("FAILED: get CAP, err: %d", err);
		return err;
	}
	cap.raw = val;

	err = xnvme_be_nvmf_fabric_prop_get(ctrlr, admin_qpair, XNVME_SPEC_FABRIC_PROP_CC, &val);
	if (err) {
		NVMF_ERROR("FAILED: get CC, err: %d", err);
		return err;
	}
	cc.raw = val;

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

	err = xnvme_be_nvmf_fabric_prop_set(ctrlr, admin_qpair, XNVME_SPEC_FABRIC_PROP_CC,
					    (uint64_t)cc.raw);
	if (err) {
		NVMF_ERROR("FAILED: set CC, err: %d", err);
		return err;
	}

	while (!csts.rdy) {
		csts.rdy = 0;

		err = xnvme_be_nvmf_fabric_prop_get(ctrlr, admin_qpair,
						    XNVME_SPEC_FABRIC_PROP_CSTS, &val);
		if (err) {
			XNVME_DEBUG("FAILED: get CSTS, err: %d", err);
			return err;
		}

		csts.raw = val;
	}

	NVMF_DEBUG("INFO: Controller CSTS:");
	_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, &csts, sizeof(csts));

	NVMF_DEBUG("INFO: Controller CSTS Ready: %d", csts.rdy);

	return 0;
}