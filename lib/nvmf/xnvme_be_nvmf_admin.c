// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#include <errno.h>
#include <unistd.h>
#include <xnvme_dev.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_qpair.h>
#include <xnvme_be_nvmf_req.h>
#include <xnvme_be_nvmf_debug.h>

#define NVMF_DEBUG_CATEGORY NVMF_DEBUG_CATEGORY_CMD_ADMIN

static inline int
_xnvme_be_nvmf_admin_cmd_idfy(struct xnvme_be_nvmf_qpair *qpair, struct xnvme_cmd_ctx *ctx,
			      struct xnvme_be_nvmf_req *req, void *dbuf, size_t dbuf_nbytes,
			      void **handle_out)
{
	struct xnvme_be_nvmf_state *state = (void *)ctx->dev->be.state;
	struct xnvme_spec_cmd *cmd = &ctx->cmd;
	struct xnvme_spec_sgl_descriptor *sgl = (void *)&cmd->common.dptr.sgl;
	struct xnvme_be_nvmf_ctrlr *ctrlr = state->ctrlr;
	void *handle;
	uint32_t key;
	int err;
	
	NVMF_DEBUG("INFO: Preparing IDFY command with dbuf at %p, cntlid: %u", dbuf,
		    qpair->cntlid);
	NVMF_DEBUG("INFO: CNS value: 0x%x", cmd->idfy.cns);

	enum xnvme_idfy_cns {
		XNVME_IDFY_CNS_NS = 0x0,
		XNVME_IDFY_CNS_CTRLR = 0x1,
		XNVME_IDFY_CNS_ACTIVE_NS = 0x2,
		XNVME_IDFY_CNS_NS_DESC = 0x3,
		XNVME_IDFY_CNS_NVMSET = 0x4,
		XNVME_IDFY_CNS_IOCSI_NS = 0x5,
		XNVME_IDFY_CNS_IOCSI_CTRLR = 0x6,
		XNVME_IDFY_CNS_IOCSI_ACTIVE_NS = 0x7,
		XNVME_IDFY_CNS_IOCSI_INDEP_NS = 0x8,
		XNVME_IDFY_CNS_NS_FMT = 0x9,
		XNVME_IDFY_CNS_IOCSI_NS_FMT = 0xA
	};

	// nsid
	switch (cmd->idfy.cns) {
	case XNVME_IDFY_CNS_NS:
	case XNVME_IDFY_CNS_ACTIVE_NS:
	case XNVME_IDFY_CNS_NS_DESC:
	case XNVME_IDFY_CNS_IOCSI_NS:
	case XNVME_IDFY_CNS_IOCSI_ACTIVE_NS:
	case XNVME_IDFY_CNS_IOCSI_INDEP_NS:
		// Do nothing for now
		NVMF_DEBUG("INFO: CNS value indicates a namespace-related identify command");
		NVMF_DEBUG("INFO: namespace=%u", cmd->common.nsid);
		break;
	default:
		cmd->common.nsid = 0; // default value for other CNS values
		break;
	}

	// cntid
	switch (cmd->idfy.cns) {
	default:
		cmd->idfy.cntid = 0; // default value for other CNS values
		break;
	}

	// csi
	switch (cmd->idfy.cns) {
	case XNVME_IDFY_CNS_IOCSI_NS:
	case XNVME_IDFY_CNS_IOCSI_CTRLR:
	case XNVME_IDFY_CNS_IOCSI_ACTIVE_NS:
	case XNVME_IDFY_CNS_NS_FMT:
	case XNVME_IDFY_CNS_IOCSI_NS_FMT:
		NVMF_DEBUG("INFO: CNS value indicates a Command Set Specific identify command");
		NVMF_DEBUG("INFO: csi=%u", cmd->idfy.csi);
		break;
	default:
		cmd->idfy.csi = 0; // default value for other CNS values
		break;
	}

	err = xnvme_be_nvmf_ctrlr_reg(ctrlr, dbuf, dbuf_nbytes, &handle, &key);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_reg() for data buffer, err: %d", err);
		return err;
	}

	NVMF_DEBUG("INFO: Command before sending:");
	_hexdump_range(NVMF_DEBUG_CATEGORY_CMD_ADMIN, cmd, sizeof(*cmd));

	cmd->common.psdt = 0b10;

	sgl->addr = (uint64_t)dbuf;
	sgl->keyed.len = dbuf_nbytes;
	sgl->keyed.key = key; // TODO: This needs to be set to the correct value for the
			      // controller.
	sgl->keyed.type = XNVME_SPEC_SGL_DESCR_TYPE_KEYED_DATA_BLOCK;
	sgl->keyed.subtype = XNVME_SPEC_SGL_DESCR_SUBTYPE_ADDRESS;

	NVMF_DEBUG("INFO: SGL address: %p", (void *)sgl->addr);
	_hexdump_range(NVMF_DEBUG_CATEGORY_CMD_ADMIN, &sgl->addr, sizeof(sgl->addr));
	NVMF_DEBUG("INFO: SGL length: %u", sgl->keyed.len);
	NVMF_DEBUG("INFO: SGL key: 0x%x", sgl->keyed.key);
	NVMF_DEBUG("INFO: SGL type: 0x%x", sgl->keyed.type);
	NVMF_DEBUG("INFO: SGL subtype: 0x%x", sgl->keyed.subtype);

	err = xnvme_be_nvmf_qpair_submit(qpair, cmd, sizeof(struct xnvme_spec_cmd), req->cid);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_submit(), err: %d", err);
		xnvme_be_nvmf_ctrlr_dereg(ctrlr, handle);
		return err;
	}

	/* Deregistered by the caller only after the command completes: the remote
	 * controller still needs the key valid to RDMA-write the Identify data. */
	*handle_out = handle;
	return 0;
}

