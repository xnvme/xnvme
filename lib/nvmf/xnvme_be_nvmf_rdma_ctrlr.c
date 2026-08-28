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
#include <unistd.h>
#include <pthread.h>

#include <xnvme_dev.h>

#include <rdma/rdma_cma.h>
#include <netinet/in.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_rdma.h>

static struct xnvme_be_nvmf_ctrlr_ops g_xnvme_be_nvmf_rdma_ctrlr_ops;

static int
_process_cm_events(struct xnvme_be_nvmf_ctrlr *ctrlr, int timeout_ms)
{
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr = TO_XNVME_NVMF_RDMA_CTRLR(ctrlr);
	struct rdma_event_channel *event_channel = rdma_ctrlr->event_channel;
	struct rdma_cm_event *event;
	struct xnvme_timer timer;
	int err = 0;

	pthread_mutex_lock(&ctrlr->lock);

	xnvme_timer_start(&timer);
	do {
		err = rdma_get_cm_event(event_channel, &event);
		if (err) {
			if (xnvme_timer_elapsed_msecs(&timer) >= (double)timeout_ms) {
				XNVME_DEBUG("FAILED: rdma_get_cm_event() timed out");
				err = -ETIMEDOUT;
				goto unlock_ctrlr;
			}

			// TODO: replace with non-blocking poll to avoid busy-wait
			usleep(1000);
		}
	} while (err);

	err = _handle_rdmacm_event(event);
	if (err) {
		XNVME_DEBUG("FAILED: _handle_rdmacm_event(), err: %d", err);
	}

	err = rdma_ack_cm_event(event);
	if (err) {
		XNVME_DEBUG("FAILED: rdma_ack_cm_event(), err: %d", err);
		goto unlock_ctrlr;
	}

unlock_ctrlr:
	pthread_mutex_unlock(&ctrlr->lock);
	return err;
}

static inline int
_rdma_resolve_addrinfo(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *uri)
{
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr = TO_XNVME_NVMF_RDMA_CTRLR(ctrlr);
	char *cpy, *ip_addr = NULL, *port = NULL;
	int err;

	cpy = strdup(uri);
	if (!cpy) {
		XNVME_DEBUG("FAILED: strdup(), err: %d", errno);
		return -ENOMEM;
	}

	// TODO: change for IPv6 support
	ip_addr = strtok(cpy, ":");
	port = strtok(NULL, ":");

	err = rdma_getaddrinfo(ip_addr, port, NULL, &rdma_ctrlr->res);
	if (err) {
		XNVME_DEBUG("FAILED: rdma_getaddrinfo(), err: %d", err);
		goto failed_getaddrinfo;
	}
	XNVME_DEBUG("INFO: Successfully retrieved address for transport: IP: %s, Port: %s",
		    ip_addr, port);
	XNVME_DEBUG("INFO: Address family: %s",
		    rdma_ctrlr->res->ai_family == AF_INET ? "IPv4" : "IPv6");

	return 0;

failed_getaddrinfo:
	free(cpy);
	return err;
}

int
xnvme_be_nvmf_create_rdma_controller(struct xnvme_be_nvmf_ctrlr **ctrlr)
{
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr;
	struct xnvme_be_nvmf_qpair_attr admin_attr = {
		.qid = XNVME_BE_NVMF_ADMIN_QUEUE_ID,
		.qsize = 8,
		.capsule_size = NVME_CMD_CAPSULE_SIZE,
		.completion_size = NVME_CPL_CAPSULE_SIZE,
	};
	struct xnvme_be_nvmf_qpair_attr sync_attr = {
		.qid = XNVME_BE_NVMF_SYNC_QUEUE_ID,
		.qsize = 8,
	};
	int err;

	rdma_ctrlr = calloc(1, sizeof(*rdma_ctrlr));
	if (!rdma_ctrlr) {
		XNVME_DEBUG("FAILED: calloc(), err: %d", errno);
		return -ENOMEM;
	}
	pthread_mutex_init(&rdma_ctrlr->base.lock, NULL);
	rdma_ctrlr->base.ops = &g_xnvme_be_nvmf_rdma_ctrlr_ops;

	rdma_ctrlr->event_channel = rdma_create_event_channel();
	if (!rdma_ctrlr->event_channel) {
		XNVME_DEBUG("FAILED: rdma_create_event_channel(), err: %d", errno);
		err = -errno;
		goto free_ctrlr;
	}

	err = xnvme_be_nvmf_qpair_create(&rdma_ctrlr->base, &admin_attr,
					      &rdma_ctrlr->base.admin_qpair);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_nvmf_qpair_create() for admin_qpair, err: %d",
			    err);
		goto destroy_event_channel;
	}

	/*
	err = xnvme_be_nvmf_qpair_create(&rdma_ctrlr->base, &sync_attr,
					      &rdma_ctrlr->base.sync_qpair);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_nvmf_qpair_create() for sync_qpair, err: %d",
			    err);
		goto destroy_admin_qpair;
	}
	*/

	*ctrlr = &rdma_ctrlr->base;

	return 0;

