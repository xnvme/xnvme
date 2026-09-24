#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include <libxnvme.h>
#include <xnvme_be.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_fabric.h>

#define NVMF_FABRIC_OPCODE 0x7f

enum xnvme_be_nvmf_fabric_command_type {
	NVMF_FCTYPE_PROPERTY_SET = 0x00,
	NVMF_FCTYPE_CONNECT = 0x01,
	NVMF_FCTYPE_PROPERTY_GET = 0x04,
};

struct nvme_ctrlr_cap {
	union {
		struct {
			uint64_t mqes : 16;  // Maximum Queue Entries Supported, bits 15:0
			uint64_t cqr : 1;  // Contiguous Queues Required, bit 16
			uint64_t ams : 2;  // Arbitration Mechanism Supported bits 18:17
			uint64_t rsvd : 5;  // Reserved, bits 23:19
			uint64_t to : 8;  // Timeout, bits 31:24
			uint64_t dstrd : 4;  // Doorbell Stride, bits 35:32
			uint64_t nssrs : 1;  // NVMe Subsystem Reset Supported, bit 36
			uint64_t ncss : 1;  // NVMe Command Set Support, bit 37
			uint64_t rsvd2 : 5;  // Reserved, bits 42:38
			uint64_t iocss : 1;  // I/O Command Set Supported, bit 43
			uint64_t noiocss : 1;  // No I/O Command Set Supported, bit 44
			uint64_t bps : 1;  // Boot Partition Support, bit 45
			uint64_t cps : 2;  // Controller Power Scope, bits 47:46
			uint64_t mpsmin : 4;  // Memory Page Size Minimum, bits 51:48
			uint64_t mpsmax : 4;  // Memory Page Size Maximum, bits 55:52
			uint64_t pmrs : 1;  // Persistent Memory Region Supported, bit 56
			uint64_t cmbs : 1;  // Controller Memory Buffer Supported, bit 57
			uint64_t nsss : 1;  // NVMe Subsystem Shutdown Supported, bit 58
			uint64_t crwms : 1;  // Controller Ready with Media Support, bit 59
			uint64_t crims : 1;  // Controller Ready Independent of Media Support, bit 60
			uint64_t nsses : 1;  // NVMe Subsystem Shutdown Enhancements Supported, bit 61
			uint64_t rsvd3 : 2;  // Reserved, bits 63:62
		} __attribute__((packed));
		uint64_t raw;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct nvme_ctrlr_cap) == 8, "Incorrect size");

struct nvme_ctrlr_cc {
	union {
		struct {
			uint32_t en : 1;  // Enable, bit 0
			uint32_t rsvd : 3;  // Reserved, bits 3:1
			uint32_t css : 3;  // I/O Command Set Selected, bits 6:4
			uint32_t mps : 4;  // Memory Page Size, bits 10:7
			uint32_t ams : 3;  // Arbitration Mechanism Selected, bits 13:11
			uint32_t shn : 2;  // Shutdown Notification, bits 15:14
			uint32_t iosqes : 4;  // I/O Submission Queue Entry Size, bits 19:16
			uint32_t iocqes : 4;  // I/O Completion Queue Entry Size, bits 23:20
			uint32_t crime : 1;  // Controller Ready Independent of Media Enable, bit 24
			uint32_t rsvd3 : 7;  // Reserved, bits 31:25
		};
		uint32_t raw;
	};
}; 
XNVME_STATIC_ASSERT(sizeof(struct nvme_ctrlr_cc) == 4, "Incorrect size");

struct nvme_ctrlr_csts {
	union {
		struct {
			uint32_t rdy : 1;  // Ready, bit 0
			uint32_t cfs : 1;  // Controller Fatal Status, bit 1
			uint32_t shst : 2;  // Shutdown Status, bits 3:2
			uint32_t nssro : 1;  // NVMe Subsystem Shutdown Ready, bit 4
			uint32_t pp : 1;  // Processing Paused, bit 5
			uint32_t st : 1;  // Shutdown Type , bit 6
			uint32_t rsvd : 25;  // Reserved, bits 31:7
		};
		uint32_t raw;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct nvme_ctrlr_csts) == 4, "Incorrect size");


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

