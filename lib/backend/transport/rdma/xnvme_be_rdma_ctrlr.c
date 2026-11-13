// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <errno.h>
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
#include <xnvme_be_rdma_ctrlr.h>
#include <xnvme_be_rdma_qpair.h>

/*
 * Top half, controller side: implements struct xnvme_transport_ctrlr_ops and the transport's
 * ctrlr_probe. Resolves the controller address, connects the admin qpair, and owns the PD
 * that memory registration uses. The qpair side lives in xnvme_be_rdma_qpair.c.
 */

#define TRANSPORT_DEBUG_CATEGORY TRANSPORT_DEBUG_CATEGORY_VERBS

#define TRANSPORT_RDMACM_DEBUG(fmt, ...) \
	_TRANSPORT_DEBUG(TRANSPORT_DEBUG_CATEGORY_RDMACM, fmt, ##__VA_ARGS__)
#define TRANSPORT_RDMACM_ERROR(fmt, ...) \
	_TRANSPORT_ERROR(TRANSPORT_DEBUG_CATEGORY_RDMACM, fmt, ##__VA_ARGS__)

static const struct xnvme_transport_ctrlr_ops g_xnvme_be_rdma_ctrlr_ops;

static inline int
_rdma_ctrlr_resolve_addrinfo(struct xnvme_transport_ctrlr *ctrlr, const char *uri)
{
	struct xnvme_be_rdma_ctrlr *rdma_ctrlr = TO_RDMA_CTRLR(ctrlr);
	char *cpy, *saveptr = NULL, *ip_addr = NULL, *port = NULL;
	int err;

	cpy = strdup(uri);
	if (!cpy) {
		TRANSPORT_RDMACM_ERROR("FAILED: strdup(), err: %d", errno);
		return -ENOMEM;
	}

	// TODO: change for IPv6 support
	ip_addr = strtok_r(cpy, ":", &saveptr);
	port = strtok_r(NULL, ":", &saveptr);
	if (!ip_addr || !port) {
		TRANSPORT_RDMACM_ERROR("FAILED: uri must be of the form <ip>:<port>, uri: %s",
				       uri);
		err = -EINVAL;
		goto failed_getaddrinfo;
	}

	err = rdma_getaddrinfo(ip_addr, port, NULL, &rdma_ctrlr->res);
	if (err) {
		err = -errno;
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

// A failed connect leaves the admin qpair unusable, so replace it before trying another address
static int
_rdma_ctrlr_reset_admin_qpair(struct xnvme_transport_ctrlr *ctrlr)
{
	struct xnvme_transport_qpair_attr attr = ctrlr->admin_qpair->attr;
	struct xnvme_dev *dev = ctrlr->admin_qpair->dev;
	int err;

	err = xnvme_transport_qpair_destroy(ctrlr->admin_qpair);
	if (err) {
		return err;
	}
	ctrlr->admin_qpair = NULL;

	return xnvme_transport_qpair_create(ctrlr, dev, &attr, &ctrlr->admin_qpair);
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

	// Stays an error until an address connects, also when no address is usable
	err = -ENXIO;
	rdma_ctrlr->selected = NULL;
	for (struct rdma_addrinfo *ai = rdma_ctrlr->res; ai != NULL; ai = ai->ai_next) {
		int err_reset;

		if (ai->ai_family != AF_INET) {
			TRANSPORT_RDMACM_DEBUG("INFO: Skipping unsupported address family: %d",
					       ai->ai_family);
			continue;
		}

		rdma_ctrlr->selected = ai;
		err = xnvme_transport_qpair_connect(ctrlr->admin_qpair);
		if (!err) {
			break;
		}
		TRANSPORT_RDMACM_ERROR("FAILED: xnvme_transport_qpair_connect(), err: %d", err);
		rdma_ctrlr->selected = NULL;

		err_reset = _rdma_ctrlr_reset_admin_qpair(ctrlr);
		if (err_reset) {
			TRANSPORT_RDMACM_ERROR("FAILED: _rdma_ctrlr_reset_admin_qpair(), err: %d",
					       err_reset);
			err = err_reset;
			break;
		}
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
	return err;
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
	int err;

	if (!rdma_ctrlr->pd) {
		TRANSPORT_RDMACM_ERROR("FAILED: no PD on controller");
		return -EINVAL;
	}

	mr = ibv_reg_mr(rdma_ctrlr->pd, buf, nbytes,
			IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
	if (!mr) {
		err = -errno;

		TRANSPORT_RDMACM_ERROR("FAILED: ibv_reg_mr(), err: %d", err);
		return err;
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
		return -err; // ibv_dereg_mr returns positive errno on failure, convert to negative
	}

	return 0;
}

static const struct xnvme_transport_ctrlr_ops g_xnvme_be_rdma_ctrlr_ops = {
	.ctrlr_free = _rdma_ctrlr_free,
	.ctrlr_teardown = _rdma_ctrlr_teardown,
	.ctrlr_connect = _rdma_ctrlr_connect,
	.ctrlr_disconnect = _rdma_ctrlr_disconnect,
	.ctrlr_reg = _rdma_ctrlr_reg,
	.ctrlr_dereg = _rdma_ctrlr_dereg,
	.ctrlr_qpair_alloc = xnvme_be_rdma_qpair_alloc,
};

struct xnvme_transport g_xnvme_be_rdma_transport = {
	.name = "rdma",
	.ctrlr_probe = _rdma_ctrlr_probe,
};
