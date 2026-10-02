// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <errno.h>

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>
#include <netinet/in.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_qpair.h>
#include <xnvme_be_nvmf_req.h>
#include <xnvme_be_nvmf_transport.h>
#include <xnvme_be_nvmf_debug.h>
#include <xnvme_be_nvmf_rdma.h>

/*
 * Top half: implements struct xnvme_be_nvmf_transport_ops.
 * Builds NVMe-oF-specific transport data and translates native rdma_cm/verbs
 * results into core state. Calls the bottom half
 * (xnvme_be_nvmf_rdma_{cm,verbs}.c) directly with native types, and calls up
 * into the core only through xnvme_be_nvmf_qpair_complete().
 */

#define NVMF_DEBUG_CATEGORY NVMF_DEBUG_CATEGORY_VERBS

#define NVMF_RDMACM_DEBUG(fmt, ...) _NVMF_DEBUG(NVMF_DEBUG_CATEGORY_RDMACM, fmt, ##__VA_ARGS__)
#define NVMF_RDMACM_ERROR(fmt, ...) _NVMF_ERROR(NVMF_DEBUG_CATEGORY_RDMACM, fmt, ##__VA_ARGS__)

#define NVMF_CTRL_DEBUG(fmt, ...) _NVMF_DEBUG(NVMF_DEBUG_CATEGORY_VERBS_CTRL, fmt, ##__VA_ARGS__)
#define NVMF_CTRL_ERROR(fmt, ...) _NVMF_ERROR(NVMF_DEBUG_CATEGORY_VERBS_CTRL, fmt, ##__VA_ARGS__)

#define NVMF_DATA_DEBUG(fmt, ...) _NVMF_DEBUG(NVMF_DEBUG_CATEGORY_VERBS_DATA, fmt, ##__VA_ARGS__)
#define NVMF_DATA_ERROR(fmt, ...) _NVMF_ERROR(NVMF_DEBUG_CATEGORY_VERBS_DATA, fmt, ##__VA_ARGS__)

static const struct xnvme_be_nvmf_transport_ops g_xnvme_be_nvmf_rdma_ops;

/*
 * ---------------------------------------------------------------------------
 * ctrlr ops
 * ---------------------------------------------------------------------------
 */

static inline int
_rdma_resolve_addrinfo(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *uri)
{
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr = TO_XNVME_NVMF_RDMA_CTRLR(ctrlr);
	char *cpy, *ip_addr = NULL, *port = NULL;
	int err;

	cpy = strdup(uri);
	if (!cpy) {
		NVMF_RDMACM_ERROR("FAILED: strdup(), err: %d", errno);
		return -ENOMEM;
	}

	// TODO: change for IPv6 support
	ip_addr = strtok(cpy, ":");
	port = strtok(NULL, ":");

	err = rdma_getaddrinfo(ip_addr, port, NULL, &rdma_ctrlr->res);
	if (err) {
		NVMF_RDMACM_ERROR("FAILED: rdma_getaddrinfo(), err: %d", err);
		goto failed_getaddrinfo;
	}
	NVMF_RDMACM_DEBUG("INFO: Successfully retrieved address for transport: IP: %s, Port: %s",
			  ip_addr, port);
	NVMF_RDMACM_DEBUG("INFO: Address family: %s",
			  rdma_ctrlr->res->ai_family == AF_INET ? "IPv4" : "IPv6");

	return 0;

failed_getaddrinfo:
	free(cpy);
	return err;
}

static int
_rdma_ctrlr_alloc(struct xnvme_be_nvmf_ctrlr **ctrlr)
{
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr;

	rdma_ctrlr = calloc(1, sizeof(*rdma_ctrlr));
	if (!rdma_ctrlr) {
		NVMF_RDMACM_ERROR("FAILED: calloc(), err: %d", errno);
		return -ENOMEM;
	}

	rdma_ctrlr->base.ops = &g_xnvme_be_nvmf_rdma_ops;
	*ctrlr = &rdma_ctrlr->base;

	return 0;
}

static void
_rdma_ctrlr_free(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr = TO_XNVME_NVMF_RDMA_CTRLR(ctrlr);

	free(rdma_ctrlr);
}

