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
#define NVME_CMD_CAPSULE_SIZE sizeof(struct xnvme_spec_cmd_common)
#define NVME_CPL_CAPSULE_SIZE sizeof(struct xnvme_spec_cpl)

typedef int (*xnvme_be_nvmf_ib_cmpl_fn)(struct xnvme_be_nvmf_qpair *qpair, struct ibv_wc *wc);

static struct xnvme_be_nvmf_qpair_ops g_xnvme_be_nvmf_rdma_qpair_ops;

static inline int
_destroy_rdma_qpair(struct xnvme_be_nvmf_rdma_qpair *qpair)
{
	int err = 0;

	if (!qpair || !qpair->cm_id) {
		XNVME_DEBUG("INFO: No qpair to destroy");
		return 0;
	}

	if (qpair->send_mr) {
		err = ibv_dereg_mr(qpair->send_mr);
		if (err) {
			XNVME_DEBUG("FAILED: ibv_dereg_mr() for send_mr, err: %d", err);
			return err;
		}
	}

	if (qpair->recv_mr) {
		err = ibv_dereg_mr(qpair->recv_mr);
		if (err) {
			XNVME_DEBUG("FAILED: ibv_dereg_mr() for recv_mr, err: %d", err);
			return err;
		}
	}

	if (qpair->send_buffer) {
		free(qpair->send_buffer);
		qpair->send_buffer = NULL;
	}

	if (qpair->recv_buffer) {
		free(qpair->recv_buffer);
		qpair->recv_buffer = NULL;
	}

	if (qpair->cm_id) {
		rdma_destroy_qp(qpair->cm_id);
		err = rdma_destroy_id(qpair->cm_id);
		if (err) {
			XNVME_DEBUG("FAILED: rdma_destroy_id(), err: %d", err);
			return err;
		}

		qpair->cm_id = NULL;
	}

	if (qpair->send_cq) {
		err = ibv_destroy_cq(qpair->send_cq);
		if (err) {
			XNVME_DEBUG("FAILED: ibv_destroy_cq() for send_cq, err: %d", err);
			return err;
		}
		qpair->send_cq = NULL;
	}

	if (qpair->recv_cq) {
		err = ibv_destroy_cq(qpair->recv_cq);
		if (err) {
			XNVME_DEBUG("FAILED: ibv_destroy_cq() for recv_cq, err: %d", err);
			return err;
		}
		qpair->recv_cq = NULL;
	}

	return 0;
}

static int
_handle_send_cmpl(struct xnvme_be_nvmf_qpair *qpair, struct ibv_wc *wc)
{
	int status = (wc->status == IBV_WC_SUCCESS) ? 0 : -EIO;
	/* wr_id carries the original buffer pointer set in _rdma_send_capsule. */
	void *buf = (void *)(uintptr_t)wc->wr_id;

	if (wc->status != IBV_WC_SUCCESS) {
		XNVME_DEBUG("FAILED: send WC error: %s", ibv_wc_status_str(wc->status));
	}

	if (qpair->on_send_cmpl) {
		qpair->on_send_cmpl(qpair, buf, status);
	}

	return 0;
}

static int
_handle_recv_cmpl(struct xnvme_be_nvmf_qpair *qpair, struct ibv_wc *wc)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	struct xnvme_be_nvmf_wr_id wr_id = {.raw = wc->wr_id};
	void *buf;
	struct ibv_sge sge;
	struct ibv_recv_wr recv_wr;
	int err;

	if (wc->status != IBV_WC_SUCCESS) {
		XNVME_DEBUG("FAILED: recv WC error: %s", ibv_wc_status_str(wc->status));
		return -EIO;
	}

	buf = rdma_qpair->recv_buffer + wr_id.index * NVME_CPL_CAPSULE_SIZE;

	/* Re-post the slot before the callback so the pool never drains. */
	sge = (struct ibv_sge){
		.addr = (uintptr_t)buf,
		.length = NVME_CPL_CAPSULE_SIZE,
		.lkey = rdma_qpair->recv_mr->lkey,
	};
	recv_wr = (struct ibv_recv_wr){
		.wr_id = wc->wr_id,
		.sg_list = &sge,
		.num_sge = 1,
	};
	err = ibv_post_recv(rdma_qpair->cm_id->qp, &recv_wr, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: ibv_post_recv() to re-post slot, err: %d", err);
		return err;
	}

	if (qpair->on_capsule_recv) {
		qpair->on_capsule_recv(qpair, buf, NVME_CPL_CAPSULE_SIZE);
	}

	return 0;
}