enum xnvmf_be_fabric_property {
	NVME_CTRLR_PROP_CAP = 0x0000,
	NVME_CTRLR_PROP_VS = 0x0008,
	NVME_CTRLR_PROP_CC = 0x0014,
	NVME_CTRLR_PROP_CSTS = 0x001C,
	NVME_CTRLR_PROP_NSSR = 0x0020,
	NVME_CTRLR_PROP_NSSD = 0x0064,
	NVME_CTRLR_PROP_CRTO = 0x0068,
	NVME_CTRLR_PROP_TRANSPORT = 0x1000,
	NVME_CTRLR_PROP_FABRIC_VENDOR = 0x1300,
};


// return value of 0 indicates that the property is reserved
static inline int
_fabric_property_size(uint32_t offset)
{
	switch (offset) {
	case NVME_CTRLR_PROP_CAP:
		return 8;
	case NVME_CTRLR_PROP_VS:
	case NVME_CTRLR_PROP_CC:
	case NVME_CTRLR_PROP_CSTS:
	case NVME_CTRLR_PROP_NSSR:
	case NVME_CTRLR_PROP_NSSD:
	case NVME_CTRLR_PROP_CRTO:
		return 4;
	case NVME_CTRLR_PROP_TRANSPORT:
		return 300;
	case NVME_CTRLR_PROP_FABRIC_VENDOR:
		return 0;  // TODO: Not supported yet -- if ever
	default:
		return 0; /* reserved */
	}
}

static inline int
_discovery_controller_supported(uint32_t offset)
{
	switch (offset) {
	case NVME_CTRLR_PROP_CAP:
	case NVME_CTRLR_PROP_VS:
	case NVME_CTRLR_PROP_CC:
	case NVME_CTRLR_PROP_CSTS:
		return 1;
	default:
		return 0;
	}
}

static inline int
_io_ctrlr_supported(uint32_t offset)
{
	switch (offset) {
	case NVME_CTRLR_PROP_CAP:
	case NVME_CTRLR_PROP_VS:
	case NVME_CTRLR_PROP_CC:
	case NVME_CTRLR_PROP_CSTS:
	case NVME_CTRLR_PROP_NSSR:
	case NVME_CTRLR_PROP_NSSD:
	case NVME_CTRLR_PROP_CRTO:
		return 1;
	default:
		return 0;
	}
}


static inline int
_admin_ctrlr_supported(uint32_t offset)
{
	return _io_ctrlr_supported(offset);  // no difference today
}

struct xnvme_be_nvmf_fabric_resp_status {
	union {
		struct {
			uint16_t rsvd2 : 1; // reserved, bit 0
			uint16_t sc : 8;  // status code, bits 8:1
			uint16_t sct : 3;  // status code type, bits 11:9
			uint16_t crd : 2;  // command retry delay, bits 13:12
			uint16_t m : 1;  // more, bit 14
			uint16_t dnr : 1;  // do not retry, bit 15
		};
		uint16_t raw;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_fabric_resp_status) ==
			    sizeof(struct xnvme_spec_status),
		    "Incorrect size")

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
	struct xnvme_be_nvmf_fabric_resp_status status;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_connect_response_cpl) ==
			    sizeof(struct xnvme_spec_cpl),
		    "Incorrect size")

struct xnvme_be_nvmf_fabric_resp_cqe {
	uint64_t frts;  // fabrics response type specific, bytes 7:0
	uint16_t sqhd;  // Submission Queue Head, bytes 9:8
	uint16_t rsvd;  // reserved, bytes 11:10
	uint16_t cid;  // Command Identifier, bytes 13:12
	struct xnvme_be_nvmf_fabric_resp_status sts;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_fabric_resp_cqe) ==
			    sizeof(struct xnvme_spec_cpl),
		    "Incorrect size")