static int
_rdma_ctrlr_init(struct xnvme_be_nvmf_ctrlr *XNVME_UNUSED(ctrlr))
{
	/* RDMA has nothing to acquire here; the PD waits for the admin
	 * qpair's transport connect. */
	return 0;
}

static int
_rdma_ctrlr_connect(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *uri)
{
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr = TO_XNVME_NVMF_RDMA_CTRLR(ctrlr);
	int err;

	err = _rdma_resolve_addrinfo(ctrlr, uri);
	if (err) {
		NVMF_RDMACM_ERROR("FAILED: _rdma_resolve_addrinfo(), err: %d", err);
		return err;
	}

	rdma_ctrlr->selected = NULL;
	for (struct rdma_addrinfo *ai = rdma_ctrlr->res; ai != NULL; ai = ai->ai_next) {
		if (ai->ai_family != AF_INET) {
			NVMF_RDMACM_DEBUG("INFO: Skipping unsupported address family: %d",
					  ai->ai_family);
			continue;
		}

		rdma_ctrlr->selected = ai;
		err = xnvme_be_nvmf_qpair_connect(ctrlr->admin_qpair);
		if (err) {
			rdma_ctrlr->selected = NULL;
			continue;
		}
		break;
	}

	if (err) {
		NVMF_RDMACM_ERROR("FAILED: Could not connect to any suitable RDMA address");
		goto destroy_qp;
	}

	NVMF_RDMACM_DEBUG("INFO: Successfully connected admin queue to remote controller");

	return 0;

destroy_qp:
	ctrlr->attached = 0;
	ctrlr->ctrlr_state = XNVME_NVMF_CTRLR_STATE_ERROR;
	if (ctrlr->admin_qpair) {
		xnvme_be_nvmf_qpair_destroy(ctrlr->admin_qpair);
		ctrlr->admin_qpair = NULL;
	}
	return -ENODEV;
}

static int
_rdma_ctrlr_disconnect(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	int err;

	if (!ctrlr || !ctrlr->admin_qpair) {
		NVMF_RDMACM_DEBUG("INFO: No admin_qpair to disconnect");
		return 0;
	}

	if (ctrlr->attached) {
		err = xnvme_be_nvmf_qpair_disconnect(ctrlr->admin_qpair);
		if (err) {
			NVMF_RDMACM_ERROR("FAILED: xnvme_be_nvmf_disconnect_qpair(), err: %d",
					  err);
			return err;
		}

		xnvme_be_nvmf_qpair_destroy(ctrlr->admin_qpair);

		ctrlr->attached = 0;
	}

	// TODO: This requires proper handling.
	// xnvme_be_nvmf_destroy_qpair(rdma_ctrlr->base.sync_qpair);
	// free(rdma_ctrlr->base.sync_qpair);

	return 0;
}

static int
_rdma_ctrlr_teardown(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr = TO_XNVME_NVMF_RDMA_CTRLR(ctrlr);

	if (rdma_ctrlr->pd) {
		ibv_dealloc_pd(rdma_ctrlr->pd);
		rdma_ctrlr->pd = NULL;
	}

	if (rdma_ctrlr->res) {
		rdma_freeaddrinfo(rdma_ctrlr->res);
		rdma_ctrlr->res = NULL;
	}

	return 0;
}

/**
 * ctrlr_reg / ctrlr_dereg
 *
 * Thin wrapper around ibv_reg_mr()/ibv_dereg_mr() on the ctrlr's PD. The PD
 * is created lazily during the admin qpair's transport connect, so it must
 * exist by the time any caller registers memory.
 */
