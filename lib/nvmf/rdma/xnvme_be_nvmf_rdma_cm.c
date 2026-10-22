// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <libxnvme.h>
#include <xnvme_be.h>

#include <errno.h>
#include <unistd.h>

#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>

#include <rdma/rdma_cma.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_qpair.h>
#include <xnvme_be_nvmf_debug.h>
#include <xnvme_be_nvmf_rdma.h>

/*
 * Bottom half: rdma_cm connection sequence. Transport-native, with no NVMe-oF
 * knowledge. It never calls up into the core.
 */

#define NVMF_DEBUG_CATEGORY NVMF_DEBUG_CATEGORY_RDMACM

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

static inline int
_nvmf_rdma_cm_resolve_route(struct rdma_cm_id *cm_id, struct ibv_pd *pd,
			    struct ibv_qp_init_attr *qp_init_attr)
{
	int err;

	err = rdma_create_qp(cm_id, pd, qp_init_attr);
	if (err) {
		NVMF_ERROR("FAILED: rdma_create_qp(), err: %d", err);
		return err;
	}

	err = rdma_resolve_route(cm_id, XNVME_BE_NVMF_MAX_RDMACM_TIMEOUT_MS);
	if (err) {
		NVMF_ERROR("FAILED: rdma_resolve_route(), err: %d", err);
		rdma_destroy_qp(cm_id);
		return err;
	}
	NVMF_DEBUG(
		"INFO: rdma_resolve_route() successful, waiting for RDMA_CM_EVENT_ROUTE_RESOLVED");

	return 0;
}

static int
_nvmf_rdma_cm_setup_qpair(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_NVMF_RDMA_QPAIR(qpair);
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr = TO_NVMF_RDMA_CTRLR(rdma_qpair->base.ctrlr);
	bool allocated_pd = false;
	int err;

	/*
	 * The PD is allocated lazily, once, on the admin qpair's transport
	 * connect. Later qpairs reuse it.
	 *
	 * On an error in this function, only a PD allocated by this call
	 * is freed here; a PD inherited from an earlier qpair may already be
	 * in use elsewhere and is left for ctrlr_teardown to free.
	 */
	if (!rdma_ctrlr->pd) {
		rdma_ctrlr->pd = ibv_alloc_pd(rdma_qpair->cm_id->verbs);
		if (!rdma_ctrlr->pd) {
			NVMF_ERROR("FAILED: ibv_alloc_pd(), err: %d", errno);
			return -errno;
		}
		allocated_pd = true;
	}

	rdma_qpair->send_cq =
		ibv_create_cq(rdma_qpair->cm_id->verbs, qpair->attr.qsize, NULL, NULL, 0);
	if (!rdma_qpair->send_cq) {
		NVMF_ERROR("FAILED: ibv_create_cq(), err: %d", errno);
		err = -errno;
		goto destroy_pd;
	}

	rdma_qpair->recv_cq =
		ibv_create_cq(rdma_qpair->cm_id->verbs, qpair->attr.qsize, NULL, NULL, 0);
	if (!rdma_qpair->recv_cq) {
		NVMF_ERROR("FAILED: ibv_create_cq(), err: %d", errno);
		err = -errno;
		goto destroy_send_cq;
	}

	rdma_qpair->qp_init_attr.send_cq = rdma_qpair->send_cq;
	rdma_qpair->qp_init_attr.recv_cq = rdma_qpair->recv_cq;

	err = _nvmf_rdma_cm_resolve_route(rdma_qpair->cm_id, rdma_ctrlr->pd,
					  &rdma_qpair->qp_init_attr);
	if (err) {
		NVMF_ERROR("FAILED: _nvmf_rdma_cm_resolve_route(), err: %d", err);
		goto destroy_recv_cq;
	}

	return 0;

destroy_recv_cq:
	ibv_destroy_cq(rdma_qpair->recv_cq);
destroy_send_cq:
	ibv_destroy_cq(rdma_qpair->send_cq);
destroy_pd:
	if (allocated_pd) {
		ibv_dealloc_pd(rdma_ctrlr->pd);
		rdma_ctrlr->pd = NULL;
	}

	return err;
}

