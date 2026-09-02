// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <string.h>

#include <libxnvme.h>
#include <xnvme_be.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_rdma.h>

#define NVMF_FCTYPE_CONNECT 0x01

struct xnvme_be_nvmf_rdma_connect_data_rec {
	uint8_t hostid[16];
	uint16_t cntlid;
	uint8_t rsvd[238];
	uint8_t subnqn[256];
	uint8_t hostnqn[256];
	uint8_t rsvd2[256];
};

enum xnvme_be_nvmf_fabric_command_psdt {
	XNVME_BE_NVMF_FABRIC_COMMAND_PSDT_NODATA =
		0b00, /* no data transferred, fabrics-command specific value*/
	XNVME_BE_NVMF_FABRIC_COMMAND_PSDT_SGL =
		0b10, /* data transferred via SGLs, fabrics-command specific value */
};

struct xnvme_be_nvmf_connect_response_cpl {
	union {
		uint32_t scs;
		struct {
			uint16_t cntlid;
			struct {
				uint16_t obsolete : 1; /* bit 0 */
				uint16_t atr  : 1; /* Authentication Transaction Required, bit 1 */
				uint16_t ascr : 1; /* Authentication and Secure Channel Required,
						      bit 2 */
				uint16_t rsvd3 : 13; /* bits 15:3 */

			} authreq;
		} success;
		struct {
			uint16_t ipo; /* Invalid Parameter Offset */
			struct {
				uint8_t ips   : 1; /* Invalid Parameter Start, bit 0 */
				uint8_t rsvd4 : 7; /* bits 7:1*/
			} iattr;                   /* Invalid Attributes */
			uint8_t rsvd5;
		} connect_invalid;
	};
	uint32_t rsvd1;
	uint16_t sqhd;
	uint16_t rsvd2;
	uint16_t cid;
	struct xnvme_spec_status status;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_connect_response_cpl) ==
			    sizeof(struct xnvme_spec_cpl),
		    "Incorrect size")

struct xnvme_be_nvmf_property_get_response_cpl {
	uint64_t value;
	uint16_t sqhd;
	uint16_t rsvd1;
	uint16_t cid;
	struct xnvme_spec_status status;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_property_get_response_cpl) ==
			    sizeof(struct xnvme_spec_cpl),
		    "Incorrect size")

struct xnvme_be_nvmf_property_set_response_cpl {
	uint64_t rsvd1;
	uint16_t sqhd;
	uint16_t cid;
	struct xnvme_spec_status status;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_property_set_response_cpl) ==
			    sizeof(struct xnvme_spec_cpl),
		    "Incorrect size")

static inline void
_handle_fabric_connect_error(struct xnvme_spec_cpl *cpl,
			     struct xnvme_be_nvmf_connect_response_cpl *connect_cpl)
{
	XNVME_DEBUG("FAILED: Fabric Connect rejected, sc: %u sct: %u", cpl->status.sc,
		    cpl->status.sct);
	_xnvme_print_error_code(cpl);
	if (cpl->status.sc = 0x02) {
		XNVME_DEBUG(
			"INFO: Fabric Connect rejected due to invalid parameter, ipo: %u ips: %u",
			connect_cpl->connect_invalid.ipo, connect_cpl->connect_invalid.iattr.ips);

		if (connect_cpl->connect_invalid.iattr.ips == 0) {
			XNVME_DEBUG("INFO: Invalid parameter in submission queue entry: %u",
				    connect_cpl->connect_invalid.ipo);
		} else {
			XNVME_DEBUG("INFO: Invalid parameter in data: %u",
				    connect_cpl->connect_invalid.ipo);
			if (connect_cpl->connect_invalid.ipo < 16) {
				XNVME_DEBUG("INFO: Invalid host identifier");
			} else if (connect_cpl->connect_invalid.ipo >= 16 &&
				   connect_cpl->connect_invalid.ipo < 18) {
				XNVME_DEBUG("INFO: Invalid controller id");
			} else if (connect_cpl->connect_invalid.ipo >= 256 &&
				   connect_cpl->connect_invalid.ipo < 512) {
				XNVME_DEBUG("INFO: Invalid subsystem NQN");
			} else if (connect_cpl->connect_invalid.ipo >= 512 &&
				   connect_cpl->connect_invalid.ipo < 768) {
				XNVME_DEBUG("INFO: Invalid host NQN");
			} else {
				XNVME_DEBUG("INFO: Invalid parameter offset: %u",
					    connect_cpl->connect_invalid.ipo);
			}
		}
	}
}

