// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <libxnvme.h>
#include <xnvme_be.h>

#ifdef XNVME_BE_NVMF_ENABLED
#include <errno.h>

#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>

#include <rdma/rdma_cma.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_rdma.h>

#define XNVME_BE_NVMF_MAX_RDMACM_TIMEOUT_MS 2000

struct xnvme_rdma_cm_request_pdf {
	uint16_t recfmt;
	uint16_t qid;
	uint16_t hrqsize;
	uint16_t hsqsize;
	uint16_t cntlid;
	uint8_t rsvd[22];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_rdma_cm_request_pdf) == 32,
		    "struct xnvme_rdma_cm_request_pdf is not 32 bytes");

typedef int (*xnvme_be_nvmf_qpair_state_fn)(struct xnvme_be_nvmf_qpair *qpair,
					    struct rdma_cm_event *event);

static int
_qpair_invalid_state_fn(struct xnvme_be_nvmf_qpair *qpair, struct rdma_cm_event *event)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	assert(qpair->state == XNVME_NVMF_QPAIR_STATE_INVALID);
	assert(rdma_qpair->rdma_qp_state == XNVME_NVMF_RDMACM_STATE_INVALID);
	assert(event == NULL);

	return -EINVAL;
}

static inline int
_resolve_route(struct rdma_cm_id *cm_id, struct ibv_pd *pd, struct ibv_qp_init_attr *qp_init_attr)
{
	int err;

	err = rdma_create_qp(cm_id, pd, qp_init_attr);
	if (err) {
		XNVME_DEBUG("FAILED: rdma_create_qp(), err: %d", err);
		return err;
	}

	err = rdma_resolve_route(cm_id, XNVME_BE_NVMF_MAX_RDMACM_TIMEOUT_MS);
	if (err) {
		XNVME_DEBUG("FAILED: rdma_resolve_route(), err: %d", err);
		rdma_destroy_qp(cm_id);
		return err;
	}
	XNVME_DEBUG(
		"INFO: rdma_resolve_route() successful, waiting for RDMA_CM_EVENT_ROUTE_RESOLVED");

	return 0;
}

static int
_resolve_rdma_route(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr =
		TO_XNVME_NVMF_RDMA_CTRLR(rdma_qpair->base.ctrlr);

	int err;

	rdma_ctrlr->pd = ibv_alloc_pd(rdma_qpair->cm_id->verbs);
	if (!rdma_ctrlr->pd) {
		XNVME_DEBUG("FAILED: ibv_alloc_pd(), err: %d", errno);
		return -errno;
	}

	rdma_qpair->send_cq =
		ibv_create_cq(rdma_qpair->cm_id->verbs, qpair->attr.qsize, NULL, NULL, 0);
	if (!rdma_qpair->send_cq) {
		XNVME_DEBUG("FAILED: ibv_create_cq(), err: %d", errno);
		err = -errno;
		goto destroy_pd;
	}

	rdma_qpair->recv_cq =
		ibv_create_cq(rdma_qpair->cm_id->verbs, qpair->attr.qsize, NULL, NULL, 0);
	if (!rdma_qpair->recv_cq) {
		XNVME_DEBUG("FAILED: ibv_create_cq(), err: %d", errno);
		err = -errno;
		goto destroy_send_cq;
	}

	rdma_qpair->qp_init_attr.send_cq = rdma_qpair->send_cq;
	rdma_qpair->qp_init_attr.recv_cq = rdma_qpair->recv_cq;

	err = _resolve_route(rdma_qpair->cm_id, rdma_ctrlr->pd, &rdma_qpair->qp_init_attr);
	if (err) {
		XNVME_DEBUG("FAILED: _resolve_route(), err: %d", err);
		goto destroy_recv_cq;
	}

	return 0;

destroy_recv_cq:
	ibv_destroy_cq(rdma_qpair->recv_cq);
destroy_send_cq:
	ibv_destroy_cq(rdma_qpair->send_cq);
destroy_pd:
	ibv_dealloc_pd(rdma_ctrlr->pd);

	return err;
}

