// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <libxnvme.h>
#include <xnvme_be.h>

#include <xnvme_transport_base.h>
#include <xnvme_transport_spec_base.h>
#include <xnvme_transport_debug.h>
#include <xnvme_transport_ctrlr.h>
#include <xnvme_transport_qpair.h>
#include <xnvme_transport_fabric.h>

#define TRANSPORT_DEBUG_CATEGORY TRANSPORT_DEBUG_CATEGORY_FABRICS

// Transport fabric timing constants
// CAP.TO is in units of 500 ms, fall back to 5 seconds when the controller reports 0
#define TRANSPORT_FABRIC_CAP_TO_UNIT_NS (500ULL * 1000000ULL)
#define TRANSPORT_FABRIC_READY_TIMEOUT_NS (5ULL * 1000000000ULL)

// Fabrics specific status codes
#define TRANSPORT_FABRIC_SCT_COMMAND_SPECIFIC 0x1
#define TRANSPORT_FABRIC_SC_CONNECT_INVALID_PARAMS 0x82

// Minimum retry interval for ready state in nanoseconds, 100us min, 5ms max
#define TRANSPORT_FABRIC_RETRY_MIN_NS (100ULL * 1000ULL)
#define TRANSPORT_FABRIC_READY_MAX_NS (5ULL * 1000000ULL)

#define RETRY_CLAMP(val, max) ((val) > (max) ? (max) : (val))

static inline void
_transport_fabric_handle_connect(struct xnvme_transport_qpair *qpair, void *buf)
{
	struct xnvme_spec_cpl *cpl = buf;
	struct xnvme_spec_fabric_connect_resp_cpl *connect_cpl =
		(struct xnvme_spec_fabric_connect_resp_cpl *)cpl;

	if (cpl->status.sc != 0) {
		TRANSPORT_ERROR("FAILED: Fabric Connect rejected, sc: %u sct: %u", cpl->status.sc,
				cpl->status.sct);
		if (cpl->status.sct == TRANSPORT_FABRIC_SCT_COMMAND_SPECIFIC &&
		    cpl->status.sc == TRANSPORT_FABRIC_SC_CONNECT_INVALID_PARAMS) {
			TRANSPORT_ERROR("FAILED: invalid parameter, ipo: %u ips: %u",
					connect_cpl->connect_invalid.ipo,
					connect_cpl->connect_invalid.iattr.ips);
		}
		xnvme_transport_debug_print_cpl_error_code(cpl);
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
		return;
	}

	// Authentication is not supported yet, return error if required
	if (connect_cpl->success.authreq.ascr) {
		TRANSPORT_ERROR("ERROR: Fabric Connect accepted, but authentication is "
				"required (ascr=1)");
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
	} else if (connect_cpl->success.authreq.atr) {
		TRANSPORT_ERROR("ERROR: Fabric Connect accepted, but authentication is "
				"required (atr=1)");
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
	} else {
		qpair->attr.cntlid = connect_cpl->success.cntlid;
		TRANSPORT_DEBUG("INFO: Fabric Connect accepted, cntlid: %u", qpair->attr.cntlid);
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_READY;
	}

	return;
}

static int
_transport_fabric_prop_get(struct xnvme_transport_qpair *admin_qpair, uint32_t property,
			   bool is_8byte, uint64_t *value)
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

	err = xnvme_transport_qpair_submit_sync(admin_qpair, &ctx);
	if (err) {
		TRANSPORT_ERROR("FAILED: property get 0x%x, err: %d", property, err);
		return err;
	}

	if (cpl->prop_get.status.sc) {
		TRANSPORT_ERROR("FAILED: property get 0x%x, sc: 0x%x, sct: 0x%x", property,
				cpl->prop_get.status.sc, cpl->prop_get.status.sct);
		return -EIO;
	}

	*value = is_8byte ? cpl->prop_get.value : cpl->prop_get.ddw;

	return 0;
}

static inline int
_transport_fabric_prop_get32(struct xnvme_transport_qpair *admin_qpair, uint32_t property,
			     uint32_t *value)
{
	uint64_t value64 = 0;
	int err;

	err = _transport_fabric_prop_get(admin_qpair, property, false, &value64);
	*value = (uint32_t)value64;

	return err;
}

static inline int
_transport_fabric_prop_get64(struct xnvme_transport_qpair *admin_qpair, uint32_t property,
			     uint64_t *value)
{
	return _transport_fabric_prop_get(admin_qpair, property, true, value);
}

