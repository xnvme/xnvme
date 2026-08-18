// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_upcie_cuda.h>

int
xnvme_be_upcie_cuda_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
				void *mbuf, size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_upcie_state *state = (void *)ctx->dev->be.state;
	struct nvme_controller *ctrlr = state->ctrlr->ctrl;
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

	req = nvme_request_alloc(state->ctrlr->sync.rpool);
	if (!req) {
		XNVME_DEBUG("FAILED: nvme_request_alloc(); errno(%d)", errno);
		return -errno;
	}

	req->user = ctx;
	cmd->cid = req->cid;

	if (dbuf) {
		if (cudamem_heap_contains(&g_upcie_cuda_rte.cuda_heap, dbuf)) {
			nvme_request_prep_command_prps_contig_cuda(
				req, &g_upcie_cuda_rte.cuda_heap, dbuf, dbuf_nbytes, cmd);
		} else {
			err = nvme_request_prep_command_prps_contig_cuda_mapped(
				req, &g_upcie_cuda_rte.mappings, g_upcie_cuda_rte.cuda_heap.config,
				dbuf, dbuf_nbytes, cmd);
			if (err) {
				XNVME_DEBUG("FAILED: prps_contig_cuda_mapped(); err(%d)", err);
				goto exit;
			}
		}
	}

	if (mbuf) {
		ctx->cmd.common.mptr = cudamem_dma_v2p(&g_upcie_cuda_rte.cuda_heap, mbuf);
	}

	err = nvme_qpair_enqueue(&state->ctrlr->sync, cmd);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_enqueue();");
		goto exit;
	}

	nvme_qpair_sqdb_update(&state->ctrlr->sync);

	err = nvme_qpair_reap_cpl(&state->ctrlr->sync, ctrlr->timeout_ms, cpl);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_reap_cpl();");
		goto exit;
	}

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		XNVME_DEBUG("FAILED: command; sc(%d); sct(%d)", ctx->cpl.status.sc,
			    ctx->cpl.status.sct);
		err = -EIO;
	}

exit:
	nvme_request_free(state->ctrlr->sync.rpool, req->cid);
	return err;
}

int
xnvme_be_upcie_cuda_sync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
				 size_t XNVME_UNUSED(dvec_nbytes), void *mbuf,
				 size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_upcie_state *state = (void *)ctx->dev->be.state;
	struct nvme_controller *ctrlr = state->ctrlr->ctrl;
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

	req = nvme_request_alloc(state->ctrlr->sync.rpool);
	if (!req) {
		XNVME_DEBUG("FAILED: nvme_request_alloc(); errno(%d)", errno);
		return -errno;
	}

	req->user = ctx;
	cmd->cid = req->cid;

	if (dvec) {
		if (dvec_cnt > 0 &&
		    cudamem_heap_contains(&g_upcie_cuda_rte.cuda_heap, dvec[0].iov_base)) {
			nvme_request_prep_command_prps_iov_cuda(req, &g_upcie_cuda_rte.cuda_heap,
								dvec, dvec_cnt, cmd);
		} else {
			err = nvme_request_prep_command_prps_iov_cuda_mapped(
				req, &g_upcie_cuda_rte.mappings, g_upcie_cuda_rte.cuda_heap.config,
				dvec, dvec_cnt, cmd);
			if (err) {
				XNVME_DEBUG("FAILED: prps_iov_cuda_mapped(); err(%d)", err);
				goto exit;
			}
		}
	}

	if (mbuf) {
		ctx->cmd.common.mptr = cudamem_dma_v2p(&g_upcie_cuda_rte.cuda_heap, mbuf);
	}

	err = nvme_qpair_enqueue(&state->ctrlr->sync, cmd);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_enqueue();");
		goto exit;
	}

	nvme_qpair_sqdb_update(&state->ctrlr->sync);

	err = nvme_qpair_reap_cpl(&state->ctrlr->sync, ctrlr->timeout_ms, cpl);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_reap_cpl();");
		goto exit;
	}

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		XNVME_DEBUG("FAILED: command; sc(%d); sct(%d)", ctx->cpl.status.sc,
			    ctx->cpl.status.sct);
		err = -EIO;
	}

exit:
	nvme_request_free(state->ctrlr->sync.rpool, req->cid);
	return err;
}

#endif

struct xnvme_be_sync g_xnvme_be_upcie_cuda_sync = {
	.id = "upcie-cuda",
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
	.cmd_io = xnvme_be_upcie_cuda_sync_cmd_io,
	.cmd_iov = xnvme_be_upcie_cuda_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
