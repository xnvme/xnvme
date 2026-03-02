// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_upcie.h>

int
xnvme_be_upcie_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_upcie_state *state = (void *)ctx->dev->be.state;
	struct nvme_controller *ctrl = state->ctrlr->ctrl;
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

	if (mbuf) {
		ctx->cmd.common.mptr = hostmem_dma_v2p(&g_upcie_rte.heap, mbuf);
	}

	if (dbuf) {
		err = nvme_qpair_submit_sync_contig_prps(
			&state->ctrlr->sync, &g_upcie_rte.heap, dbuf, dbuf_nbytes,
			(struct nvme_command *)&ctx->cmd, ctrl->timeout_ms,
			(struct nvme_completion *)&ctx->cpl);
		if (err || xnvme_cmd_ctx_cpl_status(ctx)) {
			XNVME_DEBUG("FAILED: nvme_qpair_submit_sync_contig_prps(); err(%d); "
				    "sc(%d); sct(%d)",
				    err, ctx->cpl.status.sc, ctx->cpl.status.sct);
			return err;
		}
	} else {
		err = nvme_qpair_submit_sync(&state->ctrlr->sync, (struct nvme_command *)&ctx->cmd,
					     ctrl->timeout_ms,
					     (struct nvme_completion *)&ctx->cpl);
		if (err || xnvme_cmd_ctx_cpl_status(ctx)) {
			XNVME_DEBUG("FAILED: nvme_qpair_submit_sync(); err(%d); sc(%d); sct(%d)",
				    err, ctx->cpl.status.sc, ctx->cpl.status.sct);
			return err;
		}
	}

	return err;
}

int
xnvme_be_upcie_sync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			    size_t XNVME_UNUSED(dvec_nbytes), void *mbuf,
			    size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_upcie_state *state = (void *)ctx->dev->be.state;
	struct nvme_controller *ctrl = state->ctrlr->ctrl;
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

	if (mbuf) {
		ctx->cmd.common.mptr = hostmem_dma_v2p(&g_upcie_rte.heap, mbuf);
	}

	if (dvec) {
		err = nvme_qpair_submit_sync_iov_prps(&state->ctrlr->sync, &g_upcie_rte.heap, dvec,
						      dvec_cnt, (struct nvme_command *)&ctx->cmd,
						      ctrl->timeout_ms,
						      (struct nvme_completion *)&ctx->cpl);
		if (err || xnvme_cmd_ctx_cpl_status(ctx)) {
			XNVME_DEBUG("FAILED: nvme_qpair_submit_sync_iov_prps(); err(%d); sc(%d); "
				    "sct(%d)",
				    err, ctx->cpl.status.sc, ctx->cpl.status.sct);
			return err;
		}
	} else {
		err = nvme_qpair_submit_sync(&state->ctrlr->sync, (struct nvme_command *)&ctx->cmd,
					     ctrl->timeout_ms,
					     (struct nvme_completion *)&ctx->cpl);
		if (err || xnvme_cmd_ctx_cpl_status(ctx)) {
			XNVME_DEBUG("FAILED: nvme_qpair_submit_sync(); err(%d); sc(%d); sct(%d)",
				    err, ctx->cpl.status.sc, ctx->cpl.status.sct);
			return err;
		}
	}

	return err;
}

#endif

struct xnvme_be_sync g_xnvme_be_upcie_sync = {
	.id = "upcie",
#ifdef XNVME_BE_UPCIE_ENABLED
	.cmd_io = xnvme_be_upcie_sync_cmd_io,
	.cmd_iov = xnvme_be_upcie_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