static inline void
_handle_fabric_connect(struct xnvme_be_nvmf_qpair *qpair, void *buf, size_t len)
{
	struct xnvme_spec_cpl *cpl = buf;
	struct xnvme_be_nvmf_connect_response_cpl *connect_cpl =
		(struct xnvme_be_nvmf_connect_response_cpl *)cpl;

	if (cpl->status.sc != 0) {
		_handle_fabric_connect_error(cpl, connect_cpl);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return;
	}

	qpair->cntlid = ((struct xnvme_be_nvmf_connect_response_cpl *)cpl)->success.cntlid;

	/* TODO: Deal with authentication requirements later*/
	if (connect_cpl->success.authreq.ascr) {
		XNVME_DEBUG("ERROR: Fabric Connect accepted, but authentication is "
				"required (ascr=1)");
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
	} else if (connect_cpl->success.authreq.atr) {
		XNVME_DEBUG("ERROR: Fabric Connect accepted, but authentication is "
				"required (atr=1)");
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
	} else {
		XNVME_DEBUG("INFO: Fabric Connect accepted, cntlid: %u", qpair->cntlid);
		qpair->state = XNVME_NVMF_QPAIR_STATE_READY;
	}

	return;
}

static inline void _handle_nvme_completion(struct xnvme_be_nvmf_qpair *qpair, void *buf, size_t len)
{
	
}

void
xnvme_be_nvmf_rdma_on_capsule_recv(struct xnvme_be_nvmf_qpair *qpair, void *buf, size_t len)
{
	struct xnvme_spec_cpl *cpl = buf;
	struct xnvme_be_nvmf_connect_response_cpl *connect_cpl =
		(struct xnvme_be_nvmf_connect_response_cpl *)cpl;
	struct xnvme_be_nvmf_req *req = NULL;
	struct xnvme_cmd_ctx *cmd_ctx = NULL;

	if (len < sizeof(*cpl)) {
		XNVME_DEBUG("FAILED: short capsule, len: %zu", len);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return;
	}

	req = xnvme_be_nvmf_req_get(qpair->req_pool, cpl->cid);
	if (!req) {
		XNVME_DEBUG("FAILED: Could not get request for wr_id index: %u", cpl->cid);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return;
	}

	req->cmpl_type = XNVME_BE_NVMF_REQ_CMPL_TYPE_RECV;
	req->status = 0;
	cmd_ctx = (struct xnvme_cmd_ctx *)&req->context;

	// copy the completion into the command context
	memcpy(&cmd_ctx->cpl, cpl, sizeof(*cpl));

	_print_nvme_completion(cpl);

	switch (qpair->state) {
	case XNVME_NVMF_QPAIR_STATE_CONNECTED:
		_handle_fabric_connect(qpair, buf, len);
		break;

	case XNVME_NVMF_QPAIR_STATE_READY:
		/* Normal I/O completions are dispatched by the layer above. */
		_handle_nvme_completion(qpair, buf, len);
		break;

	default:
		XNVME_DEBUG("FAILED: capsule in unexpected state: %d", qpair->state);
		break;
	}
}

void
xnvme_be_nvmf_rdma_on_send_cmpl(struct xnvme_be_nvmf_qpair *qpair, void *buf, int status)
{
	(void)qpair;
	(void)buf;
	if (status)
		XNVME_DEBUG("FAILED: send completed with error: %d", status);
}

