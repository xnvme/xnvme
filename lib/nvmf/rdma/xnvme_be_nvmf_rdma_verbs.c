// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <libxnvme.h>
#include <xnvme_be.h>

#include <errno.h>

#include <rdma/rdma_cma.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_qpair.h>
#include <xnvme_be_nvmf_req.h>
#include <xnvme_be_nvmf_debug.h>
#include <xnvme_be_nvmf_rdma.h>

/*
 * Verbs completion queue mechanics. Pure
 * CQ/work-completion decoding, transport-native, with no NVMe-oF knowledge.
 * `_handle_recv_cmpl()` is the one place that reaches into the top half,
 * through `xnvme_be_nvmf_rdma_top_recv_complete()`, to deliver a received
 * capsule.
 */

#define NVMF_DEBUG_CATEGORY NVMF_DEBUG_CATEGORY_VERBS_DATA

typedef int (*xnvme_be_nvmf_ib_cmpl_fn)(struct xnvme_be_nvmf_qpair *qpair, struct ibv_wc *wc);

static inline const char *
_ibv_wc_opcode_str(enum ibv_wc_opcode opcode)
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
_handle_send_cmpl(struct xnvme_be_nvmf_qpair *qpair, struct ibv_wc *wc)
{
	struct xnvme_be_nvmf_req *req;
	struct xnvme_be_nvmf_wr_id wr_id = {.raw = wc->wr_id};
	int status = (wc->status == IBV_WC_SUCCESS) ? 0 : -EIO;

	NVMF_DEBUG("INFO: Work completion, status: %s, opcode: %s, wr_id index: %u, type: %u",
		   ibv_wc_status_str(wc->status), _ibv_wc_opcode_str(wc->opcode), wr_id.index,
		   wr_id.type);

	req = xnvme_be_nvmf_req_get(qpair->req_pool, wr_id.index);
	if (!req) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_req_get() for wr_id index: %u", wr_id.index);
		return -EIO; // TODO: Find a different error code
	}

	req->cmpl_type = XNVME_BE_NVMF_REQ_CMPL_TYPE_SEND;
	req->status = wc->status;
	if (wc->status != IBV_WC_SUCCESS) {
		NVMF_ERROR("FAILED: send WC error: %s", ibv_wc_status_str(wc->status));
	}

	return status;
}

static inline int
_repost_recv_buffer(struct xnvme_be_nvmf_rdma_qpair *rdma_qpair, uint64_t index,
		    struct ibv_wc *XNVME_UNUSED(wc))
{
	struct ibv_recv_wr recv_wr;
	struct ibv_recv_wr *bad_recv_wr;
	struct ibv_sge sge;
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
		.wr_id = index,
		.sg_list = &sge,
		.num_sge = 1,
	};

	err = ibv_post_recv(rdma_qpair->cm_id->qp, &recv_wr, &bad_recv_wr);
	if (err) {
		NVMF_ERROR("FAILED: ibv_post_recv() to re-post slot, err: %d", err);
		return err;
	}

	return err;
}

static int
_handle_recv_cmpl(struct xnvme_be_nvmf_qpair *qpair, struct ibv_wc *wc)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	struct xnvme_be_nvmf_wr_id wr_id = {.raw = wc->wr_id};
	void *buf;
	int err;

	NVMF_DEBUG("INFO: Work completion, status: %s, opcode: %s, byte_len: %u, wr_id "
		   "index: %u, type: %u",
		   ibv_wc_status_str(wc->status), _ibv_wc_opcode_str(wc->opcode), wc->byte_len,
		   wr_id.index, wr_id.type);

	if (wc->status != IBV_WC_SUCCESS) {
		NVMF_ERROR("FAILED: recv WC error: %s", ibv_wc_status_str(wc->status));
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR; // TODO: Cannot track which request
							     // caused the error, so ignore for now
							     // and mark the qpair as dead.
		return -EIO;
	}

	buf = rdma_qpair->recv_buffer + wr_id.index * qpair->attr.completion_size;

	NVMF_DEBUG("INFO: Received buffer at index: %u", wr_id.index);
	_hexdump_range(NVMF_DEBUG_CATEGORY_VERBS_DATA, buf, qpair->attr.completion_size);

	err = xnvme_be_nvmf_rdma_top_recv_complete(qpair, buf, qpair->attr.completion_size);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_rdma_top_recv_complete(), err: %d", err);
		return err;
	}

	err = _repost_recv_buffer(rdma_qpair, wr_id.index, wc);
	if (err) {
		NVMF_ERROR("FAILED: _repost_recv_buffer(), err: %d", err);
		return err;
	}

	return err;
}

static inline void
_handle_ibv_poll_error(struct xnvme_be_nvmf_qpair *qpair, struct ibv_wc *wc, int err)
{
	NVMF_ERROR("FAILED: ibv_poll_cq() for cq, err: %d", err);
	NVMF_DEBUG("INFO: ibv_poll_cq() returned error, wc status: %s, opcode: %s, "
		   "byte_len: %u, wr_id index: %lu, type: %u",
		   ibv_wc_status_str(wc->status), _ibv_wc_opcode_str(wc->opcode), wc->byte_len,
		   wc->wr_id, 0);
	qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
}

/*
 * Returns -errno on error, 0 on no completions, or the number of completions processed on success.
 */
static inline int
_process_completions(struct xnvme_be_nvmf_qpair *qpair, struct ibv_cq *cq,
		     xnvme_be_nvmf_ib_cmpl_fn handle_cmpl, int max_completions)
{
	struct ibv_wc wc;
	int count = 0;
	int err;

	while (count < max_completions || !max_completions) {
		err = ibv_poll_cq(cq, 1, &wc);
		if (err < 0) {
			_handle_ibv_poll_error(qpair, &wc, err);
			break;
		}

		if (err == 0) {
			break;
		}

		err = handle_cmpl(qpair, &wc);
		if (err) {
			NVMF_ERROR("FAILED: handle_cmpl(), err: %d", err);
			return -err;
		}

		count++;
	}

	return count;
}

int
_process_recv_completions(struct xnvme_be_nvmf_qpair *qpair, int max_completions)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);

	return _process_completions(qpair, rdma_qpair->cm_id->qp->recv_cq, _handle_recv_cmpl,
				    max_completions);
}

int
_process_send_completions(struct xnvme_be_nvmf_qpair *qpair, int max_completions)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);

	return _process_completions(qpair, rdma_qpair->cm_id->qp->send_cq, _handle_send_cmpl,
				    max_completions);
}

static inline int
_progress_all_completion_queues(struct xnvme_be_nvmf_qpair *qpair)
{
	int err;

	err = _process_send_completions(qpair, 0);
	if (err < 0) {
		NVMF_ERROR("FAILED: _process_send_completions(), err: %d", err);
		return err;
	} else if (err > 0) {
		NVMF_DEBUG("INFO: Processed %d send completions", err);
	}

	err = _process_recv_completions(qpair, 0);
	if (err < 0) {
		NVMF_ERROR("FAILED: _process_recv_completions(), err: %d", err);
		return err;
	} else if (err > 0) {
		NVMF_DEBUG("INFO: Processed %d receive completions", err);
	}

	return err;
}