static int
_qpair_resolve_address_state_fn(struct xnvme_be_nvmf_qpair *qpair, struct rdma_cm_event *event)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	int err;

	assert(qpair->state == XNVME_NVMF_QPAIR_STATE_INIT);
	assert(rdma_qpair->rdma_qp_state == XNVME_NVMF_RDMACM_STATE_RESOLVE_ADDRESS);
	assert(event != NULL);

	switch (event->event) {
	case RDMA_CM_EVENT_ADDR_RESOLVED:
		XNVME_DEBUG("INFO: RDMA_CM_EVENT_ADDR_RESOLVED");
		err = _resolve_rdma_route(qpair);
		if (err) {
			XNVME_DEBUG("FAILED: _resolve_rdma_route(), err: %d", err);
			return err;
		}
		qpair->state = XNVME_NVMF_QPAIR_STATE_CONNECTING;
		rdma_qpair->rdma_qp_state = XNVME_NVMF_RDMACM_STATE_RESOLVE_ROUTE;
		break;
	default:
		XNVME_DEBUG("FAILED: Unexpected RDMA CM event: %d", event->event);
		return -EINVAL;
	}

	return 0;
}

static inline int
_connect(struct rdma_cm_id *cm_id, uint16_t qid, uint16_t qsize, uint16_t ctrlr_id)
{
	int err;
	struct rdma_conn_param conn_param = {0};
	struct xnvme_rdma_cm_request_pdf private_data = {0};
	struct ibv_device_attr device_attr;

	err = ibv_query_device(cm_id->verbs, &device_attr);
	if (err) {
		XNVME_DEBUG("FAILED: ibv_query_device(), err: %d", err);
		return err;
	}

	conn_param.private_data = &private_data;
	conn_param.private_data_len = sizeof(private_data);
	conn_param.responder_resources = device_attr.max_qp_rd_atom;
	conn_param.retry_count = 3;
	conn_param.rnr_retry_count = 7;

	private_data.recfmt = 0;
	private_data.qid = qid;
	private_data.hrqsize = qsize;
	private_data.hsqsize = qsize;
	private_data.cntlid = ctrlr_id;

	err = rdma_connect(cm_id, &conn_param);
	if (err) {
		XNVME_DEBUG("FAILED: rdma_connect(), err: %d", err);
		return err;
	}
	XNVME_DEBUG("INFO: rdma_connect() successful, waiting for RDMA_CM_EVENT_ESTABLISHED");

	return 0;
}

