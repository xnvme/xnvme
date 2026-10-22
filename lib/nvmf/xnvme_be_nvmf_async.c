// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#include <errno.h>

#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_debug.h>
#include <xnvme_be_nvmf_mem.h>
#include <xnvme_be_nvmf_qpair.h>

#define NVMF_DEBUG_CATEGORY NVMF_DEBUG_CATEGORY_CMD_IO

static inline int
_nvmf_async_cmd_submit(struct xnvme_be_nvmf_qpair *qpair, struct xnvme_cmd_ctx *ctx, void *dbuf,
		       size_t dbuf_nbytes)
{
	struct xnvme_be_nvmf_req *req;
	int err;

	err = xnvme_be_nvmf_cmd_set_sgl(ctx, dbuf, dbuf_nbytes);
	if (err) {
		return err;
	}

	err = xnvme_be_nvmf_qpair_submit(qpair, ctx, &req);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_submit(), err: %d", err);
		xnvme_be_nvmf_hexdump_range(NVMF_DEBUG_CATEGORY_CMD_IO, &ctx->cmd,
					    sizeof(ctx->cmd));
	}

	return err;
}

/**
 * Command queue for asynchronous submission and completion
 *
 * The queue is not thread-safe, the user initializes one per thread. Each queue owns an NVMe-oF
 * qpair, carried in xnvme_queue->be_rsvd.
 */
int
xnvme_be_nvmf_queue_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	struct xnvme_dev *dev = q->base.dev;
	struct xnvme_be_nvmf_state *state = TO_NVMF_DEV_STATE(dev);
	struct xnvme_be_nvmf_qpair *admin_qpair = TO_NVMF_QPAIR(state->admin_qpair);
	struct xnvme_be_nvmf_ctrlr *ctrlr = TO_NVMF_CTRLR(state->ctrlr);
	struct xnvme_be_nvmf_queue *queue = TO_NVMF_QUEUE(q);
	struct xnvme_be_nvmf_qpair *async_qpair;
	struct xnvme_be_nvmf_qpair_attr attr = {
		.qid = ctrlr->last_allocated_queue_id++,
		.qsize = q->base.capacity, // TODO: Evaluate the performance implications when set
					   // low
		.capsule_size = NVME_CMD_CAPSULE_SIZE,
		.completion_size = NVME_CPL_CAPSULE_SIZE,
		.cntlid = admin_qpair->attr.cntlid,
	};
	int err;

	err = xnvme_be_nvmf_qpair_create(ctrlr, dev, &attr, &async_qpair);
	if (err) {
		XNVME_DEBUG("Failed to create qpair: %d", err);
		return err;
	}

	err = xnvme_be_nvmf_qpair_connect(async_qpair);
	if (err) {
		XNVME_DEBUG("Failed to connect qpair: %d", err);
		xnvme_be_nvmf_qpair_destroy(async_qpair);
		return err;
	}

	queue->qpair = async_qpair;
	return 0;
}

int
xnvme_be_nvmf_queue_term(struct xnvme_queue *q)
{
	struct xnvme_be_nvmf_qpair *qpair = TO_NVMF_QUEUE_QPAIR(q);
	int err;

	if (xnvme_be_nvmf_qpair_is_connected(qpair)) {
		err = xnvme_be_nvmf_qpair_disconnect(qpair);
		if (err) {
			XNVME_DEBUG("Failed to disconnect qpair: %d", err);
			return err;
		}
	}

	err = xnvme_be_nvmf_qpair_destroy(qpair);
	if (err) {
		XNVME_DEBUG("Failed to destroy qpair: %d", err);
		return err;
	}

	return err;
}

int
xnvme_be_nvmf_queue_poke(struct xnvme_queue *q, uint32_t max)
{
	return xnvme_be_nvmf_qpair_poll(TO_NVMF_QUEUE_QPAIR(q), max);
}

int
xnvme_be_nvmf_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			   void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	int err;

	err = _nvmf_async_cmd_submit(TO_NVMF_QUEUE_QPAIR(ctx->async.queue), ctx, dbuf,
				     dbuf_nbytes);
	if (err) {
		XNVME_DEBUG("Failed to submit IO command: %d", err);
		return err;
	}

	ctx->async.queue->base.outstanding++;

	return err;
}

int
xnvme_be_nvmf_async_cmd_iov(struct xnvme_cmd_ctx *XNVME_UNUSED(ctx),
			    struct iovec *XNVME_UNUSED(dvec), size_t XNVME_UNUSED(dvec_cnt),
			    size_t XNVME_UNUSED(dvec_nbytes), void *XNVME_UNUSED(mbuf),
			    size_t XNVME_UNUSED(mbuf_nbytes))
{
	return -ENOSYS;
}

struct xnvme_be_async g_xnvme_be_nvmf_async = {
	.id = "nvmf",
	.cmd_io = xnvme_be_nvmf_async_cmd_io,
	.cmd_iov = xnvme_be_nvmf_async_cmd_iov,
	.poke = xnvme_be_nvmf_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nvmf_queue_init,
	.term = xnvme_be_nvmf_queue_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
};
