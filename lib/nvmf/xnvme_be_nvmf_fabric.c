// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <libxnvme.h>
#include <xnvme_be.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_qpair.h>
#include <xnvme_be_nvmf_fabric.h>
#include <xnvme_be_nvmf_spec_base.h>
#include <xnvme_be_nvmf_debug.h>

#define NVMF_DEBUG_CATEGORY NVMF_DEBUG_CATEGORY_FABRICS

static inline void
_nvmf_fabric_handle_connect(struct xnvme_be_nvmf_qpair *qpair, void *buf)
{
	struct xnvme_spec_cpl *cpl = buf;
	struct xnvme_spec_fabric_connect_resp_cpl *connect_cpl =
		(struct xnvme_spec_fabric_connect_resp_cpl *)cpl;

	if (cpl->status.sc != 0) {
		NVMF_ERROR("FAILED: Fabric Connect rejected, sc: %u sct: %u", cpl->status.sc,
			   cpl->status.sct);
		if (cpl->status.sc == 0x02) {
			NVMF_ERROR("FAILED: invalid parameter, ipo: %u ips: %u",
				   connect_cpl->connect_invalid.ipo,
				   connect_cpl->connect_invalid.iattr.ips);
		}
		xnvme_be_nvmf_debug_print_cpl_error_code(cpl);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return;
	}

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
		NVMF_DEBUG("INFO: Fabric Connect accepted, cntlid: %u", qpair->attr.cntlid);
		qpair->state = XNVME_NVMF_QPAIR_STATE_READY;
	}

	NVMF_DEBUG("INFO: Updated qpair cntlid to %u", qpair->attr.cntlid);
	qpair->attr.cntlid = connect_cpl->success.cntlid;

	return;
}

static int
_nvmf_fabric_prop_get(struct xnvme_be_nvmf_qpair *admin_qpair, uint32_t property, bool is_8byte,
		      uint64_t *value)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(admin_qpair->dev);
	struct xnvme_spec_fabric_cmd *fcmd = (struct xnvme_spec_fabric_cmd *)&ctx.cmd;
	struct xnvme_spec_fabric_generic_cpl *cpl =
		(struct xnvme_spec_fabric_generic_cpl *)&ctx.cpl;
	int err;

	ctx.cmd.common.opcode = XNVME_SPEC_FABRIC_OPC;
	fcmd->property_get.fctype = XNVME_SPEC_FABRIC_COMMAND_PROPERTY_GET;
	fcmd->property_get.attrib.prs = is_8byte ? 0b001 : 0b000;
	fcmd->property_get.ofst = property;

	err = xnvme_be_nvmf_qpair_submit_sync(admin_qpair, &ctx);
	if (err) {
		NVMF_ERROR("FAILED: property get 0x%x, err: %d", property, err);
		return err;
	}

	if (cpl->prop_get.status.sc) {
		NVMF_ERROR("FAILED: property get 0x%x, status code: 0x%x", property,
			   cpl->prop_get.status.sc);
		return cpl->prop_get.status.sc;
	}

	*value = is_8byte ? cpl->prop_get.value : cpl->prop_get.ddw;

	return 0;
}

static inline int
_nvmf_fabric_prop_get32(struct xnvme_be_nvmf_qpair *admin_qpair, uint32_t property,
			uint32_t *value)
{
	uint64_t value64 = 0;
	int err;

	err = _nvmf_fabric_prop_get(admin_qpair, property, false, &value64);
	*value = (uint32_t)value64;

	return err;
}

static inline int
_nvmf_fabric_prop_get64(struct xnvme_be_nvmf_qpair *admin_qpair, uint32_t property,
			uint64_t *value)
{
	return _nvmf_fabric_prop_get(admin_qpair, property, true, value);
}

static int
_nvmf_fabric_prop_set32(struct xnvme_be_nvmf_qpair *admin_qpair, uint32_t property, uint32_t value)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(admin_qpair->dev);
	struct xnvme_spec_fabric_cmd *fcmd = (struct xnvme_spec_fabric_cmd *)&ctx.cmd;
	struct xnvme_spec_fabric_generic_cpl *cpl =
		(struct xnvme_spec_fabric_generic_cpl *)&ctx.cpl;
	int err;

	ctx.cmd.common.opcode = XNVME_SPEC_FABRIC_OPC;
	fcmd->property_set.fctype = XNVME_SPEC_FABRIC_COMMAND_PROPERTY_SET;
	fcmd->property_set.attrib.pus = 0b000;
	fcmd->property_set.ofst = property;
	fcmd->property_set.ddw = value;

	err = xnvme_be_nvmf_qpair_submit_sync(admin_qpair, &ctx);
	if (err) {
		NVMF_ERROR("FAILED: property set 0x%x, err: %d", property, err);
		return err;
	}

	if (cpl->prop_set.status.sc) {
		NVMF_ERROR("FAILED: property set 0x%x, status code: 0x%x", property,
			   cpl->prop_set.status.sc);
		xnvme_be_nvmf_print_completion(&ctx.cpl);
		return cpl->prop_set.status.sc;
	}

	return 0;
}