static inline int
_nvmf_rdma_cm_send_connect(struct rdma_cm_id *cm_id, uint16_t qid, uint16_t qsize,
			   uint16_t ctrlr_id)
{
	int err;
	struct rdma_conn_param conn_param = {0};
	struct xnvme_rdma_cm_request_pdf private_data = {0};
	struct ibv_device_attr device_attr;

	err = ibv_query_device(cm_id->verbs, &device_attr);
	if (err) {
		NVMF_ERROR("FAILED: ibv_query_device(), err: %d", err);
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
		NVMF_ERROR("FAILED: rdma_connect(), err: %d", err);
		return err;
	}
	NVMF_DEBUG("INFO: rdma_connect() successful, waiting for RDMA_CM_EVENT_ESTABLISHED");

	return 0;
}

static int
_nvmf_rdma_cm_connect_qpair(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_NVMF_RDMA_QPAIR(qpair);
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr = TO_NVMF_RDMA_CTRLR(rdma_qpair->base.ctrlr);
	int err;

	rdma_qpair->send_buffer = calloc(qpair->attr.qsize, qpair->attr.capsule_size);
	if (!rdma_qpair->send_buffer) {
		NVMF_ERROR("FAILED: malloc() for send_buffer, err: %d", errno);
		return -ENOMEM;
	}

	rdma_qpair->recv_buffer = calloc(qpair->attr.qsize, qpair->attr.completion_size);
	if (!rdma_qpair->recv_buffer) {
		NVMF_ERROR("FAILED: malloc() for recv_buffer, err: %d", errno);
		err = -errno;
		goto free_send_buffer;
	}

	rdma_qpair->send_mr = ibv_reg_mr(rdma_ctrlr->pd, rdma_qpair->send_buffer,
					 qpair->attr.qsize * qpair->attr.capsule_size,
					 IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
						 IBV_ACCESS_REMOTE_WRITE);
	if (!rdma_qpair->send_mr) {
		NVMF_ERROR("FAILED: ibv_reg_mr() for send_buffer, err: %d", errno);
		err = -errno;
		goto free_recv_buffer;
	}

	rdma_qpair->recv_mr = ibv_reg_mr(rdma_ctrlr->pd, rdma_qpair->recv_buffer,
					 qpair->attr.qsize * qpair->attr.completion_size,
					 IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
						 IBV_ACCESS_REMOTE_WRITE);
	if (!rdma_qpair->recv_mr) {
		NVMF_ERROR("FAILED: ibv_reg_mr() for recv_buffer, err: %d", errno);
		err = -errno;
		goto dereg_send_mr;
	}

	for (int i = 0; i < qpair->attr.qsize; ++i) {
		struct ibv_sge sge = {
			.addr = (uintptr_t)rdma_qpair->recv_buffer +
				i * qpair->attr.completion_size,
			.length = qpair->attr.completion_size,
			.lkey = rdma_qpair->recv_mr->lkey,
		};
		struct xnvme_be_nvmf_wr_id wr_id = {0};

		wr_id.index = i;
		wr_id.type = XNVME_BE_NVMF_WR_TYPE_RECV;

		struct ibv_recv_wr recv_wr = {
			.wr_id = wr_id.raw,
			.sg_list = &sge,
			.num_sge = 1,
		};

		err = ibv_post_recv(rdma_qpair->cm_id->qp, &recv_wr, NULL);
		if (err) {
			NVMF_ERROR("FAILED: ibv_post_recv(), err: %d", err);
			goto destroy_qp;
		}
	}

	err = _nvmf_rdma_cm_send_connect(rdma_qpair->cm_id, rdma_qpair->base.attr.qid,
					 rdma_qpair->base.attr.qsize,
					 rdma_qpair->base.attr.cntlid);
	if (err) {
		NVMF_ERROR("FAILED: _nvmf_rdma_cm_send_connect(), err: %d", err);
		goto destroy_qp;
	}

	return 0;

destroy_qp:
	rdma_destroy_qp(rdma_qpair->cm_id);
	rdma_qpair->cm_id = NULL;

	ibv_dereg_mr(rdma_qpair->recv_mr);
	rdma_qpair->recv_mr = NULL;
dereg_send_mr:
	ibv_dereg_mr(rdma_qpair->send_mr);
	rdma_qpair->send_mr = NULL;

free_recv_buffer:
	free(rdma_qpair->recv_buffer);
	rdma_qpair->recv_buffer = NULL;

free_send_buffer:
	free(rdma_qpair->send_buffer);
	rdma_qpair->send_buffer = NULL;

	return err;
}