destroy_admin_qpair:
	xnvme_be_nvmf_destroy_qpair(rdma_ctrlr->base.admin_qpair);
destroy_event_channel:
	rdma_destroy_event_channel(rdma_ctrlr->event_channel);
free_ctrlr:
	free(rdma_ctrlr);
	return err;
}

static inline int
_connect_rdma_controller(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *uri)
{
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr = TO_XNVME_NVMF_RDMA_CTRLR(ctrlr);
	int err;

	err = _rdma_resolve_addrinfo(ctrlr, uri);
	if (err) {
		XNVME_DEBUG("FAILED: _rdma_resolve_addrinfo(), err: %d", err);
		return err;
	}

	rdma_ctrlr->selected = NULL;
	for (struct rdma_addrinfo *ai = rdma_ctrlr->res; ai != NULL; ai = ai->ai_next) {
		if (ai->ai_family != AF_INET) {
			XNVME_DEBUG("INFO: Skipping unsupported address family: %d",
				    ai->ai_family);
			continue;
		}

		rdma_ctrlr->selected = ai;
		err = xnvme_be_nvmf_connect_qpair(ctrlr->admin_qpair);
		if (err) {
			rdma_ctrlr->selected = NULL;
			continue;
		}
		break;
	}

	if (!rdma_ctrlr->selected) {
		XNVME_DEBUG("FAILED: No suitable address found for transport");
		return -ENODEV;
	}

	return 0;
}

static int
_disconnect_rdma_controller(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	int err;

	if (!ctrlr || !ctrlr->admin_qpair) {
		XNVME_DEBUG("INFO: No admin_qpair to disconnect");
		return 0;
	}

	if (ctrlr->attached) {
		err = xnvme_be_nvmf_disconnect_qpair(ctrlr->admin_qpair);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_be_nvmf_disconnect_qpair(), err: %d", err);
			return err;
		}

		xnvme_be_nvmf_destroy_qpair(ctrlr->admin_qpair);

		ctrlr->attached = 0;
	}

	// TODO: This requires proper handling.
	//xnvme_be_nvmf_destroy_qpair(rdma_ctrlr->base.sync_qpair);
	//free(rdma_ctrlr->base.sync_qpair);

	return 0;
}

static int
_destroy_rdma_controller(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr = TO_XNVME_NVMF_RDMA_CTRLR(ctrlr);

	if (rdma_ctrlr->event_channel) {
		rdma_destroy_event_channel(rdma_ctrlr->event_channel);
		rdma_ctrlr->event_channel = NULL;
	}

	if (rdma_ctrlr->res) {
		rdma_freeaddrinfo(rdma_ctrlr->res);
		rdma_ctrlr->res = NULL;
	}

	return 0;
}

static struct xnvme_be_nvmf_ctrlr_ops g_xnvme_be_nvmf_rdma_ctrlr_ops = {
	.connect = _connect_rdma_controller,
	.disconnect = _disconnect_rdma_controller,
	.destroy = _destroy_rdma_controller,

	.create_qpair = xnvme_be_nvmf_create_rdma_qpair,
	.process_events = _process_cm_events,
};

#endif // XNVME_BE_NVMF_ENABLED
