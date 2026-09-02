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

/**
 * Command Queue for asynchronous command submission and completion
 *
 * The context is not thread-safe and the intent is that the user must
 * initialize the opaque #xnvme_async_ctx via xnvme_async_init() pr. thread,
 * which is then delegated to the backend, in this case XNVME_BE_NVMF, which
 * then initialized a struct containing what it needs for a submission /
 * completion path, in the case of XNVME_BE_NVMF, then a qpair is needed and
 * thus allocated and de-allocated by:
 *
 * The XNVME_BE_NVMF specific context is a NVMF qpair and it is carried inside:
 *
 * xnvme_queue->be_rsvd
 */
int
xnvme_be_nvmf_queue_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	struct xnvme_be_nvmf_queue *queue = (struct xnvme_be_nvmf_queue *)q;

	return 0;
}

int
xnvme_be_nvmf_queue_term(struct xnvme_queue *q)
{
	struct xnvme_be_nvmf_queue *queue = (struct xnvme_be_nvmf_queue *)q;
	struct xnvme_be_nvmf_qpair *qpair = queue->qpair;
	int err;

	if (qpair->state == XNVME_NVMF_QPAIR_STATE_CONNECTED || \
			qpair->state == XNVME_NVMF_QPAIR_STATE_READY) {
		err = xnvme_be_nvmf_disconnect_qpair(qpair);
		if (err) {
			XNVME_DEBUG("Failed to disconnect qpair: %d", err);
			return err;
		}
	}
	
	err = xnvme_be_nvmf_destroy_qpair(qpair);
	if (err) {
		XNVME_DEBUG("Failed to destroy qpair: %d", err);
		return err;
	}

	return err;
}

int
xnvme_be_nvmf_queue_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_be_nvmf_queue *queue = (struct xnvme_be_nvmf_queue *)q;
	struct xnvme_be_nvmf_qpair *qpair = queue->qpair;

	return xnvme_be_nvmf_qpair_process_completions(qpair, max);
}

int
xnvme_be_nvmf_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_nvmf_queue *queue = (struct xnvme_be_nvmf_queue *)ctx->async.queue;
	struct xnvme_be_nvmf_req *req;
	int err = -ENOSYS;

	req = xnvme_be_nvmf_req_alloc(queue->qpair->req_pool, true, (void *)ctx);
	if (!req) {
		XNVME_DEBUG("Failed to allocate request");
		return -ENOSPC;
	}

	if (!err) {
		queue->base.outstanding++;
		goto free_req;
	}

	return err;

free_req:
	xnvme_be_nvmf_req_free(queue->qpair->req_pool, req);
	return err;
}

int
xnvme_be_nvmf_async_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			    size_t dvec_nbytes, void *mbuf, size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_nvmf_queue *queue = (struct xnvme_be_nvmf_queue *)ctx->async.queue;
	struct xnvme_be_nvmf_req *req;
	int err = -ENOSYS;

	req = xnvme_be_nvmf_req_alloc(queue->qpair->req_pool, true, (void *)ctx);
	if (!req) {
		XNVME_DEBUG("Failed to allocate request");
		return -ENOSPC;
	}

	if (!err) {
		queue->base.outstanding++;
		goto free_req;
	}

	return err;

free_req:
	xnvme_be_nvmf_req_free(queue->qpair->req_pool, req);
	return err;
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
