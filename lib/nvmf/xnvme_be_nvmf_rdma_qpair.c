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

int
_initialize_rdma_qpair(struct xnvme_be_nvmf_ctrlr *ctrlr, uint16_t qid,
		       struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);

	rdma_qpair->qp_init_attr.cap.max_send_wr = 128 + XNVME_NVMF_QPAIR_ASYNC_SEND_RESERVE;
	rdma_qpair->qp_init_attr.cap.max_recv_wr = 128 + XNVME_NVMF_QPAIR_ASYNC_RECV_RESERVE;
	rdma_qpair->qp_init_attr.cap.max_send_sge = 1;
	rdma_qpair->qp_init_attr.cap.max_recv_sge = 1;
	rdma_qpair->qp_init_attr.cap.max_inline_data = NVME_CMD_CAPSULE_SIZE;
	rdma_qpair->qp_init_attr.qp_type = IBV_QPT_RC;
	rdma_qpair->qp_init_attr.qp_context = (void *)&qpair;

	rdma_qpair->base = (struct xnvme_be_nvmf_qpair){
		.ops = &g_xnvme_be_nvmf_rdma_qpair_ops,
		.ctrlr = ctrlr,
		.state = XNVME_NVMF_QPAIR_STATE_INIT,
	};

	return 0;
}

int
_create_rdma_qpair(struct xnvme_be_nvmf_ctrlr *ctrlr, uint16_t qid,
		   struct xnvme_be_nvmf_qpair **qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair;

	rdma_qpair = calloc(1, sizeof(*rdma_qpair));
	if (!rdma_qpair) {
		XNVME_DEBUG("FAILED: calloc(), err: %d", errno);
		return -ENOMEM;
	}

	_initialize_rdma_qpair(ctrlr, qid, &rdma_qpair->base);

	*qpair = &rdma_qpair->base;

	return 0;
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
	.destroy = _rdma_destroy,
};

#endif // XNVME_BE_NVMF_ENABLED