static int
_xnvme_be_nvmf_admin_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			       void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_nvmf_state *state = (void *)ctx->dev->be.state;
	struct xnvme_be_nvmf_ctrlr *ctrlr = state->ctrlr;
	struct xnvme_be_nvmf_qpair *qpair = ctrlr->admin_qpair;
	struct xnvme_be_nvmf_req *req = NULL;
	void *handle = NULL;
	int err = 0;

	NVMF_DEBUG("INFO: admin_cmd() for NVMe-oF device: %s", ctx->dev->ident.uri);
	NVMF_DEBUG("INFO: opcode: 0x%x, nsid: %d", ctx->cmd.common.opcode, ctx->cmd.common.nsid);

	req = xnvme_be_nvmf_req_alloc(qpair->req_pool, false, (void *)ctx);
	if (!req) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_req_alloc()");
		return -ENOSPC;
	}

	/* Set the command identifier (CID) to the request's CID */
	ctx->cmd.common.cid = req->cid;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_ADM_OPC_IDFY:

		//_hexdump_range(dbuf, dbuf_nbytes);
		err = _xnvme_be_nvmf_admin_cmd_idfy(qpair, ctx, req, dbuf, dbuf_nbytes, &handle);
		if (err) {
			NVMF_ERROR("FAILED: _xnvme_be_nvmf_admin_cmd_idfy(), err: %d", err);
		}

		_print_nvme_completion(&ctx->cpl);
		//_hexdump_range(dbuf, dbuf_nbytes);
		break;
	case XNVME_SPEC_ADM_OPC_GFEAT:
	default:
		NVMF_ERROR("FAILED: ENOSYS opcode: %d", ctx->cmd.common.opcode);
		err = -ENOSYS;
		break;
	}

	xnvme_be_nvmf_wait_for_completion(qpair, req);

	if (handle) {
		xnvme_be_nvmf_ctrlr_dereg(ctrlr, handle);
	}

	xnvme_be_nvmf_req_free(qpair->req_pool, req);

	return err;
}

struct xnvme_be_admin g_xnvme_be_nvmf_admin = {
	.id = "nvmf",
	.cmd_admin = _xnvme_be_nvmf_admin_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
};
