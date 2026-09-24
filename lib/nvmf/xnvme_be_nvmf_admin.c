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
#include <xnvme_be_nvmf_rdma.h> // LATER

#include <infiniband/verbs.h>

static inline int
_xnvme_be_nvmf_admin_cmd_idfy(struct xnvme_be_nvmf_ctrlr *ctrlr, struct xnvme_cmd_ctx *ctx, struct xnvme_be_nvmf_req *req, void *dbuf,
			      size_t dbuf_nbytes, struct ibv_mr **data_mr_out)
{
	struct xnvme_be_nvmf_qpair *qpair = (void *)ctrlr->admin_qpair;
	struct xnvme_spec_cmd *cmd = &ctx->cmd;
	struct xnvme_spec_sgl_descriptor *sgl = (void *)&cmd->common.dptr.sgl;
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr = TO_XNVME_NVMF_RDMA_CTRLR(ctrlr); // LATER
	int err;

	XNVME_DEBUG("INFO: Preparing IDFY command with dbuf at %p, cntlid: %zu", dbuf, qpair->cntlid);
	XNVME_DEBUG("INFO: CNS value: 0x%x", cmd->idfy.cns);
	switch (cmd->idfy.cns) {
		case 0x02:
			cmd->idfy.cntid = 0x0;  // cleared for specific CNS values
			cmd->idfy.csi = 0x0;  // cleared for specific CNS values
			break;
		default:
			break;
	}


	struct ibv_mr *data_mr = ibv_reg_mr(rdma_ctrlr->pd, dbuf, dbuf_nbytes,
						IBV_ACCESS_LOCAL_WRITE | \
						IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
	if (!data_mr) {
		XNVME_DEBUG("FAILED: ibv_reg_mr() for data buffer, err: %d", errno);
		err = -errno;
		return -errno;
	}

	XNVME_DEBUG("INFO: Command before sending:");
	_hexdump_range(cmd, sizeof(*cmd));

	cmd->common.psdt = 0b10;

	sgl->addr = (uint64_t)dbuf;
	sgl->keyed.len = dbuf_nbytes;
	sgl->keyed.key =
		data_mr->rkey; // TODO: This needs to be set to the correct value for the controller.
	sgl->keyed.type = XNVME_SPEC_SGL_DESCR_TYPE_KEYED_DATA_BLOCK;
	sgl->keyed.subtype = XNVME_SPEC_SGL_DESCR_SUBTYPE_ADDRESS;

	XNVME_DEBUG("INFO: SGL address: %p", (void*) sgl->addr);
	_hexdump_range(&sgl->addr, sizeof(sgl->addr));
	XNVME_DEBUG("INFO: SGL length: %zu", sgl->keyed.len);
	XNVME_DEBUG("INFO: SGL key: 0x%x", sgl->keyed.key);
	XNVME_DEBUG("INFO: SGL type: 0x%x", sgl->keyed.type);
	XNVME_DEBUG("INFO: SGL subtype: 0x%x", sgl->keyed.subtype);

	err = xnvme_be_nvmf_qpair_send_capsule(qpair, req, cmd, sizeof(struct xnvme_spec_cmd));
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_nvmf_qpair_send_capsule(), err: %d", err);
		ibv_dereg_mr(data_mr);
		return err;
	}

	/* Deregistered by the caller only after the command completes: the remote
	 * controller still needs the rkey valid to RDMA-write the Identify data. */
	*data_mr_out = data_mr;
	return 0;
}

static int
_xnvme_be_nvmf_admin_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			       void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_nvmf_state *state = (void *)ctx->dev->be.state;
	struct xnvme_be_nvmf_ctrlr *ctrlr = (void *)state->ctrlr;
	struct xnvme_be_nvmf_qpair *qpair = (void *)ctrlr->admin_qpair;
	struct xnvme_be_nvmf_req *req = NULL;
	struct ibv_mr *data_mr = NULL;
	int err = 0;

	XNVME_DEBUG("INFO: admin_cmd() for NVMe-oF device: %s", ctx->dev->ident.uri);
	XNVME_DEBUG("INFO: opcode: 0x%x, nsid: %d", ctx->cmd.common.opcode, ctx->cmd.common.nsid);

	req = xnvme_be_nvmf_req_alloc(qpair->req_pool, false, (void *)ctx);
	if (!req) {
		XNVME_DEBUG("FAILED: xnvme_be_nvmf_req_alloc()");
		return -ENOSPC;
	}

	/* Set the command identifier (CID) to the request's CID */
	ctx->cmd.common.cid = req->cid;

	pthread_mutex_lock(&state->lock);
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_ADM_OPC_IDFY:

		//_hexdump_range(dbuf, dbuf_nbytes);
		err = _xnvme_be_nvmf_admin_cmd_idfy(ctrlr, ctx, req, dbuf, dbuf_nbytes, &data_mr);
		if (err) {
			XNVME_DEBUG("FAILED: _xnvme_be_nvmf_admin_cmd_idfy(), err: %d", err);
		}

		_print_nvme_completion(&ctx->cpl);
		//_hexdump_range(dbuf, dbuf_nbytes);
		break;
	case XNVME_SPEC_ADM_OPC_GFEAT:
	default:
		XNVME_DEBUG("FAILED: ENOSYS opcode: %d", ctx->cmd.common.opcode);
		err = -ENOSYS;
		break;
	}
	pthread_mutex_unlock(&state->lock);

	xnvme_be_nvmf_wait_for_completion(qpair, req);

	if (data_mr) {
		ibv_dereg_mr(data_mr);
	}

	xnvme_be_nvmf_req_free(qpair->req_pool, req);

	return err;
}

struct xnvme_be_admin g_xnvme_be_nvmf_admin = {
	.id = "nvmf",
	.cmd_admin = _xnvme_be_nvmf_admin_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
};
