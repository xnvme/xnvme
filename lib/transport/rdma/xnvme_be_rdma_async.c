// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#include <xnvme_dev.h>
#include <xnvme_queue.h>

#include <xnvme_transport_mem.h>
#include <xnvme_transport_debug.h>
#include <xnvme_transport_qpair.h>

#include <xnvme_be_rdma.h>

#define TRANSPORT_DEBUG_CATEGORY TRANSPORT_DEBUG_CATEGORY_CMD_IO

static inline int
_rdma_async_cmd_submit(struct xnvme_transport_qpair *qpair, struct xnvme_cmd_ctx *ctx, void *dbuf,
		       size_t dbuf_nbytes)
{
	struct xnvme_transport_req *req;
	int err;

	err = xnvme_transport_cmd_set_sgl(ctx, dbuf, dbuf_nbytes);
	if (err) {
		return err;
	}

	err = xnvme_transport_qpair_submit(qpair, ctx, &req);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_submit(), err: %d", err);
		xnvme_transport_hexdump_range(TRANSPORT_DEBUG_CATEGORY_CMD_IO, &ctx->cmd,
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
xnvme_be_rdma_queue_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	struct xnvme_dev *dev = q->base.dev;
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	struct xnvme_transport_qpair *admin_qpair = TO_TRANSPORT_QPAIR(state->admin_qpair);
	struct xnvme_transport_ctrlr *ctrlr = TO_TRANSPORT_CTRLR(state->ctrlr);
	struct xnvme_be_rdma_queue *queue = TO_RDMA_QUEUE(q);
	struct xnvme_transport_qpair *async_qpair;
	struct xnvme_transport_qpair_attr attr = {
		.qid = ctrlr->last_allocated_queue_id++,
		.qsize = q->base.capacity, // TODO: Evaluate the performance implications when set
					   // low
		.capsule_size = NVME_CMD_CAPSULE_SIZE,
		.completion_size = NVME_CPL_CAPSULE_SIZE,
		.cntlid = admin_qpair->attr.cntlid,
	};
	int err;

	err = xnvme_transport_qpair_create(ctrlr, dev, &attr, &async_qpair);
	if (err) {
		XNVME_DEBUG("Failed to create qpair: %d", err);
		return err;
	}

	err = xnvme_transport_qpair_connect(async_qpair);
	if (err) {
		XNVME_DEBUG("Failed to connect qpair: %d", err);
		xnvme_transport_qpair_destroy(async_qpair);
		return err;
	}

	queue->qpair = async_qpair;
	return 0;
}

int
xnvme_be_rdma_queue_term(struct xnvme_queue *q)
{
	struct xnvme_transport_qpair *qpair = TO_RDMA_QUEUE_QPAIR(q);
	int err;

	err = xnvme_transport_qpair_destroy(qpair);
	if (err) {
		XNVME_DEBUG("Failed to destroy qpair: %d", err);
		return err;
	}

	return 0;
}

int
xnvme_be_rdma_queue_poke(struct xnvme_queue *q, uint32_t max)
{
	return xnvme_transport_qpair_poll(TO_RDMA_QUEUE_QPAIR(q), max);
}

int
xnvme_be_rdma_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t mbuf_nbytes)
{
	int err;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: metadata buffers are not supported");
		return -ENOSYS;
	}

	err = _rdma_async_cmd_submit(TO_RDMA_QUEUE_QPAIR(ctx->async.queue), ctx, dbuf,
				     dbuf_nbytes);
	if (err) {
		XNVME_DEBUG("Failed to submit IO command: %d", err);
		return err;
	}

	ctx->async.queue->base.outstanding++;

	return err;
}

int
xnvme_be_rdma_async_cmd_iov(struct xnvme_cmd_ctx *XNVME_UNUSED(ctx),
			    struct iovec *XNVME_UNUSED(dvec), size_t XNVME_UNUSED(dvec_cnt),
			    size_t XNVME_UNUSED(dvec_nbytes), void *XNVME_UNUSED(mbuf),
			    size_t XNVME_UNUSED(mbuf_nbytes))
{
	return -ENOSYS;
}

struct xnvme_be_async g_xnvme_be_rdma_async = {
	.id = "rdma",
	.cmd_io = xnvme_be_rdma_async_cmd_io,
	.cmd_iov = xnvme_be_rdma_async_cmd_iov,
	.poke = xnvme_be_rdma_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_rdma_queue_init,
	.term = xnvme_be_rdma_queue_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
};
