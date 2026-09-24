// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be_nvmf.h>

#define XNVME_MIN_CAPSULE_SIZE sizeof(struct xnvme_spec_cmd)
#define XNVME_MIN_COMPLETION_SIZE sizeof(struct xnvme_spec_cpl)
#define XNVME_BE_NVMF_MAX_QSIZE 4096

int
xnvme_be_nvmf_qpair_create(struct xnvme_be_nvmf_ctrlr *ctrlr, struct xnvme_be_nvmf_qpair_attr *attr,
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
	tmp->state = XNVME_NVMF_QPAIR_STATE_INIT;

	err = xnvme_be_nvmf_req_pool_alloc(&tmp->req_pool, attr->qsize);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_nvmf_req_pool_alloc(), err: %d", err);
		free(tmp);
		return err;
	}

	*qpair = tmp;
	return 0;
}

int
xnvme_be_nvmf_connect_qpair(struct xnvme_be_nvmf_qpair *qpair)
{
	int err;

	err = qpair->ops->connect(qpair);
	if (err) {
		XNVME_DEBUG("FAILED: transport connect, err: %d", err);
		return err;
	}

	// At this point, the transport has connected, but the fabric connect sequence is not complete.
	assert(qpair->state == XNVME_NVMF_QPAIR_STATE_CONNECTED);

	err = xnvme_be_nvmf_send_fabric_connect_command(qpair);
	if (err) {
		XNVME_DEBUG("FAILED: send fabric connect command, err: %d", err);
			return err;
		}

	if (qpair->attr.qid == 0) {
		// Admin queue
		// Initialize controller
		// 
	} else {
		// I/O queue
	}

	// TODO: Finish this 
	qpair->state = XNVME_NVMF_QPAIR_STATE_READY;

	return err;
}

int
xnvme_be_nvmf_disconnect_qpair(struct xnvme_be_nvmf_qpair *qpair)
{
	return qpair->ops->disconnect(qpair);
}

int
xnvme_be_nvmf_destroy_qpair(struct xnvme_be_nvmf_qpair *qpair)
{
	int err; 

	err = xnvme_be_nvmf_req_pool_free(qpair->req_pool);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_be_nvmf_req_pool_free(), err: %d", err);
		return err;
	}

	return qpair->ops->destroy(qpair);
}
