// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <assert.h>
#include <inttypes.h>

#include <libxnvme.h>
#include <xnvme_cmd.h>

#include <xnvme_transport_base.h>
#include <xnvme_transport_debug.h>
#include <xnvme_transport_ctrlr.h>
#include <xnvme_transport_fabric.h>
#include <xnvme_transport_ns.h>
#include <xnvme_transport_qpair.h>

#define XNVME_MIN_CAPSULE_SIZE sizeof(union xnvme_spec_sqe)
#define XNVME_MIN_COMPLETION_SIZE sizeof(struct xnvme_spec_cpl)
#define XNVME_TRANSPORT_MAX_QSIZE 4096

#define TRANSPORT_DEBUG_CATEGORY TRANSPORT_DEBUG_CATEGORY_CORE_QPAIR

int
xnvme_transport_qpair_create(struct xnvme_transport_ctrlr *ctrlr, struct xnvme_dev *dev,
			     struct xnvme_transport_qpair_attr *attr,
			     struct xnvme_transport_qpair **qpair)
{
	struct xnvme_transport_qpair *tmp;
	int err;

	if (!attr) {
		return -EINVAL;
	}

	if (attr->qsize == 0 || attr->qsize > XNVME_TRANSPORT_MAX_QSIZE) {
		return -EINVAL;
	}

	if (attr->capsule_size < XNVME_MIN_CAPSULE_SIZE) {
		return -EINVAL;
	}

	if (attr->completion_size < XNVME_MIN_COMPLETION_SIZE) {
		return -EINVAL;
	}

	err = ctrlr->ops->ctrlr_qpair_alloc(ctrlr, &tmp);
	if (err) {
		return err;
	}

	tmp->attr = *attr;
	tmp->ctrlr = ctrlr;
	tmp->dev = dev;
	tmp->state = XNVME_TRANSPORT_QPAIR_STATE_INIT;

	err = tmp->ops->qpair_init(tmp);
	if (err) {
		TRANSPORT_ERROR("FAILED: qpair->ops->qpair_init(), err: %d", err);
		tmp->ops->qpair_free(tmp);
		return err;
	}

	err = xnvme_transport_req_pool_alloc(&tmp->req_pool, attr->qsize);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_req_pool_alloc(), err: %d", err);
		tmp->ops->qpair_teardown(tmp);
		tmp->ops->qpair_free(tmp);
		return err;
	}

	*qpair = tmp;
	return 0;
}

int
xnvme_transport_qpair_create_io(struct xnvme_transport_ns *ns, struct xnvme_dev *dev,
				uint32_t qsize, struct xnvme_transport_qpair **qpair)
{
	struct xnvme_transport_qpair_attr attr = {
		.capsule_size = NVME_CMD_CAPSULE_SIZE,
		.completion_size = NVME_CPL_CAPSULE_SIZE,
		.cntlid = ns->admin_qpair->attr.cntlid,
	};
	struct xnvme_transport_qpair *tmp;
	int err;

	// The Connect command carries qsize as SQSIZE, which the controller bounds by CAP.MQES
	if (!qsize || qsize > XNVME_TRANSPORT_MAX_QSIZE || qsize > ns->mqes) {
		TRANSPORT_ERROR("FAILED: qsize: %" PRIu32 " exceeds MQES: %u", qsize, ns->mqes);
		return -EINVAL;
	}
	attr.qsize = (uint16_t)qsize;

	err = xnvme_transport_ns_alloc_next_qid(ns, &attr.qid);
	if (err < 0) {
		TRANSPORT_ERROR("FAILED: no free queue identifiers");
		return err;
	}

	TRANSPORT_DEBUG("INFO: Allocating QID: %u", attr.qid);

	err = xnvme_transport_qpair_create(ns->admin_qpair->ctrlr, dev, &attr, &tmp);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_create(), err: %d", err);
		return err;
	}

	ns->qidmap &= ~(1ULL << (attr.qid - 1));

	*qpair = tmp;
	return 0;
}

int
xnvme_transport_qpair_connect(struct xnvme_transport_qpair *qpair)
{
	int err;

	err = qpair->ops->qpair_connect(qpair);
	if (err) {
		TRANSPORT_ERROR("FAILED: transport connect, err: %d", err);
		return err;
	}

	// At this point, the transport has connected, but the fabric connect sequence is not
	// complete.
	assert(qpair->state == XNVME_TRANSPORT_QPAIR_STATE_CONNECTED);

	err = xnvme_transport_fabric_connect(qpair);
	if (err) {
		TRANSPORT_ERROR("FAILED: send fabric connect command, err: %d", err);
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
		return err;
	}

	qpair->state = XNVME_TRANSPORT_QPAIR_STATE_READY;

	return err;
}

int
xnvme_transport_qpair_disconnect(struct xnvme_transport_qpair *qpair)
{
	return qpair->ops->qpair_disconnect(qpair);
}

int
xnvme_transport_qpair_destroy(struct xnvme_transport_qpair *qpair)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(qpair->dev);
	struct xnvme_transport_ns *ns = qpair->dev ? state->ns : NULL;
	int err;

	// A connected qpair is disconnected first, a failure is logged and does not stop the
	// destroy
	if (xnvme_transport_qpair_is_connected(qpair)) {
		err = xnvme_transport_qpair_disconnect(qpair);
		if (err) {
			TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_disconnect(), err: %d",
					err);
		}
	}

	// Requests still in flight cannot complete once the qpair is gone, so they are discarded
	if (qpair->req_pool->allocated) {
		TRANSPORT_ERROR("FAILED: discarding %" PRIu64 " outstanding requests",
				qpair->req_pool->allocated);
	}
	xnvme_transport_req_pool_free(qpair->req_pool);
	qpair->req_pool = NULL;

	// Return the qid to the namespace's qidmap
	if (ns && qpair->attr.qid) {
		xnvme_transport_ns_free_qid(ns, qpair->attr.qid);
	}

	// Free even when the teardown fails, the qpair cannot be used again either way
	err = qpair->ops->qpair_teardown(qpair);
	if (err) {
		TRANSPORT_ERROR("FAILED: qpair->ops->qpair_teardown(), err: %d", err);
	}

	qpair->ops->qpair_free(qpair);

	return err;
}

int
xnvme_transport_qpair_complete(struct xnvme_transport_qpair *qpair,
			       const struct xnvme_spec_cpl *cpl)
{
	struct xnvme_transport_req *req;
	struct xnvme_cmd_ctx *ctx;

	req = xnvme_transport_req_get(qpair->req_pool, cpl->cid);
	if (!req) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_req_get() for cid: %u", cpl->cid);
		return -EINVAL;
	}

	ctx = (struct xnvme_cmd_ctx *)req->context;
	ctx->cpl = *cpl;

	TRANSPORT_DEBUG("INFO: Completing request for cid: %u", ctx->cpl.cid);
	xnvme_transport_hexdump_range(TRANSPORT_DEBUG_CATEGORY_FABRICS, &ctx->cpl,
				      sizeof(ctx->cpl));

	req->complete = 1;
	req->status = 0;

	if (ctx->async.queue) {
		// do a user level completion
		ctx->async.queue->base.outstanding--;
		xnvme_transport_req_free(qpair->req_pool, req);

		ctx->async.cb(ctx, ctx->async.cb_arg);
	}

	return 0;
}