struct xnvme_be_nvmf_property_get_response_cpl {
	union {
		struct {
			uint32_t ddw;
			uint32_t rsvd;
		};
		uint64_t value;
	};
	uint16_t sqhd;
	uint16_t rsvd1;
	uint16_t cid;
	struct xnvme_be_nvmf_fabric_resp_status status;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_property_get_response_cpl) ==
			    sizeof(struct xnvme_spec_cpl),
		    "Incorrect size")

struct xnvme_be_nvmf_property_set_response_cpl {
	uint64_t rsvd1;
	uint16_t sqhd;
	uint16_t rsvd2;
	uint16_t cid;
	struct xnvme_be_nvmf_fabric_resp_status status;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_property_set_response_cpl) ==
			    sizeof(struct xnvme_spec_cpl),
		    "Incorrect size")

struct xnvme_be_nvmf_fabric_generic_cpl {
	union {
		struct xnvme_spec_cpl generic;
		struct xnvme_be_nvmf_fabric_resp_cqe fabric_resp;
		struct xnvme_be_nvmf_property_get_response_cpl prop_get;
		struct xnvme_be_nvmf_property_set_response_cpl prop_set;
		struct xnvme_be_nvmf_connect_response_cpl connect;	
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_fabric_generic_cpl) ==
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
_perform_property_get(struct xnvme_be_nvmf_ctrlr *ctrlr, uint32_t property, uint64_t *value)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(ctrlr->dev);
	struct xnvme_spec_fabric_property_get_cmd *prop_get_cmd = &ctx.cmd.fabric_property_get;
	struct xnvme_be_nvmf_fabric_generic_cpl *cpl;
	struct xnvme_be_nvmf_req *req;
	int prop_sz;
	int err;


	if (ctrlr->discovery_ctrlr) {
		if (!_discovery_controller_supported(property)) {
			XNVME_DEBUG("ERROR: Property 0x%x not supported by discovery controller", property);
			return -EINVAL;
		}
		prop_sz = _fabric_property_size(property);
 	} else {
		if (!_admin_ctrlr_supported(property)) {
			XNVME_DEBUG("ERROR: Property 0x%x not supported by admin controller", property);
			return -EINVAL;
		}
		prop_sz = _fabric_property_size(property);
	}

	if (prop_sz == 0) {
		XNVME_DEBUG("INFO: Property 0x%x is reserved, doing nothing", property);
		*value = 0;  // Set value to 0 for reserved entries
		return 0;  // Do nothing for reserved entries
	}

	req = xnvme_be_nvmf_req_internal_alloc(ctrlr->admin_qpair->req_pool, false, &ctx);
	if (!req) {
		XNVME_DEBUG("ERROR: Failed to allocate internal request");
		return ENOSPC;
	}
	XNVME_DEBUG("INFO: Allocated internal request: cid: %u, req: %p, req->context: %p, ctx.cmd: %p, ctx.cpl: %p", 
		req->cid, req, req->context, &ctx.cmd, &ctx.cpl);
	XNVME_DEBUG("INFO: Hexdump of request");
	_hexdump_range(req, sizeof(*req));

	ctx.cmd.common.cid = req->cid;
	ctx.cmd.common.opcode = NVMF_FABRIC_OPCODE; /* Fabric command opcode */
	ctx.cmd.common.fuse = 0;      /* There are no fused fabrics commands */

	prop_get_cmd->fctype = NVMF_FCTYPE_PROPERTY_GET;  // property-get code
	prop_get_cmd->attrib.prs = prop_sz == 8 ? 0b001 : 0b000; // property size
	prop_get_cmd->ofst = property;

	err = xnvme_be_nvmf_qpair_send_capsule(ctrlr->admin_qpair, req, &ctx.cmd, sizeof(*prop_get_cmd));
	if (err) {
		XNVME_DEBUG("ERROR: Failed to send property get command");
		return err;
	}

	XNVME_DEBUG("INFO: Waiting for completion of property get command");
	XNVME_DEBUG("INFO: cmd: %p, cpl: %p", &ctx.cmd, &ctx.cpl);