static int
_connect_rdma_qpair(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr =
		TO_XNVME_NVMF_RDMA_CTRLR(rdma_qpair->base.ctrlr);
	int err;

	rdma_qpair->send_buffer = calloc(qpair->attr.qsize, qpair->attr.capsule_size);
	if (!rdma_qpair->send_buffer) {
		XNVME_DEBUG("FAILED: malloc() for send_buffer, err: %d", errno);
		return -ENOMEM;
	}

	rdma_qpair->recv_buffer = calloc(qpair->attr.qsize, qpair->attr.completion_size);
	if (!rdma_qpair->recv_buffer) {
		XNVME_DEBUG("FAILED: malloc() for recv_buffer, err: %d", errno);
		goto free_send_buffer;
	}

	rdma_qpair->send_mr = ibv_reg_mr(
		rdma_ctrlr->pd, rdma_qpair->send_buffer, qpair->attr.qsize * qpair->attr.capsule_size,
		IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
	if (!rdma_qpair->send_mr) {
		XNVME_DEBUG("FAILED: ibv_reg_mr() for send_buffer, err: %d", errno);
		goto free_recv_buffer;
	}

	rdma_qpair->recv_mr = ibv_reg_mr(
		rdma_ctrlr->pd, rdma_qpair->recv_buffer, qpair->attr.qsize * qpair->attr.completion_size,
		IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
	if (!rdma_qpair->recv_mr) {
		XNVME_DEBUG("FAILED: ibv_reg_mr() for recv_buffer, err: %d", errno);
		goto dereg_send_mr;
	}

	rdma_qpair->internal.size = 4096;
	rdma_qpair->internal.buffer =
		calloc(1, rdma_qpair->internal.size); // Allocate 4KB for internal buffer
	if (!rdma_qpair->internal.buffer) {
		XNVME_DEBUG("FAILED: malloc() for internal buffer, err: %d", errno);
		goto dereg_recv_mr;
	}

	rdma_qpair->internal.mr = ibv_reg_mr(
		rdma_ctrlr->pd, rdma_qpair->internal.buffer, rdma_qpair->internal.size,
		IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
	if (!rdma_qpair->internal.mr) {
		XNVME_DEBUG("FAILED: ibv_reg_mr() for internal buffer, err: %d", errno);
		goto free_internal_buffer;
	}

	for (int i = 0; i < qpair->attr.qsize; ++i) {
		struct ibv_sge sge = {
			.addr = (uintptr_t)rdma_qpair->recv_buffer + i * qpair->attr.completion_size,
			.length = qpair->attr.completion_size,
			.lkey = rdma_qpair->recv_mr->lkey,
		};
		struct xnvme_be_nvmf_wr_id wr_id = {
			.index = i,
			.type = XNVME_BE_NVMF_WR_TYPE_RECV,
		};
		struct ibv_recv_wr recv_wr = {
			.wr_id = wr_id.raw,
			.sg_list = &sge,
			.num_sge = 1,
		};

		err = ibv_post_recv(rdma_qpair->cm_id->qp, &recv_wr, NULL);
		if (err) {
			XNVME_DEBUG("FAILED: ibv_post_recv(), err: %d", err);
			goto dereg_internal_buffer_mr;
		}
	}

	err = _connect(rdma_qpair->cm_id, rdma_qpair->base.attr.qid, rdma_qpair->base.attr.qsize, rdma_qpair->base.ctrlr->ctrlr_id);
	if (err) {
		XNVME_DEBUG("FAILED: _connect(), err: %d", err);
		goto dereg_internal_buffer_mr;
	}

	return 0;

dereg_internal_buffer_mr:
	ibv_dereg_mr(rdma_qpair->internal.mr);
free_internal_buffer:
	free(rdma_qpair->internal.buffer);
destroy_qp:
	rdma_destroy_qp(rdma_qpair->cm_id);
dereg_recv_mr:
	ibv_dereg_mr(rdma_qpair->recv_mr);
dereg_send_mr:
	ibv_dereg_mr(rdma_qpair->send_mr);
free_recv_buffer:
	free(rdma_qpair->recv_buffer);
free_send_buffer:
	free(rdma_qpair->send_buffer);

	return err;
}

static int
_qpair_resolve_route_state_fn(struct xnvme_be_nvmf_qpair *qpair, struct rdma_cm_event *event)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	int err;

	assert(qpair->state == XNVME_NVMF_QPAIR_STATE_CONNECTING);
	assert(rdma_qpair->rdma_qp_state == XNVME_NVMF_RDMACM_STATE_RESOLVE_ROUTE);
	assert(event != NULL);

	switch (event->event) {
	case RDMA_CM_EVENT_ROUTE_RESOLVED:
		XNVME_DEBUG("INFO: RDMA_CM_EVENT_ROUTE_RESOLVED");
		err = _connect_rdma_qpair(qpair);
		if (err) {
			XNVME_DEBUG("FAILED: _connect_rdma_qpair(), err: %d", err);
			return err;
		}
		rdma_qpair->rdma_qp_state = XNVME_NVMF_RDMACM_STATE_CONNECTING;
		break;
	default:
		XNVME_DEBUG("FAILED: Unexpected RDMA CM event: %d", event->event);
		return -EINVAL;
	}

	return 0;
}

static int
_qpair_connecting_state_fn(struct xnvme_be_nvmf_qpair *qpair, struct rdma_cm_event *event)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);

	assert(qpair->state == XNVME_NVMF_QPAIR_STATE_CONNECTING);
	assert(rdma_qpair->rdma_qp_state == XNVME_NVMF_RDMACM_STATE_CONNECTING);
	assert(event != NULL);

	switch (event->event) {
	case RDMA_CM_EVENT_ESTABLISHED:
		XNVME_DEBUG("INFO: RDMA_CM_EVENT_ESTABLISHED");
		qpair->state = XNVME_NVMF_QPAIR_STATE_CONNECTED;
		rdma_qpair->rdma_qp_state = XNVME_NVMF_RDMACM_STATE_CONNECTED;
		if (qpair->on_state_change) {
			qpair->on_state_change(qpair, XNVME_NVMF_QPAIR_STATE_CONNECTED, NULL);
		}
		break;
	default:
		XNVME_DEBUG("FAILED: Unexpected RDMA CM event: %d", event->event);
		return -EINVAL;
	}

	return 0;
}

