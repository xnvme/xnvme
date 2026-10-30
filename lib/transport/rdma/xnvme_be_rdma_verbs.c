// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <errno.h>
#include <string.h>

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

	TRANSPORT_DEBUG("INFO: Work completion, status: %s, opcode: %s, wr_id index: %u, type: %u",
			ibv_wc_status_str(wc->status), _rdma_verbs_wc_opcode_str(wc->opcode),
			wr_id.index, wr_id.type);

	if (wc->status != IBV_WC_SUCCESS) {
		TRANSPORT_ERROR("FAILED: send WC error: %s", ibv_wc_status_str(wc->status));
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;

		req = xnvme_transport_req_get(qpair->req_pool, wr_id.index);
		if (req) {
			req->status = -EIO;
		}

		return -EIO;
	}

	// The response may be processed before its send completion, so the request can be gone
	return 0;
}

static inline int
_rdma_verbs_repost_recv(struct xnvme_be_rdma_qpair *rdma_qpair, uint64_t index)
{
	struct ibv_recv_wr recv_wr;
	struct ibv_recv_wr *bad_recv_wr;
	struct ibv_sge sge;
	struct xnvme_be_rdma_wr_id wr_id = {
		.index = index,
		.type = XNVME_BE_RDMA_WR_TYPE_RECV,
	};
	uint8_t *buf;
	int err;

	buf = (uint8_t *)rdma_qpair->recv_buffer + index * rdma_qpair->base.attr.completion_size;

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
		err = -err; // ibv_post_recv returns positive errno on failure, convert to negative
	}

	return err;
}

static int
_rdma_verbs_handle_recv(struct xnvme_transport_qpair *qpair, struct ibv_wc *wc)
{
	struct xnvme_be_rdma_qpair *rdma_qpair = TO_RDMA_QPAIR(qpair);
	struct xnvme_be_rdma_wr_id wr_id = {.raw = wc->wr_id};
	struct xnvme_spec_cpl cpl;
	uint8_t *buf;
	int err;

	TRANSPORT_DEBUG("INFO: Work completion, status: %s, opcode: %s, byte_len: %u, wr_id "
			"index: %u, type: %u",
			ibv_wc_status_str(wc->status), _rdma_verbs_wc_opcode_str(wc->opcode),
			wc->byte_len, wr_id.index, wr_id.type);

	if (wc->status != IBV_WC_SUCCESS) {
		TRANSPORT_ERROR("FAILED: recv WC error: %s", ibv_wc_status_str(wc->status));
		// TODO: The failing request cannot be identified, so mark the qpair as dead
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
		return -EIO;
	}

	if (wc->byte_len < sizeof(cpl)) {
		TRANSPORT_ERROR("FAILED: short capsule, byte_len: %u", wc->byte_len);
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
		return -EIO;
	}

	buf = (uint8_t *)rdma_qpair->recv_buffer + wr_id.index * qpair->attr.completion_size;

	TRANSPORT_DEBUG("INFO: Received buffer at index: %u", wr_id.index);
	xnvme_transport_hexdump_range(TRANSPORT_DEBUG_CATEGORY_VERBS_DATA, buf,
				      qpair->attr.completion_size);

	// Copy the completion out so the slot can be re-posted before it is delivered
	memcpy(&cpl, buf, sizeof(cpl));

	err = _rdma_verbs_repost_recv(rdma_qpair, wr_id.index);
	if (err) {
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
		return err;
	}

	err = _rdma_top_recv_complete(qpair, &cpl, wc->byte_len);
	if (err) {
		TRANSPORT_ERROR("FAILED: _rdma_top_recv_complete(), err: %d", err);
	}

	return err;
}

/*
 * Returns -errno on error, 0 on no completions, or the number of completions processed on success.
 * A @max_completions of 0 drains the queue. Every completion that was dequeued is handled, the
 * first handler error is returned once the batch is done.
 */
static inline int
_rdma_verbs_process_cq(struct xnvme_transport_qpair *qpair, struct ibv_cq *cq,
		       xnvme_be_rdma_ib_cmpl_fn handle_cmpl, int max_completions)
{
	struct ibv_wc wcs[XNVME_BE_RDMA_CQ_BATCH];
	int count = 0;
	int first_err = 0;

	while (!max_completions || count < max_completions) {
		int want = XNVME_BE_RDMA_CQ_BATCH;
		int n;

		if (max_completions && max_completions - count < want) {
			want = max_completions - count;
		}

		n = ibv_poll_cq(cq, want, wcs);
		if (n < 0) {
			TRANSPORT_ERROR("FAILED: ibv_poll_cq(), err: %d", n);
			qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
			return n;
		}

		for (int i = 0; i < n; ++i) {
			int err = handle_cmpl(qpair, &wcs[i]);
			if (err) {
				TRANSPORT_ERROR("FAILED: handle_cmpl(), err: %d", err);
				if (!first_err) {
					first_err = err;
				}
			}
			count++;
		}

		if (first_err || n < want) {
			break;
		}
	}

	return first_err ? first_err : count;
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
