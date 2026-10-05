// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>

#include <xnvme_cmd.h>
#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_qpair.h>
#include <xnvme_be_nvmf_req.h>
#include <xnvme_be_nvmf_fabric.h>
#include <xnvme_be_nvmf_debug.h>

#define XNVME_MIN_CAPSULE_SIZE sizeof(struct xnvme_spec_cmd)
#define XNVME_MIN_COMPLETION_SIZE sizeof(struct xnvme_spec_cpl)
#define XNVME_BE_NVMF_MAX_QSIZE 4096

#define NVMF_DEBUG_CATEGORY NVMF_DEBUG_CATEGORY_CORE_QPAIR

int
xnvme_be_nvmf_qpair_create(struct xnvme_be_nvmf_ctrlr *ctrlr, struct xnvme_dev *dev,
			   struct xnvme_be_nvmf_qpair_attr *attr,
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

	err = ctrlr->ops->qpair_alloc(ctrlr, &tmp);
	if (err) {
		return err;
	}

	tmp->attr = *attr;
	tmp->ctrlr = ctrlr;
	tmp->dev = dev;
	tmp->state = XNVME_NVMF_QPAIR_STATE_INIT;

	err = tmp->ops->qpair_init(tmp);
	if (err) {
		NVMF_DEBUG("FAILED: qpair->ops->qpair_init(), err: %d", err);
		tmp->ops->qpair_free(tmp);
		return err;
	}

	err = xnvme_be_nvmf_req_pool_alloc(&tmp->req_pool, attr->qsize);
	if (err) {
		NVMF_DEBUG("FAILED: xnvme_be_nvmf_req_pool_alloc(), err: %d", err);
		tmp->ops->qpair_teardown(tmp);
		tmp->ops->qpair_free(tmp);
		return err;
	}

	*qpair = tmp;
	return 0;
}

int
xnvme_be_nvmf_qpair_connect(struct xnvme_be_nvmf_qpair *qpair)
{
	int err;

	err = qpair->ops->qpair_connect(qpair);
	if (err) {
		NVMF_DEBUG("FAILED: transport connect, err: %d", err);
		return err;
	}

	// At this point, the transport has connected, but the fabric connect sequence is not
	// complete.
	assert(qpair->state == XNVME_NVMF_QPAIR_STATE_CONNECTED);

	err = xnvme_be_nvmf_fabric_connect(qpair);
	if (err) {
		NVMF_DEBUG("FAILED: send fabric connect command, err: %d", err);
		return err;
	}

	qpair->state = XNVME_NVMF_QPAIR_STATE_READY;

	return err;
}

int
xnvme_be_nvmf_qpair_disconnect(struct xnvme_be_nvmf_qpair *qpair)
{
	return qpair->ops->qpair_disconnect(qpair);
}

int
xnvme_be_nvmf_qpair_destroy(struct xnvme_be_nvmf_qpair *qpair)
{
	int err;

	err = xnvme_be_nvmf_req_pool_free(qpair->req_pool);
	if (err) {
		NVMF_DEBUG("FAILED: nvme_be_nvmf_req_pool_free(), err: %d", err);
		return err;
	}

	err = qpair->ops->qpair_teardown(qpair);
	if (err) {
		NVMF_DEBUG("FAILED: qpair->ops->qpair_teardown(), err: %d", err);
		return err;
	}

	qpair->ops->qpair_free(qpair);

	return 0;
}

int
xnvme_be_nvmf_qpair_complete(struct xnvme_be_nvmf_qpair *qpair, const struct xnvme_spec_cpl *cpl)
{
	struct xnvme_be_nvmf_req *req;
	struct xnvme_cmd_ctx *ctx;

	req = xnvme_be_nvmf_req_get(qpair->req_pool, cpl->cid);
	if (!req) {
		NVMF_DEBUG("FAILED: xnvme_be_nvmf_req_get() for cid: %u", cpl->cid);
		return -EINVAL;
	}

	ctx = (struct xnvme_cmd_ctx *)req->context;
	ctx->cpl = *cpl;

	NVMF_DEBUG("INFO: Completing request for cid: %u", ctx->cpl.cid);
	_hexdump_range(NVMF_DEBUG_CATEGORY_FABRICS, &ctx->cpl, sizeof(ctx->cpl));

	req->cmpl_type = XNVME_BE_NVMF_REQ_CMPL_TYPE_RECV;
	req->status = 0;

	return 0;
}
