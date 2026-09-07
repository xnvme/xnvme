// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <string.h>

#include <libxnvme.h>
#include <xnvme_be.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_rdma.h>

void
xnvme_be_nvmf_rdma_on_capsule_recv(struct xnvme_be_nvmf_qpair *qpair, void *buf, size_t len)
{
	struct xnvme_spec_cpl *cpl = buf;
	struct xnvme_be_nvmf_connect_response_cpl *connect_cpl =
		(struct xnvme_be_nvmf_connect_response_cpl *)cpl;
	struct xnvme_be_nvmf_req *req = NULL;
	struct xnvme_cmd_ctx *cmd_ctx = NULL;

	if (len < sizeof(*cpl)) {
		XNVME_DEBUG("FAILED: short capsule, len: %zu", len);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return;
	}

	req = xnvme_be_nvmf_req_get(qpair->req_pool, cpl->cid);
	if (!req) {
		XNVME_DEBUG("FAILED: Could not get request for wr_id index: %u", cpl->cid);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return;
	}

	req->cmpl_type = XNVME_BE_NVMF_REQ_CMPL_TYPE_RECV;
	req->status = 0;
	cmd_ctx = (struct xnvme_cmd_ctx *)req->context;

	// copy the completion into the command context
	memcpy(&cmd_ctx->cpl, cpl, sizeof(*cpl));

	_print_nvme_completion(cpl);

	switch (qpair->state) {
	case XNVME_NVMF_QPAIR_STATE_CONNECTED:
	case XNVME_NVMF_QPAIR_STATE_READY:
		break;

	default:
		XNVME_DEBUG("FAILED: capsule in unexpected state: %d", qpair->state);
		break;
	}
}

void
xnvme_be_nvmf_rdma_on_send_cmpl(struct xnvme_be_nvmf_qpair *qpair, void *buf, int status)
{
	(void)qpair;
	(void)buf;
	if (status)
		XNVME_DEBUG("FAILED: send completed with error: %d", status);
}

void
xnvme_be_nvmf_rdma_on_state_change(struct xnvme_be_nvmf_qpair *qpair,
				   enum xnvme_nvmf_qpair_state state, void *ctx)
{
	(void)ctx;

	switch (state) {
	case XNVME_NVMF_QPAIR_STATE_CONNECTED:
		break;
	case XNVME_NVMF_QPAIR_STATE_READY:
		// DONE
		break;
	case XNVME_NVMF_QPAIR_STATE_DISCONNECTED:
	case XNVME_NVMF_QPAIR_STATE_ERROR:
		break;

	default:
		break;
	}
}

struct xnvme_be_nvmf_transport g_xnvme_be_nvmf_rdma_transport = {
	.name = "rdma",
	.ops =
		{
			.create_ctrlr = xnvme_be_nvmf_create_rdma_controller,
		},
};