static inline int
_process_completions(struct xnvme_be_nvmf_qpair *qpair, struct ibv_cq *cq,
		     xnvme_be_nvmf_ib_cmpl_fn handle_cmpl)
{
	struct ibv_wc wc;
	int count = 0;
	int err;

	while (1) {
		err = ibv_poll_cq(cq, 1, &wc);
		if (err < 0) {
			XNVME_DEBUG("FAILED: ibv_poll_cq() for cq, err: %d", err);
			return err;
		} else if (err == 0) {
			break;
		}

		if (wc.status != IBV_WC_SUCCESS) {
			XNVME_DEBUG("FAILED: Work completion error, status: %d, error=%s",
				    wc.status, ibv_wc_status_str(wc.status));
			return -EIO;
		}

		XNVME_DEBUG("INFO: Completion received, wr_id: %lu", wc.wr_id);
		err = handle_cmpl(qpair, &wc);
		if (err) {
			XNVME_DEBUG("FAILED: handle_cmpl(), err: %d", err);
			return err;
		}
		count++;
	}

	return count;
}

static inline int
_process_recv_completions(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);

	return _process_completions(qpair, rdma_qpair->cm_id->qp->recv_cq, _handle_recv_cmpl);
}

static inline int
_process_send_completions(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);

	return _process_completions(qpair, rdma_qpair->cm_id->qp->send_cq, _handle_send_cmpl);
}

static inline int
_progress_all_completion_queues(struct xnvme_be_nvmf_qpair *qpair)
{
	int err;

	err = _process_send_completions(qpair);
	if (err < 0) {
		XNVME_DEBUG("FAILED: _process_send_completions(), err: %d", err);
		return err;
	} else if (err > 0) {
		XNVME_DEBUG("INFO: Processed %d send completions", err);
	}

	err = _process_recv_completions(qpair);
	if (err < 0) {
		XNVME_DEBUG("FAILED: _process_recv_completions(), err: %d", err);
		return err;
	} else if (err > 0) {
		XNVME_DEBUG("INFO: Processed %d receive completions", err);
	}

	return err;
}

static int
_disconnect_rdma_qpair_sync(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	int err;

	if (qpair->state != XNVME_NVMF_QPAIR_STATE_CONNECTED &&
	    qpair->state != XNVME_NVMF_QPAIR_STATE_READY) {
		XNVME_DEBUG("INFO: QPair is not connected, skipping disconnect");
		return -ENOLINK;
	}

	err = rdma_disconnect(rdma_qpair->cm_id);
	if (err) {
		XNVME_DEBUG("FAILED: rdma_disconnect(), err: %d", err);
		return err;
	}
	XNVME_DEBUG("INFO: rdma_disconnect() successful, waiting for RDMA_CM_EVENT_DISCONNECTED");

	while (qpair->state != XNVME_NVMF_QPAIR_STATE_DISCONNECTED) {
		err = qpair->ctrlr->ops->process_events(qpair->ctrlr,
							XNVME_BE_NVMF_MAX_RDMACM_TIMEOUT_MS);
		if (err) {
			XNVME_DEBUG("FAILED: process_events(), err: %d", err);
			return err;
		}
	}

	return 0;
}

static int
_connect_rdma_qpair_sync(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr =
		TO_XNVME_NVMF_RDMA_CTRLR(rdma_qpair->base.ctrlr);
	struct rdma_addrinfo *ai = rdma_ctrlr->selected;
	int err;

	assert(qpair->state == XNVME_NVMF_QPAIR_STATE_INIT);

	err = rdma_create_id(rdma_ctrlr->event_channel, &rdma_qpair->cm_id, qpair,
			     ai->ai_port_space);
	if (err) {
		XNVME_DEBUG("FAILED: rdma_create_id(), err: %d", err);
		return err;
	}

	err = rdma_resolve_addr(rdma_qpair->cm_id, NULL, ai->ai_dst_addr,
				XNVME_BE_NVMF_MAX_RDMACM_TIMEOUT_MS);
	if (err) {
		XNVME_DEBUG("FAILED: rdma_resolve_addr(), err: %d", err);
		rdma_destroy_id(rdma_qpair->cm_id);
		return err;
	}
	XNVME_DEBUG(
		"INFO: rdma_resolve_addr() successful, waiting for RDMA_CM_EVENT_ADDR_RESOLVED");
	rdma_qpair->rdma_qp_state = XNVME_NVMF_RDMACM_STATE_RESOLVE_ADDRESS;

	while (qpair->state != XNVME_NVMF_QPAIR_STATE_CONNECTED) {
		err = xnvme_be_nvmf_ctrlr_process_events(qpair->ctrlr,
							 XNVME_BE_NVMF_MAX_RDMACM_TIMEOUT_MS);
		if (err) {
			XNVME_DEBUG("FAILED: process_events(), err: %d", err);
			return err;
		}

		if (qpair->state == XNVME_NVMF_QPAIR_STATE_ERROR) {
			XNVME_DEBUG("FAILED: QPair entered ERROR state during connection");
			_destroy_rdma_qpair(TO_XNVME_NVMF_RDMA_QPAIR(qpair));
			return -EIO;
		}
	}

	return 0;
}