static int
_rdma_ctrlr_reg(struct xnvme_be_nvmf_ctrlr *ctrlr, void *buf, size_t nbytes, void **handle,
		uint32_t *key)
{
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr = TO_XNVME_NVMF_RDMA_CTRLR(ctrlr);
	struct ibv_mr *mr;

	if (!rdma_ctrlr->pd) {
		NVMF_RDMACM_ERROR("FAILED: no PD on controller");
		return -EINVAL;
	}

	mr = ibv_reg_mr(rdma_ctrlr->pd, buf, nbytes,
			IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
	if (!mr) {
		NVMF_RDMACM_ERROR("FAILED: ibv_reg_mr(), err: %d", errno);
		return -errno;
	}

	*handle = mr;
	*key = mr->rkey;

	return 0;
}

static int
_rdma_ctrlr_dereg(struct xnvme_be_nvmf_ctrlr *XNVME_UNUSED(ctrlr), void *handle)
{
	struct ibv_mr *mr = handle;
	int err;

	if (!mr) {
		return 0;
	}

	err = ibv_dereg_mr(mr);
	if (err) {
		NVMF_RDMACM_ERROR("FAILED: ibv_dereg_mr(), err: %d", err);
		return -err;
	}

	return 0;
}

/*
 * ---------------------------------------------------------------------------
 * qpair ops
 * ---------------------------------------------------------------------------
 */

static int
_rdma_qpair_alloc(struct xnvme_be_nvmf_ctrlr *XNVME_UNUSED(ctrlr),
		  struct xnvme_be_nvmf_qpair **qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair;

	rdma_qpair = calloc(1, sizeof(*rdma_qpair));
	if (!rdma_qpair) {
		NVMF_CTRL_ERROR("FAILED: calloc(), err: %d", errno);
		return -ENOMEM;
	}

	rdma_qpair->base.ops = &g_xnvme_be_nvmf_rdma_ops;
	*qpair = &rdma_qpair->base;

	return 0;
}

static void
_rdma_qpair_free(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);

	free(rdma_qpair);
}

static int
_rdma_qpair_init(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);

	rdma_qpair->event_channel = rdma_create_event_channel();
	if (!rdma_qpair->event_channel) {
		NVMF_CTRL_ERROR("FAILED: rdma_create_event_channel(), err: %d", errno);
		return -errno;
	}

	rdma_qpair->qp_init_attr.cap.max_send_wr = qpair->attr.qsize;
	rdma_qpair->qp_init_attr.cap.max_recv_wr = qpair->attr.qsize;
	rdma_qpair->qp_init_attr.cap.max_send_sge = 1;
	rdma_qpair->qp_init_attr.cap.max_recv_sge = 1;
	rdma_qpair->qp_init_attr.cap.max_inline_data = NVME_CMD_CAPSULE_SIZE;
	rdma_qpair->qp_init_attr.qp_type = IBV_QPT_RC;
	/* Not the same thing as the cm_id context that _handle_rdmacm_event()
	 * reads: qp_context is separate, unused verbs metadata that nothing
	 * reads. Still, point it at the real qpair instead of the address of
	 * a stack parameter. */
	rdma_qpair->qp_init_attr.qp_context = qpair;

	return 0;
}

static int
_rdma_qpair_teardown(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	int err = 0;

	if (!rdma_qpair->cm_id) {
		NVMF_CTRL_ERROR("INFO: No qpair to destroy");
		return 0;
	}

	if (rdma_qpair->send_mr) {
		err = ibv_dereg_mr(rdma_qpair->send_mr);
		if (err) {
			NVMF_CTRL_ERROR("FAILED: ibv_dereg_mr() for send_mr, err: %d", err);
			return err;
		}
	}

	if (rdma_qpair->recv_mr) {
		err = ibv_dereg_mr(rdma_qpair->recv_mr);
		if (err) {
			NVMF_CTRL_ERROR("FAILED: ibv_dereg_mr() for recv_mr, err: %d", err);
			return err;
		}
	}

	if (rdma_qpair->send_buffer) {
		free(rdma_qpair->send_buffer);
		rdma_qpair->send_buffer = NULL;
	}

	if (rdma_qpair->recv_buffer) {
		free(rdma_qpair->recv_buffer);
		rdma_qpair->recv_buffer = NULL;
	}

	if (rdma_qpair->cm_id) {
		rdma_destroy_qp(rdma_qpair->cm_id);
		err = rdma_destroy_id(rdma_qpair->cm_id);
		if (err) {
			NVMF_CTRL_ERROR("FAILED: rdma_destroy_id(), err: %d", err);
			return err;
		}

		rdma_qpair->cm_id = NULL;
	}

	if (rdma_qpair->send_cq) {
		err = ibv_destroy_cq(rdma_qpair->send_cq);
		if (err) {
			NVMF_CTRL_ERROR("FAILED: ibv_destroy_cq() for send_cq, err: %d", err);
			return err;
		}
		rdma_qpair->send_cq = NULL;
	}

	if (rdma_qpair->recv_cq) {
		err = ibv_destroy_cq(rdma_qpair->recv_cq);
		if (err) {
			NVMF_CTRL_ERROR("FAILED: ibv_destroy_cq() for recv_cq, err: %d", err);
			return err;
		}
		rdma_qpair->recv_cq = NULL;
	}

	if (rdma_qpair->event_channel) {
		rdma_destroy_event_channel(rdma_qpair->event_channel);
		rdma_qpair->event_channel = NULL;
	}

	return 0;
}

