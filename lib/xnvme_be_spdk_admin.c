// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_SPDK_ENABLED
#include <xnvme_dev.h>
#include <spdk/env.h>
#include <errno.h>
#include <xnvme_queue.h>
#include <xnvme_be_spdk.h>

static void
cmd_sync_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl)
{
	struct xnvme_cmd_ctx *ctx = cb_arg;
	uint8_t *completed = &ctx->be_rsvd[0];

	ctx->cpl = *(const struct xnvme_spec_cpl *)cpl;
	*completed = 1;
}

// TODO: consider whether 'opts' should be used for anything here...
static inline int
cmd_admin_submit(struct spdk_nvme_ctrlr *ctrlr, struct xnvme_cmd_ctx *ctx, void *dbuf,
		 uint32_t dbuf_nbytes, void *mbuf, uint32_t mbuf_nbytes, spdk_nvme_cmd_cb cb_fn,
		 void *cb_arg)
{
	ctx->cmd.common.mptr = (uint64_t)mbuf ? (uint64_t)mbuf : ctx->cmd.common.mptr;

	if (mbuf_nbytes && mbuf) {
		ctx->cmd.common.ndm = mbuf_nbytes / 4;
	}

#ifdef XNVME_TRACE_ENABLED
	XNVME_DEBUG("Dumping ADMIN command");
	xnvme_spec_cmd_pr(cmd, 0x0);
#endif

	return spdk_nvme_ctrlr_cmd_admin_raw(ctrlr, (struct spdk_nvme_cmd *)&ctx->cmd, dbuf,
					     dbuf_nbytes, cb_fn, cb_arg);
}

int
xnvme_be_spdk_sync_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			     size_t mbuf_nbytes)
{
	struct xnvme_be_spdk_state *state = (void *)ctx->dev->be.state;
	uint8_t *completed = &ctx->be_rsvd[0];

	*completed = 0;

	if (cmd_admin_submit(state->ctrlr, ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes, cmd_sync_cb,
			     ctx)) {
		XNVME_DEBUG("FAILED: cmd_admin_submit");
		return -EIO;
	}

	while (!*completed) { // Wait for completion-indicator
		spdk_nvme_ctrlr_process_admin_completions(state->ctrlr);
	}

	*completed = 0;

	// check for errors
	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		XNVME_DEBUG("FAILED: xnvme_cmd_ctx_cpl_status()");
		return -EIO;
	}

	return 0;
}
#endif

struct xnvme_be_admin g_xnvme_be_spdk_admin = {
	.id = "nvme",
#ifdef XNVME_BE_SPDK_ENABLED
	.cmd_admin = xnvme_be_spdk_sync_cmd_admin,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
#endif
};