void
xnvme_be_nvmf_rdma_on_state_change(struct xnvme_be_nvmf_qpair *qpair,
				   enum xnvme_nvmf_qpair_state state, void *ctx)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	struct xnvme_spec_cmd cmd = {0};
	struct xnvme_spec_sgl_descriptor *sgl;
	int err;

	(void)ctx;

	switch (state) {
	case XNVME_NVMF_QPAIR_STATE_CONNECTED:
		struct xnvme_be_nvmf_rdma_connect_data_rec *connect_data;
		struct xnvme_be_nvmf_req *req = NULL;

		req = xnvme_be_nvmf_req_alloc(qpair->req_pool); 
		if (!req) {
			XNVME_DEBUG("FAILED: could not allocate request");
			qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
			return;
		}

		sgl = &cmd.fabric_connect.sgl1;
		connect_data =
			(struct xnvme_be_nvmf_rdma_connect_data_rec *)rdma_qpair->internal.buffer;
		memset(connect_data, 0, sizeof(*connect_data));

		/* TODO: This is not fully populated */
		connect_data->cntlid = 0xffff; /* assume dynamic controller model for now */

		cmd.common.opcode = 0x7f; /* Fabric command opcode */
		cmd.common.cid = req->cid;       /* TODO: This should be unique for each command */
		cmd.common.fuse = 0;      /* There are no fused fabrics commands */
		cmd.common.psdt = XNVME_SPEC_PSDT_SGL_MPTR_SGL;

		if (qpair->ctrlr->discovery_ctrlr) {
			XNVME_DEBUG("INFO: Setting subnqn to discovery NQN: %s",
				    XNVME_NVMF_DISCOVERY_NQN);
			strncpy((char *)connect_data->subnqn, XNVME_NVMF_DISCOVERY_NQN,
				sizeof(connect_data->subnqn));
		} else {
			XNVME_DEBUG("INFO: NOT YET IMPLEMENTED: Setting subnqn to target NQN, "
				    "which should be provided by the user or discovered through "
				    "some other means.");
			// TODO: This should be set to the NQN of the target subsystem, which
			// should be provided by the user or discovered through some other means.
		}

		cmd.fabric_connect.fctype = NVMF_FCTYPE_CONNECT;
		cmd.fabric_connect.recfmt = 0;
		cmd.fabric_connect.qid = qpair->attr.qid;
		cmd.fabric_connect.sqsize = qpair->attr.qsize;
		cmd.fabric_connect.kato = 0;     /* No keep-alive timeout */
		cmd.fabric_connect.nvmsetid = 0; /* Default NVM set */
		cmd.fabric_connect.cattr.connent = 0;
		cmd.fabric_connect.cattr.indivioqdels = 1;
		cmd.fabric_connect.cattr.dissqfc = 0;
		cmd.fabric_connect.cattr.prioclass = 0;

		// Set the SGL descriptor type and subtype
		sgl->keyed.type = XNVME_SPEC_SGL_DESCR_TYPE_KEYED_DATA_BLOCK;
		sgl->keyed.subtype = XNVME_SPEC_SGL_DESCR_SUBTYPE_ADDRESS;

		// set the SGL descriptor to point to the internal buffer
		sgl->addr = (uintptr_t)rdma_qpair->internal.buffer;
		sgl->keyed.len = rdma_qpair->internal.size;
		sgl->keyed.key = rdma_qpair->internal.mr->rkey;

		err = qpair->ops->send_capsule(qpair, &cmd, sizeof(cmd));
		if (err) {
			XNVME_DEBUG("FAILED: send_capsule() for Fabric Connect, err: %d", err);
			qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		}
		break;

	case XNVME_NVMF_QPAIR_STATE_DISCONNECTED:
	case XNVME_NVMF_QPAIR_STATE_ERROR:
		break;

	default:
		break;
	}
}

struct xnvme_be_nvmf_transport g_xnvme_be_nvmf_rdma_transport = {
	.name = "rdma",
	.ops =
		{
			.create_ctrlr = xnvme_be_nvmf_create_rdma_controller,
		},
};