static int
_rdma_qpair_connect(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr =
		TO_XNVME_NVMF_RDMA_CTRLR(rdma_qpair->base.ctrlr);
	struct rdma_addrinfo *ai = rdma_ctrlr->selected;
	int err;

	assert(qpair->state == XNVME_NVMF_QPAIR_STATE_INIT);

	err = rdma_create_id(rdma_qpair->event_channel, &rdma_qpair->cm_id, qpair,
			     ai->ai_port_space);
	if (err) {
		NVMF_CTRL_ERROR("FAILED: rdma_create_id(), err: %d", err);
		return err;
	}

	err = rdma_resolve_addr(rdma_qpair->cm_id, NULL, ai->ai_dst_addr,
				XNVME_BE_NVMF_MAX_RDMACM_TIMEOUT_MS);
	if (err) {
		NVMF_CTRL_ERROR("FAILED: rdma_resolve_addr(), err: %d", err);
		rdma_destroy_id(rdma_qpair->cm_id);
		return err;
	}
	NVMF_CTRL_DEBUG(
		"INFO: rdma_resolve_addr() successful, waiting for RDMA_CM_EVENT_ADDR_RESOLVED");
	rdma_qpair->rdma_qp_state = XNVME_NVMF_RDMACM_STATE_RESOLVE_ADDRESS;

	while (qpair->state != XNVME_NVMF_QPAIR_STATE_CONNECTED) {
		err = _process_qpair_cm_events(qpair, XNVME_BE_NVMF_MAX_RDMACM_TIMEOUT_MS);
		if (err) {
			NVMF_CTRL_ERROR("FAILED: _process_qpair_cm_events(), err: %d", err);
			return err;
		}

		if (qpair->state == XNVME_NVMF_QPAIR_STATE_ERROR) {
			NVMF_CTRL_ERROR("FAILED: QPair entered ERROR state during connection");
			_rdma_qpair_teardown(qpair);
			return -EIO;
		}
	}

	NVMF_CTRL_DEBUG("INFO: QPair transport-connected successfully");

	return 0;
}

static int
_rdma_qpair_disconnect(struct xnvme_be_nvmf_qpair *qpair)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	int err;

	if (qpair->state != XNVME_NVMF_QPAIR_STATE_CONNECTED &&
	    qpair->state != XNVME_NVMF_QPAIR_STATE_READY) {
		NVMF_CTRL_DEBUG("INFO: QPair is not connected, skipping disconnect");
		return -ENOLINK;
	}

	err = rdma_disconnect(rdma_qpair->cm_id);
	if (err) {
		NVMF_CTRL_ERROR("FAILED: rdma_disconnect(), err: %d", err);
		return err;
	}
	NVMF_CTRL_DEBUG(
		"INFO: rdma_disconnect() successful, waiting for RDMA_CM_EVENT_DISCONNECTED");

	while (qpair->state != XNVME_NVMF_QPAIR_STATE_DISCONNECTED) {
		err = _process_qpair_cm_events(qpair, XNVME_BE_NVMF_MAX_RDMACM_TIMEOUT_MS);
		if (err) {
			NVMF_CTRL_ERROR("FAILED: _process_qpair_cm_events(), err: %d", err);
			return err;
		}
	}

	return 0;
}

