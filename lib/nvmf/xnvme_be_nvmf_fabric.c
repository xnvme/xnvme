#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <libxnvme.h>
#include <xnvme_be.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_fabric.h>

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


static int
_perform_property_get(void)
{

}

static int
_perform_property_set(void)
{

}

static int
_perform_nvme_fabric_connect(void)
{
	// do authentication if required

	// determine controller capabilities

	// determine the supposed IO set
	_perform_property_get(); // CAP.CSS.NOIOCSS
	_perform_property_get(); // CAP.CSS.IOCSS

	// host configures controller settings
	_perform_property_set(); // CC.AMS
	_perform_property_set(); // CC.MPS

	// enable controller
	_perform_property_set(); // CC.EN

	// spin on CC.EN until the controller is ready
	_perform_property_get(); // CC.EN

	// determine configuration of controller by issuing identify command specificing the
	// Identify Controller data structure (i.e., CNS 01h)

	// Host deterines any i/o command set specific configuration information

	// Host determines maximum I/O queue size using CAP.MQES

	// Host determines the number of I/O queues supported by the controlled using the response
	// fromt he set features command with the number of queues feature identifier.
}

static inline void
_encode_fabric_connect_data(struct xnvme_be_nvmf_qpair *qpair, void *buf)
{
	struct xnvme_be_nvmf_rdma_connect_data_rec *data = buf;
	
	memset(data, 0, sizeof(*data));

	/* TODO: This is not fully populated */
	data->cntlid = 0xffff; /* assume dynamic controller model for now */

	if (qpair->ctrlr->discovery_ctrlr) {
		XNVME_DEBUG("INFO: Setting subnqn to discovery NQN: %s",
				XNVME_NVMF_DISCOVERY_NQN);
		strncpy((char *)data->subnqn, XNVME_NVMF_DISCOVERY_NQN,
			sizeof(data->subnqn));
	} else {
		XNVME_DEBUG("INFO: NOT YET IMPLEMENTED: Setting subnqn to target NQN, "
				"which should be provided by the user or discovered through "
				"some other means.");
		// TODO: This should be set to the NQN of the target subsystem, which
		// should be provided by the user or discovered through some other means.
	}
}

static inline void
_send_fabric_connect_command(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_connect_data_rec *connect_data;
	struct xnvme_be_nvmf_req *req = NULL;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(qpair->ctrlr->dev);
	struct xnvme_spec_cmd *cmd = &ctx.cmd;
	struct xnvme_spec_sgl_descriptor *sgl = &cmd->fabric_connect.sgl1;
	void *handle;
	void *buffer = NULL;
	uint64_t lkey = 0;
	uint64_t rkey = 0;
	int err;

    buffer = xnvme_buf_virt_alloc(0x1000, sizeof(*connect_data));
	if (!buffer) {
		XNVME_DEBUG("FAILED: xnvme_buf_virt_alloc() for connect_data");
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return;
	}

	if (qpair->ops->reg_mr) {
		err = qpair->ops->reg_mr(qpair, buffer, sizeof(*connect_data), &handle, &lkey, &rkey);
		if (err) {
			XNVME_DEBUG("FAILED: reg_mr() for connect_data");
			goto free_data_buffer;
		}
	}

	req = xnvme_be_nvmf_req_internal_alloc(qpair->req_pool, false, (void *) &ctx); 
	if (!req) {
		XNVME_DEBUG("FAILED: could not allocate request");
		goto dereg_data_buffer;
	}

	cmd->common.opcode = 0x7f; /* Fabric command opcode */
	cmd->common.cid = req->cid;       /* TODO: This should be unique for each command */
	cmd->common.fuse = 0;      /* There are no fused fabrics commands */
	cmd->common.psdt = XNVME_SPEC_PSDT_SGL_MPTR_SGL;	

	cmd->fabric_connect.fctype = NVMF_FCTYPE_CONNECT;
	cmd->fabric_connect.recfmt = 0;
	cmd->fabric_connect.qid = qpair->attr.qid;
	cmd->fabric_connect.sqsize = qpair->attr.qsize;
	cmd->fabric_connect.kato = 0;     /* No keep-alive timeout */
	cmd->fabric_connect.nvmsetid = 0; /* Default NVM set */
	cmd->fabric_connect.cattr.connent = 0;
	cmd->fabric_connect.cattr.indivioqdels = 1;
	cmd->fabric_connect.cattr.dissqfc = 0;
	cmd->fabric_connect.cattr.prioclass = 0;

	_encode_fabric_connect_data(qpair, buffer);

	_encode_keyed_sgl(sgl, buffer, sizeof(*connect_data), rkey);

	err = xnvme_be_nvmf_qpair_send_capsule(qpair, req, cmd, sizeof(*cmd));
	if (err) {
		XNVME_DEBUG("FAILED: send_capsule() for Fabric Connect, err: %d", err);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
	}

	xnvme_be_nvmf_wait_for_completion(qpair, req);

	//_handle_fabric_connect(qpair, &ctx.cpl, sizeof(ctx.cpl));

	xnvme_be_nvmf_req_free(qpair->req_pool, req);

	if (qpair->ops->dereg_mr && handle) 
		qpair->ops->dereg_mr(qpair, handle);

	xnvme_buf_virt_free(buffer);
	return;

dereg_data_buffer:
	if (qpair->ops->dereg_mr) {
		qpair->ops->dereg_mr(qpair, handle);
	}
free_data_buffer:
	xnvme_buf_virt_free(buffer);
	qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
	return;
}
