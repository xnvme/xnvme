// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_SPDK_ENABLED
#include <xnvme_dev.h>
#include <spdk/env.h>
#include <errno.h>
#include <libxnvme_spec_fs.h>
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

static inline int
submit_ioc(struct spdk_nvme_ctrlr *ctrlr, struct spdk_nvme_qpair *qpair, struct xnvme_cmd_ctx *ctx,
	   void *dbuf, uint32_t dbuf_nbytes, void *mbuf, spdk_nvme_cmd_cb cb_fn, void *cb_arg)
{
#ifdef XNVME_TRACE_ENABLED
	XNVME_DEBUG("Dumping IO command");
	xnvme_spec_cmd_pr(cmd, XNVME_PR_DEF);
#endif

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_FS_OPC_READ:
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		break;
	}

	return spdk_nvme_ctrlr_cmd_io_raw_with_md(ctrlr, qpair, (struct spdk_nvme_cmd *)&ctx->cmd,
						  dbuf, dbuf_nbytes, mbuf, cb_fn, cb_arg);
}

// TODO: consider whether 'mbuf_nbytes' is needed here
// TODO: consider whether 'opts' is needed here
int
xnvme_be_spdk_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			  size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_spdk_state *state = (void *)ctx->dev->be.state;
	struct spdk_nvme_qpair *qpair = state->qpair;
	pthread_mutex_t *qpair_lock = &state->qpair_lock;
	uint8_t *completed = &ctx->be_rsvd[0];
	int err, err_lock;

	*completed = 0;

	err_lock = pthread_mutex_lock(qpair_lock);
	if (err_lock) {
		XNVME_DEBUG("FAILED: pthread_mutex_lock(), err_lock: %d", err_lock);
		return -err_lock;
	}

	err = submit_ioc(state->ctrlr, qpair, ctx, dbuf, dbuf_nbytes, mbuf, cmd_sync_cb, ctx);
	if (err) {
		XNVME_DEBUG("FAILED: submit_ioc(), err: %d", err);
		goto exit;
	}
	while (!*completed) {
		err = spdk_nvme_qpair_process_completions(qpair, 0);
		if (err < 0) {
			XNVME_DEBUG("FAILED: spdk_nvme_qpair_process_completions(), err: %d", err);
			goto exit;
		}

		err = 0;
	}

exit:
	err_lock = pthread_mutex_unlock(qpair_lock);
	if (err_lock) {
		XNVME_DEBUG("FAILED: pthread_mutex_unlock(), err_lock: %d", err_lock);
	}

	*completed = 0;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		XNVME_DEBUG("FAILED: xnvme_cmd_ctx_cpl_status()");
		err = err ? err : -EIO;
	}

	return err;
}
#endif

struct xnvme_be_sync g_xnvme_be_spdk_sync = {
	.id = "nvme",
#ifdef XNVME_BE_SPDK_ENABLED
	.cmd_io = xnvme_be_spdk_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
