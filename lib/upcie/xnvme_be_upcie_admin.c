// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_spec.h>
#include <xnvme_be_upcie.h>

int
xnvme_be_upcie_sync_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			      void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_upcie_state *state = (void *)ctx->dev->be.state;
	struct xnvme_be_upcie_ctrlr *ctrlr = state->ctrlr;
	struct nvme_controller *ctrl = state->ctrlr->ctrl;
	struct nvme_command *cmd = (struct nvme_command *)&ctx->cmd;
	struct nvme_completion *cpl = (struct nvme_completion *)&ctx->cpl;
	int err;

	xnvme_be_upcie_ctrlr_mutex_lock(ctrlr);

	if (dbuf) {
		err = nvme_qpair_submit_sync_contig_prps(&ctrl->aq, &g_upcie_rte.heap, dbuf,
							 dbuf_nbytes, cmd, ctrl->timeout_ms, cpl);
	} else {
		err = nvme_qpair_submit_sync(&ctrl->aq, cmd, ctrl->timeout_ms, cpl);
	}

	xnvme_be_upcie_ctrlr_mutex_unlock(ctrlr);

	if (err || xnvme_cmd_ctx_cpl_status(ctx)) {
		XNVME_DEBUG("FAILED: nvme_submit_sync(); err(%d)", err);
		return err;
	}

	return 0;
}

int
xnvme_be_upcie_sync_cmd_pseudo(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			       void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_upcie_state *state = (void *)ctx->dev->be.state;
	struct nvme_controller *ctrl = state->ctrlr->ctrl;
	void *bar0 = ctrl->func.bars[0].region;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_PSEUDO_OPC_SHOW_REGS:
		if (dbuf_nbytes != sizeof(struct xnvme_spec_ctrlr_bar)) {
			XNVME_DEBUG(
				"FAILED: dbuf_nbytes(%zu) != sizeof(struct xnvme_spec_ctrlr_bar)",
				dbuf_nbytes);
			return -EINVAL;
		}
		memcpy(dbuf, bar0, dbuf_nbytes);
		return 0;

	case XNVME_SPEC_PSEUDO_OPC_CONTROLLER_RESET:
		XNVME_DEBUG(
			"FAILED: controller-reset not supported (requires admin queue re-init)");
		return -ENOSYS;

	case XNVME_SPEC_PSEUDO_OPC_NAMESPACE_RESCAN:
		return 0;

	case XNVME_SPEC_PSEUDO_OPC_SUBSYSTEM_RESET:
		XNVME_DEBUG(
			"FAILED: subsystem-reset not supported (requires admin queue re-init)");
		return -ENOSYS;

	default:
		XNVME_DEBUG("FAILED: unsupported opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}
}
#endif

struct xnvme_be_admin g_xnvme_be_upcie_admin = {
	.id = "upcie",
#ifdef XNVME_BE_UPCIE_ENABLED
	.cmd_admin = xnvme_be_upcie_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_upcie_sync_cmd_pseudo,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#endif
};