	xnvme_be_nvmf_wait_for_completion(ctrlr->admin_qpair, req);
	XNVME_DEBUG("INFO: Completion received for property get command");
	_hexdump_range(&ctx.cpl, sizeof(ctx.cpl));
	cpl = (struct xnvme_be_nvmf_fabric_generic_cpl *) &ctx.cpl;
	XNVME_DEBUG("INFO: Hexdump of request");
	_hexdump_range(req, sizeof(*req));

	xnvme_be_nvmf_req_free(ctrlr->admin_qpair->req_pool, req);	

	if (cpl->prop_get.status.sc) {
		XNVME_DEBUG("ERROR: Property get command failed with status code 0x%x", cpl->prop_get.status.sc);
		return cpl->prop_get.status.sc;
	}

	if (prop_sz == 4)
		*value = cpl->prop_get.ddw;
	else
		*value = cpl->prop_get.value;

	return 0;
}

static int
_perform_property_set(struct xnvme_be_nvmf_ctrlr *ctrlr, uint32_t property, uint64_t value)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(ctrlr->dev);
	struct xnvme_spec_fabric_property_set_cmd *prop_set_cmd = &ctx.cmd.fabric_property_set;
	struct xnvme_be_nvmf_fabric_generic_cpl *cpl;
	struct xnvme_be_nvmf_req *req;
	int prop_sz; 
	int err;

	if (ctrlr->discovery_ctrlr) {
		if (!_discovery_controller_supported(property)) {
			XNVME_DEBUG("ERROR: Property 0x%x not supported by discovery controller", property);
			return -EINVAL;
		}
		prop_sz = _fabric_property_size(property);
 	} else {
		if (!_admin_ctrlr_supported(property)) {
			XNVME_DEBUG("ERROR: Property 0x%x not supported by admin controller", property);
			return -EINVAL;
		}
		prop_sz = _fabric_property_size(property);
	}

	if (prop_sz == 0) {
		XNVME_DEBUG("INFO: Property 0x%x is reserved, doing nothing", property);
		return 0;  // Do nothing for reserved entries
	}

	req = xnvme_be_nvmf_req_internal_alloc(ctrlr->admin_qpair->req_pool, false, &ctx);
	if (!req) {
		XNVME_DEBUG("ERROR: Failed to allocate internal request");
		return ENOSPC;
	}

	ctx.cmd.common.cid = req->cid;
	ctx.cmd.common.opcode = NVMF_FABRIC_OPCODE; /* Fabric command opcode */
	ctx.cmd.common.fuse = 0;      /* There are no fused fabrics commands */

	prop_set_cmd->fctype = NVMF_FCTYPE_PROPERTY_SET;  // property-set code
	prop_set_cmd->ofst = property;
	prop_set_cmd->attrib.pus = prop_sz == 8 ? 0b001 : 0b000; // 4 bytes
	if (prop_sz == 4)
		prop_set_cmd->ddw = value;
	else
		prop_set_cmd->value = value;
	
	err = xnvme_be_nvmf_qpair_send_capsule(ctrlr->admin_qpair, req, &ctx.cmd, sizeof(*prop_set_cmd));
	if (err) {
		XNVME_DEBUG("ERROR: Failed to send property set command");
		return err;
	}

	xnvme_be_nvmf_wait_for_completion(ctrlr->admin_qpair, req);

	cpl = (struct xnvme_be_nvmf_fabric_generic_cpl *) &ctx.cpl;
	xnvme_be_nvmf_req_free(ctrlr->admin_qpair->req_pool, req);

	if (cpl->prop_set.status.sc) {
		XNVME_DEBUG("ERROR: Property set command failed with status code 0x%x", cpl->prop_set.status.sc);
		_print_nvme_completion(&ctx.cpl);
		return cpl->prop_set.status.sc;
	}

	XNVME_DEBUG("INFO: Property set command completed successfully");
	return 0;
}

