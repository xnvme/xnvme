// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>

#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#include <xnvme_transport.h>
#include <xnvme_transport_debug.h>
#include <xnvme_transport_ctrlr.h>
#include <xnvme_transport_qpair.h>

#include <xnvme_be_rdma.h>
#include <xnvme_be_rdma_transport.h>

/*
 * Top half: implements struct xnvme_transport_ctrlr_ops and xnvme_transport_qpair_ops.
 * Builds NVMe-oF-specific transport data and translates native rdma_cm/verbs
 * results into core state. Calls the bottom half
 * (xnvme_be_rdma_{cm,verbs}.c) directly with native types, and calls up
 * into the core only through xnvme_transport_qpair_complete().
 */

#define TRANSPORT_DEBUG_CATEGORY TRANSPORT_DEBUG_CATEGORY_VERBS

#define TRANSPORT_RDMACM_DEBUG(fmt, ...) \
	_TRANSPORT_DEBUG(TRANSPORT_DEBUG_CATEGORY_RDMACM, fmt, ##__VA_ARGS__)
#define TRANSPORT_RDMACM_ERROR(fmt, ...) \
	_TRANSPORT_ERROR(TRANSPORT_DEBUG_CATEGORY_RDMACM, fmt, ##__VA_ARGS__)

#define TRANSPORT_CTRL_DEBUG(fmt, ...) \
	_TRANSPORT_DEBUG(TRANSPORT_DEBUG_CATEGORY_VERBS_CTRL, fmt, ##__VA_ARGS__)
#define TRANSPORT_CTRL_ERROR(fmt, ...) \
	_TRANSPORT_ERROR(TRANSPORT_DEBUG_CATEGORY_VERBS_CTRL, fmt, ##__VA_ARGS__)

#define TRANSPORT_DATA_DEBUG(fmt, ...) \
	_TRANSPORT_DEBUG(TRANSPORT_DEBUG_CATEGORY_VERBS_DATA, fmt, ##__VA_ARGS__)
#define TRANSPORT_DATA_ERROR(fmt, ...) \
	_TRANSPORT_ERROR(TRANSPORT_DEBUG_CATEGORY_VERBS_DATA, fmt, ##__VA_ARGS__)

static const struct xnvme_transport_ctrlr_ops g_xnvme_be_rdma_ctrlr_ops;
static const struct xnvme_transport_qpair_ops g_xnvme_be_rdma_qpair_ops;

/*
 * ---------------------------------------------------------------------------
 * ctrlr ops
 * ---------------------------------------------------------------------------
 */

static inline int
_rdma_ctrlr_resolve_addrinfo(struct xnvme_transport_ctrlr *ctrlr, const char *uri)
{
	struct xnvme_be_rdma_ctrlr *rdma_ctrlr = TO_RDMA_CTRLR(ctrlr);
	char *cpy, *ip_addr = NULL, *port = NULL;
	int err;

	cpy = strdup(uri);
	if (!cpy) {
		TRANSPORT_RDMACM_ERROR("FAILED: strdup(), err: %d", errno);
		return -ENOMEM;
	}

	// TODO: change for IPv6 support
	ip_addr = strtok(cpy, ":");
	port = strtok(NULL, ":");

	err = rdma_getaddrinfo(ip_addr, port, NULL, &rdma_ctrlr->res);
	if (err) {
		TRANSPORT_RDMACM_ERROR("FAILED: rdma_getaddrinfo(), err: %d", err);
		goto failed_getaddrinfo;
	}
	TRANSPORT_RDMACM_DEBUG(
		"INFO: Successfully retrieved address for transport: IP: %s, Port: %s", ip_addr,
		port);
	TRANSPORT_RDMACM_DEBUG("INFO: Address family: %s",
			       rdma_ctrlr->res->ai_family == AF_INET ? "IPv4" : "IPv6");

failed_getaddrinfo:
	free(cpy);
	return err;
}

static int
_rdma_ctrlr_probe(struct xnvme_transport_ctrlr **ctrlr)
{
	struct xnvme_be_rdma_ctrlr *rdma_ctrlr;

	rdma_ctrlr = calloc(1, sizeof(*rdma_ctrlr));
	if (!rdma_ctrlr) {
		TRANSPORT_RDMACM_ERROR("FAILED: calloc(), err: %d", errno);
		return -ENOMEM;
	}

	rdma_ctrlr->base.ops = &g_xnvme_be_rdma_ctrlr_ops;
	*ctrlr = &rdma_ctrlr->base;

	return 0;
}

static void
_rdma_ctrlr_free(struct xnvme_transport_ctrlr *ctrlr)
{
	struct xnvme_be_rdma_ctrlr *rdma_ctrlr = TO_RDMA_CTRLR(ctrlr);

	free(rdma_ctrlr);
}

static int
_rdma_ctrlr_connect(struct xnvme_transport_ctrlr *ctrlr, const char *uri)
{
	struct xnvme_be_rdma_ctrlr *rdma_ctrlr = TO_RDMA_CTRLR(ctrlr);
	int err;

	err = _rdma_ctrlr_resolve_addrinfo(ctrlr, uri);
	if (err) {
		TRANSPORT_RDMACM_ERROR("FAILED: _rdma_ctrlr_resolve_addrinfo(), err: %d", err);
		return err;
	}

	rdma_ctrlr->selected = NULL;
	for (struct rdma_addrinfo *ai = rdma_ctrlr->res; ai != NULL; ai = ai->ai_next) {
		if (ai->ai_family != AF_INET) {
			TRANSPORT_RDMACM_DEBUG("INFO: Skipping unsupported address family: %d",
					       ai->ai_family);
			continue;
		}

		rdma_ctrlr->selected = ai;
		err = xnvme_transport_qpair_connect(ctrlr->admin_qpair);
		if (err) {
			rdma_ctrlr->selected = NULL;
			continue;
		}
		break;
	}

	if (err) {
		TRANSPORT_RDMACM_ERROR("FAILED: Could not connect to any suitable RDMA address");
		goto destroy_qp;
	}

	TRANSPORT_RDMACM_DEBUG("INFO: Successfully connected admin queue to remote controller");

	return 0;

destroy_qp:
	ctrlr->attached = 0;
	ctrlr->ctrlr_state = XNVME_TRANSPORT_CTRLR_STATE_ERROR;
	if (ctrlr->admin_qpair) {
		xnvme_transport_qpair_destroy(ctrlr->admin_qpair);
		ctrlr->admin_qpair = NULL;
	}
	return -ENODEV;
}

static int
_rdma_ctrlr_disconnect(struct xnvme_transport_ctrlr *ctrlr)
{
	int err;

	if (!ctrlr || !ctrlr->admin_qpair) {
		TRANSPORT_RDMACM_DEBUG("INFO: No admin_qpair to disconnect");
		return 0;
	}

	if (ctrlr->attached) {
		err = xnvme_transport_qpair_disconnect(ctrlr->admin_qpair);
		if (err) {
			TRANSPORT_RDMACM_ERROR("FAILED: xnvme_be_rdma_disconnect_qpair(), err: %d",
					       err);
			return err;
		}

		xnvme_transport_qpair_destroy(ctrlr->admin_qpair);

		ctrlr->admin_qpair = NULL;
		ctrlr->attached = 0;
	}

	return 0;
}

static int
_rdma_ctrlr_teardown(struct xnvme_transport_ctrlr *ctrlr)
{
	struct xnvme_be_rdma_ctrlr *rdma_ctrlr = TO_RDMA_CTRLR(ctrlr);

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
_rdma_ctrlr_reg(struct xnvme_transport_ctrlr *ctrlr, void *buf, size_t nbytes, void **handle,
		uint32_t *key)
{
	struct xnvme_be_rdma_ctrlr *rdma_ctrlr = TO_RDMA_CTRLR(ctrlr);
	struct ibv_mr *mr;

	if (!rdma_ctrlr->pd) {
		TRANSPORT_RDMACM_ERROR("FAILED: no PD on controller");
		return -EINVAL;
	}

	mr = ibv_reg_mr(rdma_ctrlr->pd, buf, nbytes,
			IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
	if (!mr) {
		TRANSPORT_RDMACM_ERROR("FAILED: ibv_reg_mr(), err: %d", errno);
		return -errno;
	}

	*handle = mr;
	*key = mr->rkey;

	return 0;
}

static int
_rdma_ctrlr_dereg(struct xnvme_transport_ctrlr *XNVME_UNUSED(ctrlr), void *handle)
{
	struct ibv_mr *mr = handle;
	int err;

	if (!mr) {
		return 0;
	}

	err = ibv_dereg_mr(mr);
	if (err) {
		TRANSPORT_RDMACM_ERROR("FAILED: ibv_dereg_mr(), err: %d", err);
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
_rdma_ctrlr_qpair_alloc(struct xnvme_transport_ctrlr *XNVME_UNUSED(ctrlr),
			struct xnvme_transport_qpair **qpair)
{
	struct xnvme_be_rdma_qpair *rdma_qpair;

	rdma_qpair = calloc(1, sizeof(*rdma_qpair));
	if (!rdma_qpair) {
		TRANSPORT_CTRL_ERROR("FAILED: calloc(), err: %d", errno);
		return -ENOMEM;
	}

	rdma_qpair->base.ops = &g_xnvme_be_rdma_qpair_ops;
	*qpair = &rdma_qpair->base;

	return 0;
}

static void
_rdma_qpair_free(struct xnvme_transport_qpair *qpair)
{
	struct xnvme_be_rdma_qpair *rdma_qpair = TO_RDMA_QPAIR(qpair);

	free(rdma_qpair);
}

static int
_rdma_qpair_init(struct xnvme_transport_qpair *qpair)
{
	struct xnvme_be_rdma_qpair *rdma_qpair = TO_RDMA_QPAIR(qpair);
	int err;

	rdma_qpair->event_channel = rdma_create_event_channel();
	if (!rdma_qpair->event_channel) {
		TRANSPORT_CTRL_ERROR("FAILED: rdma_create_event_channel(), err: %d", errno);
		return -errno;
	}

	/* Non-blocking, so event waits can poll() the fd with a deadline */
	err = fcntl(rdma_qpair->event_channel->fd, F_SETFL,
		    fcntl(rdma_qpair->event_channel->fd, F_GETFL) | O_NONBLOCK);
	if (err) {
		err = -errno;
		TRANSPORT_CTRL_ERROR("FAILED: fcntl(O_NONBLOCK), err: %d", err);
		rdma_destroy_event_channel(rdma_qpair->event_channel);
		rdma_qpair->event_channel = NULL;
		return err;
	}

	rdma_qpair->qp_init_attr.cap.max_send_wr = qpair->attr.qsize;
	rdma_qpair->qp_init_attr.cap.max_recv_wr = qpair->attr.qsize;
	rdma_qpair->qp_init_attr.cap.max_send_sge = 1;
	rdma_qpair->qp_init_attr.cap.max_recv_sge = 1;
	rdma_qpair->qp_init_attr.cap.max_inline_data = NVME_CMD_CAPSULE_SIZE;
	rdma_qpair->qp_init_attr.qp_type = IBV_QPT_RC;
	/* Unused verbs metadata, point it at the qpair. */
	rdma_qpair->qp_init_attr.qp_context = qpair;

	return 0;
}

static void
_rdma_flush_cm_events(struct xnvme_be_rdma_qpair *rdma_qpair)
{
	struct rdma_cm_event *event;

	while (rdma_get_cm_event(rdma_qpair->event_channel, &event) == 0) {
		rdma_ack_cm_event(event);
	}
}

static int
_rdma_qpair_teardown(struct xnvme_transport_qpair *qpair)
{
	struct xnvme_be_rdma_qpair *rdma_qpair = TO_RDMA_QPAIR(qpair);
	int err = 0;

	if (!rdma_qpair->cm_id) {
		TRANSPORT_CTRL_ERROR("INFO: No qpair to destroy");
		if (rdma_qpair->event_channel) {
			rdma_destroy_event_channel(rdma_qpair->event_channel);
			rdma_qpair->event_channel = NULL;
		}
		return 0;
	}

	if (rdma_qpair->send_mr) {
		err = ibv_dereg_mr(rdma_qpair->send_mr);
		if (err) {
			TRANSPORT_CTRL_ERROR("FAILED: ibv_dereg_mr() for send_mr, err: %d", err);
			return err;
		}
		rdma_qpair->send_mr = NULL;
	}

	if (rdma_qpair->recv_mr) {
		err = ibv_dereg_mr(rdma_qpair->recv_mr);
		if (err) {
			TRANSPORT_CTRL_ERROR("FAILED: ibv_dereg_mr() for recv_mr, err: %d", err);
			return err;
		}
		rdma_qpair->recv_mr = NULL;
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

		_rdma_flush_cm_events(rdma_qpair);

		err = rdma_destroy_id(rdma_qpair->cm_id);
		if (err) {
			TRANSPORT_CTRL_ERROR("FAILED: rdma_destroy_id(), err: %d", err);
			return err;
		}

		rdma_qpair->cm_id = NULL;
	}

	if (rdma_qpair->send_cq) {
		err = ibv_destroy_cq(rdma_qpair->send_cq);
		if (err) {
			TRANSPORT_CTRL_ERROR("FAILED: ibv_destroy_cq() for send_cq, err: %d", err);
			return err;
		}
		rdma_qpair->send_cq = NULL;
	}

	if (rdma_qpair->recv_cq) {
		err = ibv_destroy_cq(rdma_qpair->recv_cq);
		if (err) {
			TRANSPORT_CTRL_ERROR("FAILED: ibv_destroy_cq() for recv_cq, err: %d", err);
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
_rdma_qpair_connect(struct xnvme_transport_qpair *qpair)
{
	int err;

	assert(qpair->state == XNVME_TRANSPORT_QPAIR_STATE_INIT);

	err = xnvme_be_rdma_cm_connect(qpair);
	if (err) {
		TRANSPORT_CTRL_ERROR("FAILED: xnvme_be_rdma_cm_connect(), err: %d", err);
		if (qpair->state == XNVME_TRANSPORT_QPAIR_STATE_ERROR) {
			_rdma_qpair_teardown(qpair);
		}
		return err;
	}

	TRANSPORT_CTRL_DEBUG("INFO: QPair transport-connected successfully");

	return 0;
}

static int
_rdma_qpair_disconnect(struct xnvme_transport_qpair *qpair)
{
	struct xnvme_be_rdma_qpair *rdma_qpair = TO_RDMA_QPAIR(qpair);
	int err;

	if (qpair->state != XNVME_TRANSPORT_QPAIR_STATE_CONNECTED &&
	    qpair->state != XNVME_TRANSPORT_QPAIR_STATE_READY) {
		TRANSPORT_CTRL_DEBUG("INFO: QPair is not connected, skipping disconnect");
		return -ENOLINK;
	}

	err = rdma_disconnect(rdma_qpair->cm_id);
	if (err) {
		TRANSPORT_CTRL_ERROR("FAILED: rdma_disconnect(), err: %d", err);
		return err;
	}
	TRANSPORT_CTRL_DEBUG(
		"INFO: rdma_disconnect() successful, waiting for RDMA_CM_EVENT_DISCONNECTED");

	err = xnvme_be_rdma_cm_wait_disconnected(qpair);
	if (err) {
		TRANSPORT_CTRL_ERROR("FAILED: xnvme_be_rdma_cm_wait_disconnected(), err: %d", err);
	}

	return err;
}

/* The core has already set the CID inside the capsule. */
static int
_rdma_qpair_submit(struct xnvme_transport_qpair *qpair, const void *capsule, size_t nbytes,
		   uint16_t cid)
{
	struct xnvme_be_rdma_qpair *rdma_qpair = TO_RDMA_QPAIR(qpair);
	struct xnvme_be_rdma_wr_id wr_id = {
		.index = cid,
		.type = XNVME_BE_RDMA_WR_TYPE_SEND,
	};
	struct ibv_sge sge = {
		.addr = (uintptr_t)capsule,
		.length = nbytes,
	};
	struct ibv_send_wr send_wr = {
		.wr_id = wr_id.raw,
		.sg_list = &sge,
		.num_sge = 1,
		.opcode = IBV_WR_SEND,
		.send_flags = IBV_SEND_SIGNALED,
	};
	struct ibv_send_wr *bad_wr = NULL;
	int err;

	if (nbytes <= (size_t)rdma_qpair->qp_init_attr.cap.max_inline_data) {
		send_wr.send_flags |= IBV_SEND_INLINE;
	} else {
		sge.addr = (uintptr_t)rdma_qpair->send_buffer + cid * qpair->attr.capsule_size;
		sge.lkey = rdma_qpair->send_mr->lkey;
		memcpy((void *)sge.addr, capsule, nbytes);
	}

	err = ibv_post_send(rdma_qpair->cm_id->qp, &send_wr, &bad_wr);
	if (err) {
		TRANSPORT_DATA_ERROR("FAILED: ibv_post_send(), err: %d", err);
	}

	return err;
}

/* upper-layer function to poke the RDMA qpair for completions */
static int
_rdma_qpair_poll(struct xnvme_transport_qpair *qpair, uint32_t max)
{
	int err = xnvme_be_rdma_verbs_process_send_completions(qpair, max);
	if (err < 0) {
		return err;
	}

	return xnvme_be_rdma_verbs_process_recv_completions(qpair, max);
}

/*
 * The only up-call the bottom half is allowed to reach: the length check, the qpair-state check,
 * and the completion delivery into the core.
 */
int
_rdma_top_recv_complete(struct xnvme_transport_qpair *qpair, void *buf, size_t len)
{
	struct xnvme_spec_cpl *cpl = buf;
	int err;

	if (len < sizeof(*cpl)) {
		TRANSPORT_DATA_ERROR("FAILED: short capsule, len: %zu", len);
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
		return -EIO;
	}

	switch (qpair->state) {
	case XNVME_TRANSPORT_QPAIR_STATE_CONNECTED:
	case XNVME_TRANSPORT_QPAIR_STATE_READY:
		break;

	default:
		TRANSPORT_FATAL("FAILED: capsule in unexpected state: %d", qpair->state);
		break;
	}

	err = xnvme_transport_qpair_complete(qpair, cpl);
	if (err) {
		TRANSPORT_DATA_ERROR("FAILED: xnvme_transport_qpair_complete(), err: %d", err);
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
		return err;
	}

	xnvme_transport_print_completion(cpl);

	return 0;
}

static const struct xnvme_transport_ctrlr_ops g_xnvme_be_rdma_ctrlr_ops = {
	.ctrlr_free = _rdma_ctrlr_free,
	.ctrlr_teardown = _rdma_ctrlr_teardown,
	.ctrlr_connect = _rdma_ctrlr_connect,
	.ctrlr_disconnect = _rdma_ctrlr_disconnect,
	.ctrlr_reg = _rdma_ctrlr_reg,
	.ctrlr_dereg = _rdma_ctrlr_dereg,
	.ctrlr_qpair_alloc = _rdma_ctrlr_qpair_alloc,
};

static const struct xnvme_transport_qpair_ops g_xnvme_be_rdma_qpair_ops = {
	.qpair_free = _rdma_qpair_free,
	.qpair_init = _rdma_qpair_init,
	.qpair_teardown = _rdma_qpair_teardown,
	.qpair_connect = _rdma_qpair_connect,
	.qpair_disconnect = _rdma_qpair_disconnect,

	.qpair_submit = _rdma_qpair_submit,
	.qpair_poll = _rdma_qpair_poll,
};

struct xnvme_transport g_xnvme_be_rdma_transport = {
	.name = "rdma",
	.ctrlr_probe = _rdma_ctrlr_probe,
};