static inline int
_rdma_send_cap_helper(struct xnvme_be_nvmf_rdma_qpair *rdma_qpair, struct ibv_send_wr *send_wr, struct ibv_sge *sge)
{
	struct ibv_send_wr *bad_wr = NULL;
	int err;

	struct xnvme_be_nvmf_wr_id wr_id = {.raw = send_wr->wr_id};
	NVMF_DATA_DEBUG("INFO: Hexdump of send buffer: addr=%p, len=%u, lkey=%u",
			(void *)sge->addr, sge->length, sge->lkey);
	_hexdump_range(NVMF_DEBUG_CATEGORY_VERBS_DATA, (void *)sge->addr, sge->length);
	NVMF_DATA_DEBUG("INFO: Sending capsule, wr_id.index: %u, wr_id.type: %u, len: %u",
			wr_id.index, wr_id.type, sge->length);

	err = ibv_post_send(rdma_qpair->cm_id->qp, send_wr, &bad_wr);
	if (err) {
		NVMF_DATA_ERROR("FAILED: ibv_post_send(), err: %d", err);
	}

	return err;
}

static inline int
_rdma_send_cap_inline(struct xnvme_be_nvmf_qpair *qpair, const void *buf, size_t len, uint16_t cid,
		      uint32_t XNVME_UNUSED(lkey))
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	struct xnvme_be_nvmf_wr_id wr_id = {0};
	struct ibv_sge sge = {
		.addr = (uintptr_t)buf,
		.length = len,
		.lkey = 0,
	};
	struct ibv_send_wr send_wr = {
		.sg_list = &sge,
		.num_sge = 1,
		.opcode = IBV_WR_SEND,
		.send_flags = IBV_SEND_SIGNALED | IBV_SEND_INLINE,
	};
	struct xnvme_spec_cmd *cmd;
	int err;
	int old_cid;

	wr_id.type = XNVME_BE_NVMF_WR_TYPE_SEND;
	wr_id.index = cid;

	send_wr.wr_id = wr_id.raw;
	// access the capsule to modify the command ID transferred to the controller
	cmd = (struct xnvme_spec_cmd *)buf;
	// When using inline, we use the user's buffer instead of the qpair memory buffer, so set
	// the CID to req->cid before sending it inline.
	old_cid = cmd->common.cid;
	cmd->common.cid = cid;

	err = _rdma_send_cap_helper(rdma_qpair, &send_wr, &sge);

	// when using INLINE, we use the user's buffer, so restore the old CID
	cmd->common.cid = old_cid;

	return err;
}

static inline int
_rdma_send_cap_eager(struct xnvme_be_nvmf_qpair *qpair, const void *buf, size_t len, uint16_t cid,
		     uint32_t lkey)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);
	struct xnvme_be_nvmf_wr_id wr_id = {0};
	struct ibv_sge sge = {
		.addr = (uintptr_t)(rdma_qpair->send_buffer + cid * qpair->attr.capsule_size),
		.length = len,
		.lkey = lkey,
	};
	struct ibv_send_wr send_wr = {
		.sg_list = &sge,
		.num_sge = 1,
		.opcode = IBV_WR_SEND,
		.send_flags = IBV_SEND_SIGNALED,
	};
	struct xnvme_spec_cmd *cmd;

	// construct work request ID and associate it with the send work request
	wr_id.type = XNVME_BE_NVMF_WR_TYPE_SEND;
	wr_id.index = cid;
	send_wr.wr_id = wr_id.raw;

	// Copy the capsule into qpair send buffer memory
	memcpy((void *)sge.addr, buf, len);

	// access the capsule to modify the command ID transferred to the controller
	cmd = (struct xnvme_spec_cmd *)sge.addr;
	cmd->common.cid = cid;

	return _rdma_send_cap_helper(rdma_qpair, &send_wr, &sge);
} 

static inline int
_rdma_send_cap(struct xnvme_be_nvmf_qpair *qpair, const void *buf, size_t len, uint16_t cid,
	       uint32_t lkey)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);

	if (len <= (size_t)rdma_qpair->qp_init_attr.cap.max_inline_data) {
		return _rdma_send_cap_inline(qpair, buf, len, cid, lkey);
	} else {
		return _rdma_send_cap_eager(qpair, buf, len, cid, lkey);
	}
}

static int
_rdma_qpair_submit(struct xnvme_be_nvmf_qpair *qpair, const void *capsule, size_t nbytes,
		   uint16_t cid)
{
	struct xnvme_be_nvmf_rdma_qpair *rdma_qpair = TO_XNVME_NVMF_RDMA_QPAIR(qpair);

	return _rdma_send_cap(qpair, capsule, nbytes, cid, rdma_qpair->send_mr->lkey);
}

