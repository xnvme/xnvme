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
	struct nvme_request *req;
	int err;

	if (g_upcie_rte.connection.alive) {
		/* There is one admin queue and it belongs to whoever opened the
		 * controller. The payload does not travel with the request: the
		 * command names an address this process registered or was
		 * given, so the answer lands here without a copy. */
		struct nvme_cplane_msg msg = {0};

		if (dbuf) {
			/* Describing the payload is all this needs the scratch
			 * for; the admin queue it is submitted on belongs to the
			 * server. */
			struct nvme_request *prp = xnvme_be_upcie_ctrlr_admin_prp(ctrlr);

			if (!prp) {
				XNVME_DEBUG("FAILED: admin PRP scratch; errno(%d)", errno);
				return -errno;
			}

			err = nvme_request_prep_command_prps_contig_dmamem(prp, state->dmem, dbuf,
									   dbuf_nbytes, cmd);
			if (err) {
				XNVME_DEBUG("FAILED: prps_contig_dmamem(); err(%d)", err);
				return err;
			}
		}

		msg.op = NVME_CPLANE_OP_ADMIN_CMD;
		memcpy(&msg.u.admin.cmd, cmd, sizeof(msg.u.admin.cmd));

		err = xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL);
		if (err) {
			XNVME_DEBUG("FAILED: asking for an admin command; err(%d)", err);
			return err;
		}

		memcpy(cpl, &msg.u.admin.cpl, sizeof(*cpl));

		return xnvme_cmd_ctx_cpl_status(ctx) ? -EIO : 0;
	}

	req = nvme_request_alloc(ctrl->aq.rpool);
	if (!req) {
		XNVME_DEBUG("FAILED: nvme_request_alloc(); errno(%d)", errno);
		err = -errno;
		return err;
	}

	req->user = ctx;
	cmd->cid = req->cid;

	if (dbuf) {
		err = nvme_request_prep_command_prps_contig_dmamem(req, state->dmem, dbuf,
								   dbuf_nbytes, cmd);
		if (err) {
			XNVME_DEBUG("FAILED: prps_contig_dmamem(); err(%d)", err);
			goto exit;
		}
	}

	err = nvme_qpair_enqueue(&ctrl->aq, cmd);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_enqueue(); err(%d)", err);
		goto exit;
	}

	nvme_qpair_sqdb_update(&ctrl->aq);

	err = nvme_qpair_reap_cpl(&ctrl->aq, ctrl->timeout_ms, cpl);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_reap_cpl(); err(%d)", err);
		goto exit;
	}

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		XNVME_DEBUG("FAILED: command; sc(%d); sct(%d)", ctx->cpl.status.sc,
			    ctx->cpl.status.sct);
		err = -EIO;
	}

exit:
	nvme_request_free(ctrl->aq.rpool, req->cid);
	return err;
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
