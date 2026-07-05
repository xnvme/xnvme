// SPDX-FileCopyrightText: Simon A. F. Lund
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_DMAMEM_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_dmamem.h>

static int
_submit_sync(struct nvme_qpair *qp, struct nvme_command *cmd, uint32_t timeout_ms,
	     struct nvme_completion *cpl)
{
	int err;

	err = nvme_qpair_enqueue(qp, cmd);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_enqueue(); err(%d)", err);
		return err;
	}
	nvme_qpair_sqdb_update(qp);

	err = nvme_qpair_reap_cpl(qp, timeout_ms, cpl);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_reap_cpl(); err(%d)", err);
		return err;
	}

	return 0;
}

int
xnvme_be_dmamem_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			    size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_dmamem_state *state = (void *)ctx->dev->be.state;
	struct nvme_controller *ctrl = state->ctrlr->ctrl;
	struct nvme_qpair *qp = &state->ctrlr->sync;
	struct nvme_command *cmd = (struct nvme_command *)&ctx->cmd;
	struct nvme_completion *cpl = (struct nvme_completion *)&ctx->cpl;
	struct nvme_request *req;
	int err;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_FS_OPC_READ:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
		break;
	}

	req = nvme_request_alloc(qp->rpool);
	if (!req) {
		XNVME_DEBUG("FAILED: nvme_request_alloc(); errno(%d)", errno);
		return -errno;
	}
	req->user = ctx;
	cmd->cid = req->cid;

	if (dbuf) {
		nvme_request_prep_command_prps_contig_dmamem(req, &g_dmamem_rte.heap, dbuf,
							     dbuf_nbytes, cmd);
	}
	if (mbuf) {
		cmd->mptr = dmamem_va_to_iova(&g_dmamem_rte.dmem, mbuf);
	}

	err = _submit_sync(qp, cmd, ctrl->timeout_ms, cpl);
	nvme_request_free(qp->rpool, req->cid);

	if (err || xnvme_cmd_ctx_cpl_status(ctx)) {
		XNVME_DEBUG("FAILED: sync_cmd_io(); err(%d); sc(%d); sct(%d)", err,
			    ctx->cpl.status.sc, ctx->cpl.status.sct);
		return err;
	}

	return 0;
}

int
xnvme_be_dmamem_sync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			     size_t XNVME_UNUSED(dvec_nbytes), void *mbuf,
			     size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_dmamem_state *state = (void *)ctx->dev->be.state;
	struct nvme_controller *ctrl = state->ctrlr->ctrl;
	struct nvme_qpair *qp = &state->ctrlr->sync;
	struct nvme_command *cmd = (struct nvme_command *)&ctx->cmd;
	struct nvme_completion *cpl = (struct nvme_completion *)&ctx->cpl;
	struct nvme_request *req;
	int err;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_FS_OPC_READ:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
		break;
	}

	req = nvme_request_alloc(qp->rpool);
	if (!req) {
		XNVME_DEBUG("FAILED: nvme_request_alloc(); errno(%d)", errno);
		return -errno;
	}
	req->user = ctx;
	cmd->cid = req->cid;

	if (dvec) {
		nvme_request_prep_command_prps_iov_dmamem(req, &g_dmamem_rte.heap, dvec, dvec_cnt,
							  cmd);
	}
	if (mbuf) {
		cmd->mptr = dmamem_va_to_iova(&g_dmamem_rte.dmem, mbuf);
	}

	err = _submit_sync(qp, cmd, ctrl->timeout_ms, cpl);
	nvme_request_free(qp->rpool, req->cid);

	if (err || xnvme_cmd_ctx_cpl_status(ctx)) {
		XNVME_DEBUG("FAILED: sync_cmd_iov(); err(%d); sc(%d); sct(%d)", err,
			    ctx->cpl.status.sc, ctx->cpl.status.sct);
		return err;
	}

	return 0;
}

#endif

struct xnvme_be_sync g_xnvme_be_dmamem_sync = {
	.id = "dmamem",
#ifdef XNVME_BE_DMAMEM_ENABLED
	.cmd_io = xnvme_be_dmamem_sync_cmd_io,
	.cmd_iov = xnvme_be_dmamem_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
