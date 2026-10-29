// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <errno.h>

#include <rdma/rdma_cma.h>

#include <libxnvme.h>
#include <xnvme_be.h>

#include <xnvme_transport_debug.h>
#include <xnvme_transport_ctrlr.h>
#include <xnvme_transport_qpair.h>

#include <xnvme_be_rdma.h>
#include <xnvme_be_rdma_transport.h>

/*
 * Verbs completion queue mechanics. Pure CQ/work-completion decoding with no NVMe-oF knowledge.
 * `_rdma_verbs_handle_recv()` is the one place that calls up into the top half, through
 * `_rdma_top_recv_complete()`, to deliver a received completion.
 */

#define TRANSPORT_DEBUG_CATEGORY TRANSPORT_DEBUG_CATEGORY_VERBS_DATA

typedef int (*xnvme_be_rdma_ib_cmpl_fn)(struct xnvme_transport_qpair *qpair, struct ibv_wc *wc);

static inline const char *
_rdma_verbs_wc_opcode_str(enum ibv_wc_opcode opcode)
{
	switch (opcode) {
	case IBV_WC_SEND:
		return "SEND";
	case IBV_WC_RDMA_WRITE:
		return "RDMA_WRITE";
	case IBV_WC_RDMA_READ:
		return "RDMA_READ";
	case IBV_WC_COMP_SWAP:
		return "COMP_SWAP";
	case IBV_WC_FETCH_ADD:
		return "FETCH_ADD";
	case IBV_WC_BIND_MW:
		return "BIND_MW";
	case IBV_WC_RECV:
		return "RECV";
	case IBV_WC_RECV_RDMA_WITH_IMM:
		return "RECV_RDMA_WITH_IMM";
	default:
		return "UNKNOWN";
	}
}

static int
_rdma_verbs_handle_send(struct xnvme_transport_qpair *qpair, struct ibv_wc *wc)
{
	struct xnvme_transport_req *req;
	struct xnvme_be_rdma_wr_id wr_id = {.raw = wc->wr_id};
	int status = (wc->status == IBV_WC_SUCCESS) ? 0 : -EIO;

	TRANSPORT_DEBUG("INFO: Work completion, status: %s, opcode: %s, wr_id index: %u, type: %u",
			ibv_wc_status_str(wc->status), _rdma_verbs_wc_opcode_str(wc->opcode),
			wr_id.index, wr_id.type);

	req = xnvme_transport_req_get(qpair->req_pool, wr_id.index);
	if (!req) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_req_get() for wr_id index: %u",
				wr_id.index);
		return -EIO; // TODO: Find a different error code
	}

	req->status = wc->status;
	if (wc->status != IBV_WC_SUCCESS) {
		TRANSPORT_ERROR("FAILED: send WC error: %s", ibv_wc_status_str(wc->status));
	}

	return status;
}

static inline int
_rdma_verbs_repost_recv(struct xnvme_be_rdma_qpair *rdma_qpair, uint64_t index,
			struct ibv_wc *XNVME_UNUSED(wc))
{
	struct ibv_recv_wr recv_wr;
	struct ibv_recv_wr *bad_recv_wr;
	struct ibv_sge sge;
	struct xnvme_be_rdma_wr_id wr_id = {
		.index = index,
		.type = XNVME_BE_RDMA_WR_TYPE_RECV,
	};
	void *buf;
	int err;

	buf = rdma_qpair->recv_buffer + index * rdma_qpair->base.attr.completion_size;

	/* Re-post the slot before the callback so the pool never drains. */
	sge = (struct ibv_sge){
		.addr = (uintptr_t)buf,
		.length = rdma_qpair->base.attr.completion_size,
		.lkey = rdma_qpair->recv_mr->lkey,
	};
	recv_wr = (struct ibv_recv_wr){
		.wr_id = wr_id.raw,
		.sg_list = &sge,
		.num_sge = 1,
	};

	err = ibv_post_recv(rdma_qpair->cm_id->qp, &recv_wr, &bad_recv_wr);
	if (err) {
		TRANSPORT_ERROR("FAILED: ibv_post_recv() to re-post slot, err: %d", err);
		return err;
	}

	return err;
}

