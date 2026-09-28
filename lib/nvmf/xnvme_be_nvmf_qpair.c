// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>

#include <xnvme_cmd.h>
#include <xnvme_be_nvmf.h>

#define XNVME_MIN_CAPSULE_SIZE sizeof(struct xnvme_spec_cmd)
#define XNVME_MIN_COMPLETION_SIZE sizeof(struct xnvme_spec_cpl)
#define XNVME_BE_NVMF_MAX_QSIZE 4096

#define _NVMF_ERROR(fmt, ...) NVMF_ERROR(NVMF_DEBUG_CATEGORY_CORE_QPAIR, fmt, ##__VA_ARGS__)
#define _NVMF_DEBUG(fmt, ...) NVMF_DEBUG(NVMF_DEBUG_CATEGORY_CORE_QPAIR, fmt, ##__VA_ARGS__)

#if 0
static void
_xnvme_be_nvmf_on_send_cmpl(struct xnvme_be_nvmf_qpair *qpair, struct xnvme_be_nvmf_req *req, int status)
{	
	return; // no-op
}

static void
_xnvme_be_nvmf_on_recv_cmpl(struct xnvme_be_nvmf_qpair *qpair, struct xnvme_be_nvmf_req *req, int status)
{
	struct xnvme_cmd_ctx *ctx = req->context;
	XNVME_DEBUG("INFO: Receive completion received for req: %p, status: %d", req, status);
	assert(qpair != NULL);
	assert(req != NULL);

	switch(req->type) {
		case XNVME_BE_NVMF_REQ_TYPE_USER:
			XNVME_DEBUG("INFO: User request receive completed");
			if (ctx->opts & XNVME_CMD_ASYNC) {
				/* Handle asynchronous completion */
				struct xnvme_be_nvmf_queue *async_queue = (struct xnvme_be_nvmf_queue *) ctx->async.queue;
				async_queue->base.outstanding--;
				async_queue->completions_pending++;

				if (ctx->async.cb) {
					ctx->async.cb(ctx, ctx->async.cb_arg);
				}
			}
			break;
		case XNVME_BE_NVMF_REQ_TYPE_INTERNAL:
			XNVME_DEBUG("INFO: Internal request receive completed");
			break;
		default:
			XNVME_DEBUG("INFO: Unknown request type receive completed");
			qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
			break;
	}
	return;
}
#endif

int
xnvme_be_nvmf_qpair_create(struct xnvme_be_nvmf_ctrlr *ctrlr, struct xnvme_dev *dev, struct xnvme_be_nvmf_qpair_attr *attr,
			   struct xnvme_be_nvmf_qpair **qpair)
{
	struct xnvme_be_nvmf_qpair *tmp;
	int err;

    if (!attr) {
		return -EINVAL;
	}

	if (attr->qsize == 0 || attr->qsize > XNVME_BE_NVMF_MAX_QSIZE) {
		return -EINVAL;
	}

	if (attr->capsule_size < XNVME_MIN_CAPSULE_SIZE) {
		return -EINVAL;
	}

	if (attr->completion_size < XNVME_MIN_COMPLETION_SIZE) {
		return -EINVAL;
	}

	err = ctrlr->ops->create_qpair(ctrlr, attr, &tmp);
	if (err) {
		return err;
	}

	tmp->attr = *attr;
	tmp->ctrlr = ctrlr;
	tmp->dev = dev;
	tmp->state = XNVME_NVMF_QPAIR_STATE_INIT;

	err = xnvme_be_nvmf_req_pool_alloc(&tmp->req_pool, attr->qsize);
	if (err) {
		_NVMF_ERROR("FAILED: xnvme_be_nvmf_req_pool_alloc(), err: %d", err);
		free(tmp);
		return err;
	}

	*qpair = tmp;
	return 0;
}

int
xnvme_be_nvmf_qpair_connect(struct xnvme_be_nvmf_qpair *qpair)
{
	int err;

	err = qpair->ops->connect(qpair);
	if (err) {
		_NVMF_ERROR("FAILED: transport connect, err: %d", err);
		return err;
	}

	// At this point, the transport has connected, but the fabric connect sequence is not complete.
	assert(qpair->state == XNVME_NVMF_QPAIR_STATE_CONNECTED);

	err = xnvme_be_nvmf_send_fabric_connect_command(qpair);
	if (err) {
		_NVMF_ERROR("FAILED: send fabric connect command, err: %d", err);
		return err;
	}

	qpair->state = XNVME_NVMF_QPAIR_STATE_READY;

	return err;
}

int
xnvme_be_nvmf_qpair_disconnect(struct xnvme_be_nvmf_qpair *qpair)
{
	return qpair->ops->disconnect(qpair);
}

int
xnvme_be_nvmf_qpair_destroy(struct xnvme_be_nvmf_qpair *qpair)
{
	int err; 

	err = xnvme_be_nvmf_req_pool_free(qpair->req_pool);
	if (err) {
		_NVMF_ERROR("FAILED: nvme_be_nvmf_req_pool_free(), err: %d", err);
		return err;
	}

	return qpair->ops->destroy(qpair);
}