/* Waits for the next event on the qpair's channel, which must be of type @expected. */
static int
_nvmf_rdma_cm_wait_event(struct xnvme_be_nvmf_qpair *qpair, enum rdma_cm_event_type expected)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_NVMF_RDMA_QPAIR(qpair);
	struct rdma_cm_event *event;
	enum rdma_cm_event_type type;
	struct xnvme_timer timer;
	int err;

	xnvme_timer_start(&timer);
	while (rdma_get_cm_event(rdma_qpair->event_channel, &event)) {
		if (xnvme_timer_elapsed_msecs(&timer) >=
		    (double)XNVME_BE_NVMF_MAX_RDMACM_TIMEOUT_MS) {
			NVMF_ERROR("FAILED: rdma_get_cm_event() timed out");
			return -ETIMEDOUT;
		}

		// TODO: replace with non-blocking poll to avoid busy-wait
		usleep(1000);
	}

	type = event->event;
	err = rdma_ack_cm_event(event);
	if (err) {
		NVMF_ERROR("FAILED: rdma_ack_cm_event(), err: %d", err);
		return err;
	}

	if (type != expected) {
		NVMF_ERROR("FAILED: unexpected RDMA CM event: %s, expected: %s",
			   rdma_event_str(type), rdma_event_str(expected));
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return -EINVAL;
	}

	NVMF_DEBUG("INFO: %s", rdma_event_str(type));

	return 0;
}

int
xnvme_be_nvmf_rdma_cm_connect(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_NVMF_RDMA_QPAIR(qpair);
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr = TO_NVMF_RDMA_CTRLR(qpair->ctrlr);
	struct rdma_addrinfo *ai = rdma_ctrlr->selected;
	int err;

	err = rdma_create_id(rdma_qpair->event_channel, &rdma_qpair->cm_id, qpair,
			     ai->ai_port_space);
	if (err) {
		NVMF_ERROR("FAILED: rdma_create_id(), err: %d", err);
		return err;
	}

	err = rdma_resolve_addr(rdma_qpair->cm_id, NULL, ai->ai_dst_addr,
				XNVME_BE_NVMF_MAX_RDMACM_TIMEOUT_MS);
	if (err) {
		NVMF_ERROR("FAILED: rdma_resolve_addr(), err: %d", err);
		rdma_destroy_id(rdma_qpair->cm_id);
		rdma_qpair->cm_id = NULL;
		return err;
	}

	err = _nvmf_rdma_cm_wait_event(qpair, RDMA_CM_EVENT_ADDR_RESOLVED);
	if (err) {
		return err;
	}

	err = _nvmf_rdma_cm_setup_qpair(qpair);
	if (err) {
		NVMF_ERROR("FAILED: _nvmf_rdma_cm_setup_qpair(), err: %d", err);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return err;
	}
	qpair->state = XNVME_NVMF_QPAIR_STATE_CONNECTING;

	err = _nvmf_rdma_cm_wait_event(qpair, RDMA_CM_EVENT_ROUTE_RESOLVED);
	if (err) {
		return err;
	}

	err = _nvmf_rdma_cm_connect_qpair(qpair);
	if (err) {
		NVMF_ERROR("FAILED: _nvmf_rdma_cm_connect_qpair(), err: %d", err);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return err;
	}

	err = _nvmf_rdma_cm_wait_event(qpair, RDMA_CM_EVENT_ESTABLISHED);
	if (err) {
		return err;
	}
	qpair->state = XNVME_NVMF_QPAIR_STATE_CONNECTED;

	return 0;
}

int
xnvme_be_nvmf_rdma_cm_wait_disconnected(struct xnvme_be_nvmf_qpair *qpair)
{
	int err;

	err = _nvmf_rdma_cm_wait_event(qpair, RDMA_CM_EVENT_DISCONNECTED);
	if (err) {
		return err;
	}
	qpair->state = XNVME_NVMF_QPAIR_STATE_DISCONNECTED;

	return 0;
}