static inline void
_encode_fabric_connect_data(struct xnvme_be_nvmf_qpair *qpair, void *buf)
{
	struct xnvme_be_nvmf_rdma_connect_data_rec *data = buf;
	
	memset(data, 0, sizeof(*data));

	/* TODO: This is not fully populated */
	data->cntlid = 0xffff; /* assume dynamic controller model for now */

	if (strlen(qpair->ctrlr->dev->ident.subnqn) == 0) {
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

int
xnvme_be_nvmf_send_fabric_connect_command(struct xnvme_be_nvmf_qpair *qpair)
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
		return -ENOMEM;
	}

	if (qpair->ops->reg_mr) {
		err = qpair->ops->reg_mr(qpair, buffer, sizeof(*connect_data), &handle, &lkey, &rkey);
		if (err) {
			XNVME_DEBUG("FAILED: reg_mr() for connect_data");
			err = -EIO;
			goto free_data_buffer;
		}
	}
	XNVME_DEBUG("INFO: Fabric connect data buffer allocated at %p, size: %zu, lkey: %" PRIu64 ", rkey: %" PRIu64, buffer, sizeof(*connect_data), lkey, rkey);

	req = xnvme_be_nvmf_req_internal_alloc(qpair->req_pool, false, (void *) &ctx); 
	if (!req) {
		XNVME_DEBUG("FAILED: could not allocate request");
		err = -ENOMEM;
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
		goto dereg_data_buffer;
	}

	xnvme_be_nvmf_wait_for_completion(qpair, req);


	XNVME_DEBUG("INFO: Handling fabric connect completion");
	_hexdump_range(&ctx.cpl, sizeof(ctx.cpl));
	_handle_fabric_connect(qpair, &ctx.cpl, sizeof(ctx.cpl));

	xnvme_be_nvmf_req_free(qpair->req_pool, req);

	if (qpair->ops->dereg_mr && handle) 
		qpair->ops->dereg_mr(qpair, handle);

	xnvme_buf_virt_free(buffer);
	return 0;

dereg_data_buffer:
	if (qpair->ops->dereg_mr) {
		qpair->ops->dereg_mr(qpair, handle);
	}
free_data_buffer:
	xnvme_buf_virt_free(buffer);
	qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
	return err;
}


int
xnvme_be_nvmf_initialize_remote_ctrlr(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	struct nvme_ctrlr_cap cap;
	struct nvme_ctrlr_cc cc;
	struct nvme_ctrlr_csts csts = {0};

	uint64_t val;
	int err;

	// determine controller capabilities
	err = _perform_property_get(ctrlr, NVME_CTRLR_PROP_CAP, &val);
	if (err) {
		XNVME_DEBUG("FAILED: get CAP, err: %d", err);
		return err;
	}
	cap.raw = val;

	err = _perform_property_get(ctrlr, NVME_CTRLR_PROP_CC, &val);
	if (err) {
		XNVME_DEBUG("FAILED: get CC, err: %d", err);
		return err;
	}
	cc.raw = val;

	XNVME_DEBUG("INFO: Controller CAP:");
	_hexdump_range(&cap, sizeof(cap));

	XNVME_DEBUG("INFO: Controller CC:");
	_hexdump_range(&cc, sizeof(cc));

	// determine the supposed IO set
	if (cap.noiocss)
		cc.css = 0b111;
	else if (cap.iocss)
		cc.css = 0b110;
	else if (!cap.iocss && cap.ncss)
		cc.css = 0b000;
	else {
		XNVME_DEBUG("FAILED: unsupported IO command set configuration, cap.css: 0x%lx", cap.noiocss | cap.iocss | cap.ncss);
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

	err = _perform_property_set(ctrlr, NVME_CTRLR_PROP_CC, (uint64_t) cc.raw);
	if (err) {
		XNVME_DEBUG("FAILED: set CC, err: %d", err);
		return err;
	}

	while (!csts.rdy) {
		err = _perform_property_get(ctrlr, NVME_CTRLR_PROP_CSTS, &val);
		if (err) {
			XNVME_DEBUG("FAILED: get CSTS, err: %d", err);
			return err;
		}
		csts.raw = val;
	}

	XNVME_DEBUG("INFO: Controller CSTS:");
	_hexdump_range(&csts, sizeof(csts));

	XNVME_DEBUG("INFO: Controller CSTS Ready: %d", csts.rdy);

#if 0


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
#endif
	return 0;
}