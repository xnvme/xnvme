// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_upcie_cuda.h>

int
xnvme_be_upcie_cuda_sync_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
				   void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_upcie_state *state = (void *)ctx->dev->be.state;
	struct nvme_controller *ctrlr = state->ctrlr->ctrl;
	struct nvme_command *cmd = (struct nvme_command *)&ctx->cmd;
	struct nvme_completion *cpl = (struct nvme_completion *)&ctx->cpl;
	struct nvme_request *req;
	int err;

	req = nvme_request_alloc(ctrlr->aq.rpool);
	if (!req) {
		XNVME_DEBUG("FAILED: nvme_request_alloc(); errno(%d)", errno);
		return -errno;
	}

	req->user = ctx;
	cmd->cid = req->cid;

	if (dbuf) {
		if (cudamem_heap_contains(&g_upcie_cuda_rte.cuda_heap, dbuf)) {
			nvme_request_prep_command_prps_contig_cuda(req, &g_upcie_cuda_rte.cuda_heap,
								   dbuf, dbuf_nbytes, cmd);
		} else {
			err = nvme_request_prep_command_prps_contig_cuda_mapped(
				req, &g_upcie_cuda_rte.mappings,
				g_upcie_cuda_rte.cuda_heap.config, dbuf, dbuf_nbytes, cmd);
			if (err) {
				XNVME_DEBUG("FAILED: prps_contig_cuda_mapped(); err(%d)", err);
				goto exit;
			}
		}
	}

	err = nvme_qpair_enqueue(&ctrlr->aq, cmd);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_enqueue();");
		goto exit;
	}

	nvme_qpair_sqdb_update(&ctrlr->aq);

	err = nvme_qpair_reap_cpl(&ctrlr->aq, ctrlr->timeout_ms, cpl);
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
	nvme_request_free(ctrlr->aq.rpool, req->cid);
	return err;
}
#endif

struct xnvme_be_admin g_xnvme_be_upcie_cuda_admin = {
	.id = "upcie-cuda",
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
	.cmd_admin = xnvme_be_upcie_cuda_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#endif
};