static int
_transport_fabric_prop_set32(struct xnvme_transport_qpair *admin_qpair, uint32_t property,
			     uint32_t value)
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

	err = xnvme_transport_qpair_submit_sync(admin_qpair, &ctx);
	if (err) {
		TRANSPORT_ERROR("FAILED: property set 0x%x, err: %d", property, err);
		return err;
	}

	if (cpl->prop_set.status.sc) {
		TRANSPORT_ERROR("FAILED: property set 0x%x, sc: 0x%x, sct: 0x%x", property,
				cpl->prop_set.status.sc, cpl->prop_set.status.sct);
		xnvme_transport_print_completion(&ctx.cpl);
		return -EIO;
	}

	return 0;
}

static inline void
_transport_fabric_connect_data_init(struct xnvme_transport_qpair *qpair, void *buf)
{
	struct xnvme_spec_fabric_connect_data *data = buf;

	memset(data, 0, sizeof(*data));

	data->cntlid = qpair->attr.cntlid;

	if (strlen(qpair->dev->ident.subnqn) == 0) {
		snprintf((char *)data->subnqn, sizeof(data->subnqn), "%s", XNVME_DISCOVERY_NQN);
	} else {
		snprintf((char *)data->subnqn, sizeof(data->subnqn), "%s",
			 qpair->dev->ident.subnqn);
	}

	if (qpair->dev->opts.hostnqn) {
		snprintf((char *)data->hostnqn, sizeof(data->hostnqn), "%s",
			 qpair->dev->opts.hostnqn);
	}

	TRANSPORT_DEBUG("INFO: Fabric connect data initialized with subnqn: %s, hostnqn: %s, "
			"cntlid: %u",
			data->subnqn, data->hostnqn, data->cntlid);
}

int
xnvme_transport_fabric_connect(struct xnvme_transport_qpair *qpair)
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
		TRANSPORT_ERROR("FAILED: xnvme_buf_virt_alloc() for connect_data");
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
		return -ENOMEM;
	}

	err = xnvme_transport_ctrlr_reg(qpair->ctrlr, buffer, sizeof(*connect_data), &handle,
					&rkey);
	if (err) {
		TRANSPORT_ERROR("FAILED: ctrlr_reg() for connect_data");
		err = -EIO;
		goto free_data_buffer;
	}
	TRANSPORT_DEBUG(
		"INFO: Fabric connect data buffer allocated at %p, size: %zu, rkey: %" PRIu32,
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

	_transport_fabric_connect_data_init(qpair, buffer);

	xnvme_transport_keyed_sgl_init(sgl, buffer, sizeof(*connect_data), rkey);

	err = xnvme_transport_qpair_submit_sync(qpair, &ctx);
	if (err) {
		TRANSPORT_ERROR("FAILED: qpair_submit_sync() for Fabric Connect, err: %d", err);
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
		goto dereg_data_buffer;
	}

	TRANSPORT_DEBUG("INFO: Handling fabric connect completion");
	xnvme_transport_hexdump_range(TRANSPORT_DEBUG_CATEGORY_FABRICS, &ctx.cpl, sizeof(ctx.cpl));
	_transport_fabric_handle_connect(qpair, &ctx.cpl);

	if (qpair->state == XNVME_TRANSPORT_QPAIR_STATE_ERROR) {
		TRANSPORT_ERROR("FAILED: QPair in error state after fabric connect");
		err = -ECONNREFUSED;
		goto dereg_data_buffer;
	}

	xnvme_transport_ctrlr_dereg(qpair->ctrlr, handle);

	xnvme_buf_virt_free(buffer);
	return 0;

dereg_data_buffer:
	xnvme_transport_ctrlr_dereg(qpair->ctrlr, handle);
free_data_buffer:
	xnvme_buf_virt_free(buffer);
	qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
	return err;
}

static inline void
_transport_sleep_ns(uint64_t ns)
{
	struct timespec ts;
	ts.tv_sec = ns / 1000000000;
	ts.tv_nsec = ns % 1000000000;
	nanosleep(&ts, NULL);
}

// Polls CSTS.RDY until it equals @rdy, backing off exponentially, or fails with -ETIMEDOUT
static int
_transport_fabric_wait_ready(struct xnvme_transport_qpair *admin_qpair, bool rdy,
			     uint64_t timeout_ns, struct nvme_ctrlr_csts *csts)
{
	uint64_t start = _xnvme_timer_clock_sample();
	uint64_t interval = TRANSPORT_FABRIC_RETRY_MIN_NS;
	int err;

	for (;;) {
		uint64_t elapsed;

		err = _transport_fabric_prop_get32(admin_qpair, XNVME_SPEC_FABRIC_PROP_CSTS,
						   &csts->raw);
		if (err) {
			return err;
		}

		if (!!csts->rdy == rdy) {
			return 0;
		}

		elapsed = _xnvme_timer_clock_sample() - start;
		if (elapsed > timeout_ns) {
			TRANSPORT_ERROR("FAILED: CSTS.RDY not %u after %" PRIu64 " ns", rdy,
					elapsed);
			return -ETIMEDOUT;
		}

		_transport_sleep_ns(interval);
		interval = RETRY_CLAMP(interval * 2, TRANSPORT_FABRIC_READY_MAX_NS);
	}
}

