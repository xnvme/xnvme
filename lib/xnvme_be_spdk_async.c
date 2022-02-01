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

/**
 * Command Queue for asynchronous command submission and completion
 *
 * The context is not thread-safe and the intent is that the user must
 * initialize the opaque #xnvme_async_ctx via xnvme_async_init() pr. thread,
 * which is then delegated to the backend, in this case XNVME_BE_SPDK, which
 * then initialized a struct containing what it needs for a submission /
 * completion path, in the case of XNVME_BE_SPDK, then a qpair is needed and
 * thus allocated and de-allocated by:
 *
 * The XNVME_BE_SPDK specific context is a SPDK qpair and it is carried inside:
 *
 * xnvme_queue->be_rsvd
 */
int
xnvme_be_spdk_queue_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	struct xnvme_queue_spdk *queue = (void *)(q);
	struct xnvme_be_spdk_state *state = (void *)queue->base.dev->be.state;
	struct spdk_nvme_io_qpair_opts qopts = {0};

	spdk_nvme_ctrlr_get_default_io_qpair_opts(state->ctrlr, &qopts, sizeof(qopts));

	qopts.io_queue_size = XNVME_MAX(queue->base.capacity, qopts.io_queue_size);
	qopts.io_queue_requests = qopts.io_queue_size * 2;

	queue->qpair = spdk_nvme_ctrlr_alloc_io_qpair(state->ctrlr, &qopts, sizeof(qopts));
	if (!queue->qpair) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_alloc_io_qpair()");
		return -ENOMEM;
	}

	return 0;
}

int
xnvme_be_spdk_queue_term(struct xnvme_queue *q)
{
	struct xnvme_queue_spdk *queue = (void *)q;
	int err;

	if (!queue->qpair) {
		XNVME_DEBUG("FAILED: !queue->qpair");
		return -EINVAL;
	}

	err = spdk_nvme_ctrlr_free_io_qpair(queue->qpair);
	if (err) {
		XNVME_DEBUG("FAILED: spdk_nvme_ctrlr_free_io_qpair(%p), errno: %s",
			    (void *)queue->qpair, strerror(errno));
		return err;
	}

	return err;
}

int
xnvme_be_spdk_queue_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_spdk *queue = (void *)q;
	int err;

	err = spdk_nvme_qpair_process_completions(queue->qpair, max);
	if (err < 0) {
		XNVME_DEBUG("FAILED: spdk_nvme_qpair_process_completion(), err: %d", err);
	}

	return err;
}

static void
cmd_async_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl)
{
	struct xnvme_cmd_ctx *ctx = cb_arg;

	ctx->async.queue->base.outstanding -= 1;
	ctx->cpl = *(const struct xnvme_spec_cpl *)cpl;
	ctx->async.cb(ctx, ctx->async.cb_arg);
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
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
		break;
	}

	return spdk_nvme_ctrlr_cmd_io_raw_with_md(ctrlr, qpair, (struct spdk_nvme_cmd *)&ctx->cmd,
						  dbuf, dbuf_nbytes, mbuf, cb_fn, cb_arg);
}

// TODO: consider whether 'mbuf_nbytes' is needed here
// TODO: consider whether 'opts' is needed here
int
xnvme_be_spdk_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_queue_spdk *queue = (void *)ctx->async.queue;
	struct xnvme_be_spdk_state *state = (void *)queue->base.dev->be.state;
	int err;

	// TODO: do something with mbuf?
	queue->base.outstanding += 1;
	err = submit_ioc(state->ctrlr, queue->qpair, ctx, dbuf, dbuf_nbytes, mbuf, cmd_async_cb,
			 ctx);
	if (err) {
		queue->base.outstanding -= 1;
		XNVME_DEBUG("FAILED: submission failed");
		return err;
	}

	return 0;
}
#endif

struct xnvme_be_async g_xnvme_be_spdk_async = {
	.id = "nvme",
#ifdef XNVME_BE_SPDK_ENABLED
	.cmd_io = xnvme_be_spdk_async_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_spdk_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_spdk_queue_init,
	.term = xnvme_be_spdk_queue_term,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
#endif
};