static inline void
_nvmf_fabric_connect_data_init(struct xnvme_be_nvmf_qpair *qpair, void *buf)
{
	struct xnvme_spec_fabric_connect_data *data = buf;

	memset(data, 0, sizeof(*data));

	data->cntlid = qpair->attr.cntlid;

	if (strlen(qpair->dev->ident.subnqn) == 0) {
		strncpy((char *)data->subnqn, XNVME_DISCOVERY_NQN, sizeof(data->subnqn));
	} else {
		strncpy((char *)data->subnqn, qpair->dev->ident.subnqn, sizeof(data->subnqn));
	}
	NVMF_DEBUG("INFO: Fabric connect data initialized with subnqn: %s, cntlid: %u",
		   data->subnqn, data->cntlid);
}

int
xnvme_be_nvmf_fabric_connect(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_spec_fabric_connect_data *connect_data;
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
	cmd->common.fuse = 0;                       /* There are no fused fabrics commands */
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

	_nvmf_fabric_connect_data_init(qpair, buffer);

	xnvme_be_nvmf_keyed_sgl_init(sgl, buffer, sizeof(*connect_data), rkey);

	err = xnvme_be_nvmf_qpair_submit_sync(qpair, &ctx);
	if (err) {
		NVMF_ERROR("FAILED: qpair_submit_sync() for Fabric Connect, err: %d", err);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		goto dereg_data_buffer;
	}

	NVMF_DEBUG("INFO: Handling fabric connect completion");
	xnvme_be_nvmf_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, &ctx.cpl, sizeof(ctx.cpl));
	_nvmf_fabric_handle_connect(qpair, &ctx.cpl);

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

int
xnvme_be_nvmf_fabric_enable(struct xnvme_be_nvmf_ctrlr *ctrlr,
			    struct xnvme_be_nvmf_qpair *admin_qpair, bool is_discovery)
{
	struct nvme_ctrlr_cap cap;
	struct nvme_ctrlr_cc cc;
	struct nvme_ctrlr_csts csts = {0};
	int err;

	// determine controller capabilities
	err = _nvmf_fabric_prop_get64(admin_qpair, XNVME_SPEC_FABRIC_PROP_CAP, &cap.raw);
	if (err) {
		return err;
	}

	err = _nvmf_fabric_prop_get32(admin_qpair, XNVME_SPEC_FABRIC_PROP_CC, &cc.raw);
	if (err) {
		return err;
	}

	NVMF_DEBUG("INFO: Controller CAP:");
	xnvme_be_nvmf_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, &cap, sizeof(cap));

	NVMF_DEBUG("INFO: Controller CC:");
	xnvme_be_nvmf_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, &cc, sizeof(cc));

	// determine the supposed IO set
	if (cap.noiocss)
		cc.css = 0b111;
	else if (cap.iocss)
		cc.css = 0b110;
	else if (!cap.iocss && cap.ncss)
		cc.css = 0b000;
	else {
		NVMF_ERROR("FAILED: unsupported IO command set configuration, cap.css: 0x%x",
			   cap.noiocss | cap.iocss | cap.ncss);
		return -EINVAL;
	}

	// host configures controller settings
	if (is_discovery) {
		cc = (struct nvme_ctrlr_cc){0}; // Cleared for discovery controllers
	} else {
		cc.mps = 0b000; // memory page size, default to 4KiB
		cc.ams = 0b000; // round-robin

		// TODO: explore this. NVME specification allows different queue entry sizes, but
		// spdk only allows 64b for IO Submission Queue Entries
		cc.iosqes = 0b0110; // IO Submission Queue Entry Size, 64 bytes
		cc.iocqes = 0b0100; // IO Completion Queue Entry Size, 16 bytes
	}

	cc.en = 1; // enable controller
	err = _nvmf_fabric_prop_set32(admin_qpair, XNVME_SPEC_FABRIC_PROP_CC, cc.raw);
	if (err) {
		return err;
	}

	while (!csts.rdy) {
		err = _nvmf_fabric_prop_get32(admin_qpair, XNVME_SPEC_FABRIC_PROP_CSTS, &csts.raw);
		if (err) {
			return err;
		}

		// TODO: implement a timeout
	}

	NVMF_DEBUG("INFO: Controller CSTS:");
	xnvme_be_nvmf_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, &csts, sizeof(csts));

	NVMF_DEBUG("INFO: Controller CSTS Ready: %d", csts.rdy);

	if (is_discovery) {
		err = xnvme_be_nvmf_discovery_get_log(ctrlr, admin_qpair, NULL);
		if (err) {
			NVMF_ERROR("FAILED: xnvme_be_nvmf_discovery_get_log(), err: %d", err);
			return err;
		}
		NVMF_DEBUG("INFO: Successfully retrieved discovery log");
	}

	return 0;
}
