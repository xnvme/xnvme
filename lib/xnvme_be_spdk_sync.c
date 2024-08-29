// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_SPDK_ENABLED
#include <errno.h>
#include <spdk/env.h>
#include <xnvme_dev.h>
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

static void
reset_sgl_offset(void *cb_arg, uint32_t offset)
{
	struct xnvme_cmd_ctx *ctx = cb_arg;
	struct xnvme_be_spdk_state *state = (void *)ctx->dev->be.state;
	struct iovec *iov;

	state->payload.iov_offset = offset;

	for (state->payload.iov_offset = 0; state->payload.iov_pos < state->payload.iov_cnt;
	     state->payload.iov_pos++) {
		iov = &state->payload.iov[state->payload.iov_pos];
		if (state->payload.iov_offset < iov->iov_len) {
			break;
		}

		state->payload.iov_offset -= iov->iov_len;
	}
}

static int
next_sgl_entry(void *cb_arg, void **address, uint32_t *length)
{
	struct xnvme_cmd_ctx *ctx = cb_arg;
	struct xnvme_be_spdk_state *state = (void *)ctx->dev->be.state;
	struct iovec *iov;

	iov = &state->payload.iov[state->payload.iov_pos];

	*address = iov->iov_base;
	*length = iov->iov_len;

	if (state->payload.iov_offset) {
		*address += state->payload.iov_offset;
		*length -= state->payload.iov_offset;
	}

	state->payload.iov_offset += *length;
	if (state->payload.iov_offset == iov->iov_len) {
		state->payload.iov_pos++;
		state->payload.iov_offset = 0;
	}

	return 0;
}

int
xnvme_be_spdk_sync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			   size_t dvec_nbytes, void *mbuf, size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_spdk_state *state = (void *)ctx->dev->be.state;
	struct spdk_nvme_qpair *qpair = state->qpair;
	pthread_mutex_t *qpair_lock = &state->qpair_lock;
	uint8_t *completed = &ctx->be_rsvd[0];
	int err, err_lock;

	*completed = 0;

	state->payload.iov = dvec;
	state->payload.iov_cnt = dvec_cnt;
	state->payload.iov_pos = 0;
	state->payload.iov_offset = 0;

	err_lock = pthread_mutex_lock(qpair_lock);
	if (err_lock) {
		XNVME_DEBUG("FAILED: pthread_mutex_lock(), err_lock: %d", err_lock);
		return -err_lock;
	}

	err = spdk_nvme_ctrlr_cmd_iov_raw_with_md(
		state->ctrlr, qpair, (struct spdk_nvme_cmd *)&ctx->cmd, (uint32_t)dvec_nbytes,
		mbuf, cmd_sync_cb, ctx, reset_sgl_offset, next_sgl_entry);

	if (err) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_cmd_iov_raw_with_md(), err: %d", err);
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
	.cmd_iov = xnvme_be_spdk_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