static int
_qpair_connected_state_fn(struct xnvme_be_nvmf_qpair *qpair, struct rdma_cm_event *event)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);

	assert(qpair->state == XNVME_NVMF_QPAIR_STATE_CONNECTED ||
	       qpair->state == XNVME_NVMF_QPAIR_STATE_READY);
	assert(rdma_qpair->rdma_qp_state == XNVME_NVMF_RDMACM_STATE_CONNECTED);
	assert(event != NULL);

	switch (event->event) {
	case RDMA_CM_EVENT_DISCONNECTED:
		XNVME_DEBUG("INFO: RDMA_CM_EVENT_DISCONNECTED");
		qpair->state = XNVME_NVMF_QPAIR_STATE_DISCONNECTED;
		rdma_qpair->rdma_qp_state = XNVME_NVMF_RDMACM_STATE_DISCONNECTED;
		if (qpair->on_state_change) {
			qpair->on_state_change(qpair, XNVME_NVMF_QPAIR_STATE_DISCONNECTED, NULL);
		}
		break;
	default:
		XNVME_DEBUG("FAILED: Unexpected RDMA CM event: %d", event->event);
		return -EINVAL;
	}

	return 0;
}

static int
_qpair_disconnected_state_fn(struct xnvme_be_nvmf_qpair *qpair, struct rdma_cm_event *event)
{
	assert(qpair->state == XNVME_NVMF_QPAIR_STATE_DISCONNECTED);
	assert(event != NULL);

	XNVME_DEBUG("INFO: QPair disconnected, event: %d", event->event);

	return 0;
}

static int
_qpair_error_state_fn(struct xnvme_be_nvmf_qpair *qpair, struct rdma_cm_event *event)
{
	assert(qpair->state == XNVME_NVMF_QPAIR_STATE_ERROR);
	assert(event != NULL);

	XNVME_DEBUG("ERROR: QPair in error state, event: %d", event->event);

	return 0;
}

static xnvme_be_nvmf_qpair_state_fn
	g_xnvme_be_nvmf_rdmacm_state_fns[XNVME_NVMF_RDMACM_STATE_MAX + 1] = {
		[XNVME_NVMF_RDMACM_STATE_INVALID] = _qpair_invalid_state_fn,
		[XNVME_NVMF_RDMACM_STATE_RESOLVE_ADDRESS] = _qpair_resolve_address_state_fn,
		[XNVME_NVMF_RDMACM_STATE_RESOLVE_ROUTE] = _qpair_resolve_route_state_fn,
		[XNVME_NVMF_RDMACM_STATE_CONNECTING] = _qpair_connecting_state_fn,
		[XNVME_NVMF_RDMACM_STATE_CONNECTED] = _qpair_connected_state_fn,
		[XNVME_NVMF_RDMACM_STATE_DISCONNECTED] = _qpair_disconnected_state_fn,
		[XNVME_NVMF_RDMACM_STATE_ERROR] = _qpair_error_state_fn,
};

int
_handle_rdmacm_event(struct rdma_cm_event *event)
{
	struct xnvme_be_nvmf_qpair *qpair = (struct xnvme_be_nvmf_qpair *)event->id->context;
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	int err;

	if (!qpair) {
		XNVME_DEBUG("FAILED: No qpair associated with RDMA CM event");
		return -EINVAL;
	}

	err = g_xnvme_be_nvmf_rdmacm_state_fns[rdma_qpair->rdma_qp_state](qpair, event);
	if (err) {
		XNVME_DEBUG("FAILED: State handler for qpair state %d returned error: %d",
			    qpair->state, err);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return err;
	}

	return 0;
}
#endif // XNVME_BE_NVMF_ENABLED
