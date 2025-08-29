// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_upcie.h>

int
xnvme_be_upcie_sync_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			      void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_upcie_state *state = (void *)ctx->dev->be.state;
	struct nvme_controller *ctrl = state->ctrlr->ctrl;
	struct nvme_command *cmd = (struct nvme_command *)&ctx->cmd;
	struct nvme_completion *cpl = (struct nvme_completion *)&ctx->cpl;
	int err;

	if (dbuf) {
		err = nvme_qpair_submit_sync_contig_prps(&ctrl->aq, &g_upcie_rte.heap, dbuf,
							 dbuf_nbytes, cmd, ctrl->timeout_ms, cpl);
	} else {
		err = nvme_qpair_submit_sync(&ctrl->aq, cmd, ctrl->timeout_ms, cpl);
	}

	if (err || xnvme_cmd_ctx_cpl_status(ctx)) {
		XNVME_DEBUG("FAILED: nvme_submit_sync(); err(%d)", err);
		return err;
	}

	return 0;
}
#endif

struct xnvme_be_admin g_xnvme_be_upcie_admin = {
	.id = "upcie",
#ifdef XNVME_BE_UPCIE_ENABLED
	.cmd_admin = xnvme_be_upcie_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#endif
};