static int
_rdma_verbs_handle_recv(struct xnvme_transport_qpair *qpair, struct ibv_wc *wc)
{
	struct xnvme_be_rdma_qpair *rdma_qpair = TO_RDMA_QPAIR(qpair);
	struct xnvme_be_rdma_wr_id wr_id = {.raw = wc->wr_id};
	void *buf;
	int err;

	TRANSPORT_DEBUG("INFO: Work completion, status: %s, opcode: %s, byte_len: %u, wr_id "
			"index: %u, type: %u",
			ibv_wc_status_str(wc->status), _rdma_verbs_wc_opcode_str(wc->opcode),
			wc->byte_len, wr_id.index, wr_id.type);

	if (wc->status != IBV_WC_SUCCESS) {
		TRANSPORT_ERROR("FAILED: recv WC error: %s", ibv_wc_status_str(wc->status));
		qpair->state =
			XNVME_TRANSPORT_QPAIR_STATE_ERROR; // TODO: Cannot track which request
							   // caused the error, so ignore for now
							   // and mark the qpair as dead.
		return -EIO;
	}

	buf = rdma_qpair->recv_buffer + wr_id.index * qpair->attr.completion_size;

	TRANSPORT_DEBUG("INFO: Received buffer at index: %u", wr_id.index);
	xnvme_transport_hexdump_range(TRANSPORT_DEBUG_CATEGORY_VERBS_DATA, buf,
				      qpair->attr.completion_size);

	err = _rdma_top_recv_complete(qpair, buf, qpair->attr.completion_size);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_be_rdma_top_recv_complete(), err: %d", err);
		return err;
	}

	err = _rdma_verbs_repost_recv(rdma_qpair, wr_id.index, wc);
	if (err) {
		TRANSPORT_ERROR("FAILED: _rdma_verbs_repost_recv(), err: %d", err);
		return err;
	}

	return err;
}

static inline void
_rdma_verbs_handle_poll_error(struct xnvme_transport_qpair *qpair,
			      struct ibv_wc *XNVME_DEBUG_UNUSED(wc), int XNVME_DEBUG_UNUSED(err))
{
	TRANSPORT_ERROR("FAILED: ibv_poll_cq() for cq, err: %d", err);
	TRANSPORT_DEBUG("INFO: ibv_poll_cq() returned error, wc status: %s, opcode: %s, "
			"byte_len: %u, wr_id index: %lu, type: %u",
			ibv_wc_status_str(wc->status), _rdma_verbs_wc_opcode_str(wc->opcode),
			wc->byte_len, wc->wr_id, 0);

	qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
}

/*
 * Returns -errno on error, 0 on no completions, or the number of completions processed on success.
 * A @max_completions of 0 drains the queue.
 */
static inline int
_rdma_verbs_process_cq(struct xnvme_transport_qpair *qpair, struct ibv_cq *cq,
		       xnvme_be_rdma_ib_cmpl_fn handle_cmpl, int max_completions)
{
	struct ibv_wc wcs[XNVME_BE_RDMA_CQ_BATCH];
	int count = 0;

	while (!max_completions || count < max_completions) {
		int want = XNVME_BE_RDMA_CQ_BATCH;
		int n;

		if (max_completions && max_completions - count < want) {
			want = max_completions - count;
		}

		n = ibv_poll_cq(cq, want, wcs);
		if (n < 0) {
			_rdma_verbs_handle_poll_error(qpair, &wcs[0], n);
			break;
		}

		for (int i = 0; i < n; ++i) {
			int err = handle_cmpl(qpair, &wcs[i]);
			if (err) {
				TRANSPORT_ERROR("FAILED: handle_cmpl(), err: %d", err);
				return err;
			}
			count++;
		}

		if (n < want) {
			break;
		}
	}

	return count;
}

int
xnvme_be_rdma_verbs_process_recv_completions(struct xnvme_transport_qpair *qpair,
					     int max_completions)
{
	struct xnvme_be_rdma_qpair *rdma_qpair = TO_RDMA_QPAIR(qpair);

	return _rdma_verbs_process_cq(qpair, rdma_qpair->cm_id->qp->recv_cq,
				      _rdma_verbs_handle_recv, max_completions);
}

int
xnvme_be_rdma_verbs_process_send_completions(struct xnvme_transport_qpair *qpair,
					     int max_completions)
{
	struct xnvme_be_rdma_qpair *rdma_qpair = TO_RDMA_QPAIR(qpair);

	return _rdma_verbs_process_cq(qpair, rdma_qpair->cm_id->qp->send_cq,
				      _rdma_verbs_handle_send, max_completions);
}