static inline uint64_t
_transport_calculate_timeout(struct nvme_ctrlr_cap cap)
{
	uint64_t timeout_ns = cap.to ? cap.to * TRANSPORT_FABRIC_CAP_TO_UNIT_NS
				     : TRANSPORT_FABRIC_READY_TIMEOUT_NS;
	timeout_ns = timeout_ns < TRANSPORT_FABRIC_READY_TIMEOUT_NS
			     ? TRANSPORT_FABRIC_READY_TIMEOUT_NS
			     : timeout_ns;
	return timeout_ns;
}

static inline int
_transport_calculate_css(struct nvme_ctrlr_cap cap, bool is_discovery, struct nvme_ctrlr_cc *cc)
{
	if (is_discovery) {
		cc->css = NVME_CC_CSS_ADMIN_ONLY_SUPPORTED;
	} else {
		if (cap.iocss) {
			cc->css = NVME_CC_CSS_ALL_IO_SUPPORTED;
		} else if (cap.ncss) {
			cc->css = NVME_CC_CSS_NVM_IO_SUPPORTED;
		} else if (cap.noiocss) {
			cc->css = NVME_CC_CSS_ADMIN_ONLY_SUPPORTED;
		} else {
			TRANSPORT_ERROR(
				"FAILED: unsupported IO command set configuration, cap.css: 0x%x",
				cap.noiocss | cap.iocss | cap.ncss);
			return -EINVAL;
		}
	}
	return 0;
}

int
xnvme_transport_fabric_enable(struct xnvme_transport_ctrlr *ctrlr,
			      struct xnvme_transport_qpair *admin_qpair, bool is_discovery)
{
	struct nvme_ctrlr_cap cap;
	struct nvme_ctrlr_cc cc;
	struct nvme_ctrlr_csts csts = {0};
	uint64_t timeout_ns;
	int err;

	// determine controller capabilities
	err = _transport_fabric_prop_get64(admin_qpair, XNVME_SPEC_FABRIC_PROP_CAP, &cap.raw);
	if (err) {
		return err;
	}

	err = _transport_fabric_prop_get32(admin_qpair, XNVME_SPEC_FABRIC_PROP_CC, &cc.raw);
	if (err) {
		return err;
	}

	TRANSPORT_DEBUG("INFO: Controller CAP:");
	xnvme_transport_hexdump_range(TRANSPORT_DEBUG_CATEGORY_FABRICS, &cap, sizeof(cap));

	TRANSPORT_DEBUG("INFO: Controller CC:");
	xnvme_transport_hexdump_range(TRANSPORT_DEBUG_CATEGORY_FABRICS, &cc, sizeof(cc));

	// calculate timeout
	timeout_ns = _transport_calculate_timeout(cap);

	// A controller that is already enabled must be left as is, as disabling it is not handled
	// here
	if (cc.en) {
		return 0;
	}

	// determine the supposed IO set
	err = _transport_calculate_css(cap, is_discovery, &cc);
	if (err) {
		return err;
	}

	// host configures controller settings
	if (is_discovery) {
		cc = (struct nvme_ctrlr_cc){0}; // Cleared for discovery controllers
	} else {
		cc.mps = 0b000; // memory page size, default to 4KiB
		cc.ams = 0b000; // round-robin

		// NVME specification allows different queue entry sizes, but
		// spdk only allows 64b for IO Submission Queue Entries
		cc.iosqes = 0b0110; // IO Submission Queue Entry Size, 64 bytes
		cc.iocqes = 0b0100; // IO Completion Queue Entry Size, 16 bytes
	}

	cc.en = 1; // enable controller
	err = _transport_fabric_prop_set32(admin_qpair, XNVME_SPEC_FABRIC_PROP_CC, cc.raw);
	if (err) {
		return err;
	}

	err = _transport_fabric_wait_ready(admin_qpair, true, timeout_ns, &csts);
	if (err) {
		return err;
	}

	TRANSPORT_DEBUG("INFO: Controller CSTS:");
	xnvme_transport_hexdump_range(TRANSPORT_DEBUG_CATEGORY_FABRICS, &csts, sizeof(csts));

	TRANSPORT_DEBUG("INFO: Controller CSTS Ready: %d", csts.rdy);

	if (is_discovery) {
		err = xnvme_transport_discovery_get_log(ctrlr, admin_qpair, NULL);
		if (err) {
			TRANSPORT_ERROR("FAILED: xnvme_transport_discovery_get_log(), err: %d",
					err);
			return err;
		}
		TRANSPORT_DEBUG("INFO: Successfully retrieved discovery log");
	}

	return 0;
}