int
_initialize_rdma_qpair(struct xnvme_be_nvmf_ctrlr *ctrlr,  size_t qsize,
		       struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);

	rdma_qpair->qp_init_attr.cap.max_send_wr = qsize;
	rdma_qpair->qp_init_attr.cap.max_recv_wr = qsize;
	rdma_qpair->qp_init_attr.cap.max_send_sge = 1;
	rdma_qpair->qp_init_attr.cap.max_recv_sge = 1;
	rdma_qpair->qp_init_attr.cap.max_inline_data = NVME_CMD_CAPSULE_SIZE;
	rdma_qpair->qp_init_attr.qp_type = IBV_QPT_RC;
	rdma_qpair->qp_init_attr.qp_context = (void *)&qpair;

	rdma_qpair->base.ops = &g_xnvme_be_nvmf_rdma_qpair_ops;
	rdma_qpair->base.on_capsule_recv = xnvme_be_nvmf_rdma_on_capsule_recv;
	rdma_qpair->base.on_send_cmpl = xnvme_be_nvmf_rdma_on_send_cmpl;
	rdma_qpair->base.on_state_change = xnvme_be_nvmf_rdma_on_state_change;

	return 0;
}

int
xnvme_be_nvmf_create_rdma_qpair(struct xnvme_be_nvmf_ctrlr *ctrlr, struct xnvme_be_nvmf_qpair_attr *attr,
				struct xnvme_be_nvmf_qpair **qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair;

	rdma_qpair = calloc(1, sizeof(*rdma_qpair));
	if (!rdma_qpair) {
		XNVME_DEBUG("FAILED: calloc(), err: %d", errno);
		return -ENOMEM;
	}

	_initialize_rdma_qpair(ctrlr, attr->qsize, &rdma_qpair->base);

	*qpair = &rdma_qpair->base;

	return 0;
}

static int
_rdma_send_capsule(struct xnvme_be_nvmf_qpair *qpair, void *buf, size_t len)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	struct ibv_sge sge = {
		.addr = (uintptr_t)buf,
		.length = len,
		.lkey = rdma_qpair->send_mr ? rdma_qpair->send_mr->lkey : 0,
	};
	struct ibv_send_wr send_wr = {
		/* Store buf pointer so on_send_cmpl can echo it back. */
		.wr_id = (uint64_t)(uintptr_t)buf,
		.sg_list = &sge,
		.num_sge = 1,
		.opcode = IBV_WR_SEND,
		.send_flags = IBV_SEND_SIGNALED,
	};
	struct ibv_send_wr *bad_wr;
	int err;

	if (len <= (size_t)rdma_qpair->qp_init_attr.cap.max_inline_data) {
		send_wr.send_flags |= IBV_SEND_INLINE;
		sge.lkey = 0;
	}

	err = ibv_post_send(rdma_qpair->cm_id->qp, &send_wr, &bad_wr);
	if (err) {
		XNVME_DEBUG("FAILED: ibv_post_send(), err: %d", err);
	}
	return err;
}

static int
_rdma_post_recv(struct xnvme_be_nvmf_qpair *qpair, void *buf, size_t len)
{
	/*
	 * The RDMA transport pre-posts all recv slots at connect time and
	 * re-posts them after each completion. No extra action needed here.
	 */
	(void)qpair;
	(void)buf;
	(void)len;
	return 0;
}

static int
_rdma_process_completions(struct xnvme_be_nvmf_qpair *qpair, int max_completions)
{
	int total = 0;
	int n;

	n = _process_send_completions(qpair);
	if (n < 0) {
		return n;
	}
	total += n;

	if (total >= max_completions) {
		return total;
	}

	n = _process_recv_completions(qpair);
	if (n < 0) {
		return n;
	}
	total += n;

	return total;
}

static int
_rdma_destroy(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	int err;

	err = _destroy_rdma_qpair(rdma_qpair);
	if (err) {
		XNVME_DEBUG("FAILED: _destroy_rdma_qpair(), err: %d", err);
		return err;
	}

	free(rdma_qpair);
	return 0;
}

static struct xnvme_be_nvmf_qpair_ops g_xnvme_be_nvmf_rdma_qpair_ops = {
	.connect = _connect_rdma_qpair_sync,
	.disconnect = _disconnect_rdma_qpair_sync,
	.destroy = _rdma_destroy,
	.send_capsule = _rdma_send_capsule,
	.post_recv = _rdma_post_recv,
	.process_completions = _rdma_process_completions,
};

#endif // XNVME_BE_NVMF_ENABLED