/* upper-layer function to poke the RDMA qpair for completions */
static int
_rdma_qpair_poll(struct xnvme_be_nvmf_qpair *qpair, uint32_t max)
{
	int err = _process_send_completions(qpair, max);
	if (err < 0) {
		return err;
	}

	return _process_recv_completions(qpair, max);
}

static int
_rdma_cmd_io(struct xnvme_be_nvmf_qpair *XNVME_UNUSED(qpair),
	     struct xnvme_cmd_ctx *XNVME_UNUSED(ctx), void *XNVME_UNUSED(dbuf),
	     size_t XNVME_UNUSED(dbuf_nbytes), void *XNVME_UNUSED(mbuf),
	     size_t XNVME_UNUSED(mbuf_nbytes))
{
	return -ENOSYS;
}

static int
_rdma_cmd_iov(struct xnvme_be_nvmf_qpair *XNVME_UNUSED(qpair),
	      struct xnvme_cmd_ctx *XNVME_UNUSED(ctx), struct iovec *XNVME_UNUSED(dvec),
	      size_t XNVME_UNUSED(dvec_cnt), size_t XNVME_UNUSED(dvec_nbytes),
	      struct iovec *XNVME_UNUSED(mvec), size_t XNVME_UNUSED(mvec_cnt),
	      size_t XNVME_UNUSED(mvec_nbytes))
{
	return -ENOSYS;
}

/*
 * The only up-call the bottom half is allowed to reach. Folds what used
 * to be the on_capsule_recv callback: the length check,
 * the qpair-state check, and the completion delivery into the core.
 */
int
xnvme_be_nvmf_rdma_top_recv_complete(struct xnvme_be_nvmf_qpair *qpair, void *buf, size_t len)
{
	struct xnvme_spec_cpl *cpl = buf;
	int err;

	if (len < sizeof(*cpl)) {
		NVMF_DATA_ERROR("FAILED: short capsule, len: %zu", len);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return -EIO;
	}

	switch (qpair->state) {
	case XNVME_NVMF_QPAIR_STATE_CONNECTED:
	case XNVME_NVMF_QPAIR_STATE_READY:
		break;

	default:
		NVMF_DATA_ERROR("FAILED: capsule in unexpected state: %d", qpair->state);
		break;
	}

	err = xnvme_be_nvmf_qpair_complete(qpair, cpl);
	if (err) {
		NVMF_DATA_ERROR("FAILED: xnvme_be_nvmf_qpair_complete(), err: %d", err);
		qpair->state = XNVME_NVMF_QPAIR_STATE_ERROR;
		return err;
	}

	_print_nvme_completion(cpl);

	return 0;
}

static const struct xnvme_be_nvmf_transport_ops g_xnvme_be_nvmf_rdma_ops = {
	.ctrlr_alloc = _rdma_ctrlr_alloc,
	.ctrlr_free = _rdma_ctrlr_free,
	.ctrlr_init = _rdma_ctrlr_init,
	.ctrlr_teardown = _rdma_ctrlr_teardown,
	.ctrlr_connect = _rdma_ctrlr_connect,
	.ctrlr_disconnect = _rdma_ctrlr_disconnect,
	.ctrlr_reg = _rdma_ctrlr_reg,
	.ctrlr_dereg = _rdma_ctrlr_dereg,

	.qpair_alloc = _rdma_qpair_alloc,
	.qpair_free = _rdma_qpair_free,
	.qpair_init = _rdma_qpair_init,
	.qpair_teardown = _rdma_qpair_teardown,
	.qpair_connect = _rdma_qpair_connect,
	.qpair_disconnect = _rdma_qpair_disconnect,

	.qpair_submit = _rdma_qpair_submit,
	.qpair_poll = _rdma_qpair_poll,
	.cmd_io = _rdma_cmd_io,
	.cmd_iov = _rdma_cmd_iov,
};

struct xnvme_be_nvmf_transport g_xnvme_be_nvmf_rdma_transport = {
	.name = "rdma",
	.ops = &g_xnvme_be_nvmf_rdma_ops,
};
