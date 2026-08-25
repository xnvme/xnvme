// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#ifdef XNVME_BE_NVMF_ENABLED
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdatomic.h>

#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>

#include <xnvme_be_nvmf.h>

/**
 * Maximum number of attempts to probe for a device matching the provided URI.
 *
 * This is used to retry probing for a device in case of transient errors or
 * delays in the NVMe-oF subsystem.
 */
#define XNVME_BE_NVMF_MAX_PROBE_ATTEMPTS 3

#define FOR_EACH_NVMF_TRANSPORT(transport)                                          \
	for (struct xnvme_be_nvmf_transport **transport =                           \
		     (struct xnvme_be_nvmf_transport **)g_xnvme_be_nvmf_transports; \
	     *transport != NULL; ++transport)

atomic_int g_xnvme_be_nvmf_ctrlr_id_counter = ATOMIC_VAR_INIT(0);
extern struct xnvme_be_nvmf_transport g_xnvme_be_nvmf_rdma_transport;

struct xnvme_be_nvmf_transport *g_xnvme_be_nvmf_transports[] = {&g_xnvme_be_nvmf_rdma_transport,
								NULL};

static inline int
xnvme_be_nvmf_ctrlr_create(struct xnvme_be_nvmf_transport *transport,
			   struct xnvme_be_nvmf_ctrlr **ctrlr)
{
	return transport->ops.create_ctrlr(ctrlr);
}

static inline int
xnvme_be_nvmf_ctrlr_disconnect(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: NULL ctrlr");
		return -EINVAL;
	}

	if (ctrlr->ops && ctrlr->ops->disconnect) {
		return ctrlr->ops->disconnect(ctrlr);
	}

	XNVME_DEBUG("FAILED: No disconnect operation defined for controller");
	return -ENOSYS;
}

static inline int
xnvme_be_nvmf_ctrlr_destroy(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: NULL ctrlr");
		return -EINVAL;
	}

	if (ctrlr->ops && ctrlr->ops->destroy) {
		return ctrlr->ops->destroy(ctrlr);
	}

	free(ctrlr);

	return 0;
}

static inline int
xnvme_be_nvmf_ctrlr_connect(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *uri)
{
	if (!ctrlr || !uri)
		return -EINVAL;

	return ctrlr->ops->connect(ctrlr, uri);
}

static inline int
xnvme_be_nvmf_transport_probe(struct xnvme_be_nvmf_transport *transport, struct xnvme_dev *dev,
			      uint16_t ctrlr_id, struct xnvme_be_nvmf_ctrlr **ctrlr)
{
	struct xnvme_be_nvmf_ctrlr *tmp_ctrlr;
	int err;

	err = xnvme_be_nvmf_ctrlr_create(transport, &tmp_ctrlr);
	if (err) {
		XNVME_DEBUG("FAILED: transport->ops->create_ctrlr(), err: %d", err);
		return err;
	}

	/* TODO: Move all of this into ctrlr_create */
	tmp_ctrlr->ctrlr_id = ctrlr_id;
	tmp_ctrlr->cm_state = XNVME_NVMF_CTRLR_STATE_INIT;
	tmp_ctrlr->last_allocated_queue_id = XNVME_BE_NVMF_IO_QUEUE_ID_START;
	tmp_ctrlr->attached = 0;
	tmp_ctrlr->discovery_ctrlr = dev->opts.nsid == 0 ? 1 : 0;
	XNVME_DEBUG("INFO: ctrlr->discovery_ctrlr set to %d based on dev->opts.nsid=%u",
		    tmp_ctrlr->discovery_ctrlr, dev->opts.nsid);
	/* END-TODO */

	err = xnvme_be_nvmf_ctrlr_connect(tmp_ctrlr, dev->ident.uri);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_nvmf_ctrlr_connect(), err: %d", err);
		goto destroy_controller;
	}

	tmp_ctrlr->cm_state = XNVME_NVMF_CTRLR_STATE_CONNECTED;
	tmp_ctrlr->attached = 1;
	*ctrlr = tmp_ctrlr;

	return 0;

destroy_controller:
	xnvme_be_nvmf_ctrlr_destroy(tmp_ctrlr);
	return err;
}

/**
 * Initialize a new controller for the device URI.
 *
 * Inits the environment, if needed, and probes for a device matching the URI.
 */
static void *
xnvme_be_nvmf_ctrlr_init(struct xnvme_dev *dev)
{
	struct xnvme_be_nvmf_state *state = (void *)dev->be.state;
	struct xnvme_be_nvmf_ctrlr *ctrlr = NULL;
	int ctrlr_id;
	int err;

	XNVME_DEBUG("INFO: ctrlr_init() for NVMe-oF device: %s, ns=%u, discovery=%s",
		    dev->ident.uri, dev->ident.nsid, dev->ident.nsid == 0 ? "yes" : "no");

	if (state->ctrlr) {
		XNVME_DEBUG("INFO: Controller already initialized, reusing existing controller");
		return state->ctrlr;
	}

	ctrlr_id = atomic_fetch_add(&g_xnvme_be_nvmf_ctrlr_id_counter, 1);
	XNVME_DEBUG("INFO: Assigned controller ID: %d", ctrlr_id);

	// Probe the uri to check if the device is reachable and supports NVMe-oF.
	for (int i = 0; !ctrlr; ++i) {
		// If the maximum number of attempts is reached, return an error.
		if (XNVME_BE_NVMF_MAX_PROBE_ATTEMPTS == i) {
			XNVME_DEBUG("FAILED: max attempts exceeded");
			errno = ENXIO;
			goto free_ctrlr_id;
		}

		FOR_EACH_NVMF_TRANSPORT(transport)
		{
			XNVME_DEBUG("INFO: Attempting to probe transport: %s", (*transport)->name);

			err = xnvme_be_nvmf_transport_probe(*transport, dev, ctrlr_id, &ctrlr);
			if (!err) {
				XNVME_DEBUG("INFO: Successfully connected to transport: %s",
					    dev->ident.uri);
				XNVME_DEBUG("INFO: transport->probe() successful, device is "
					    "reachable and supports NVMe-oF");
				break;
			} else {
				XNVME_DEBUG("INFO: transport->probe() failed for transport: %s, "
					    "err: %d",
					    (*transport)->name, err);
			}
		}
	}

	if (!ctrlr) {
		XNVME_DEBUG("FAILED: No transport could connect to the device: %s",
			    dev->ident.uri);
		errno = ENXIO;
		goto free_ctrlr_id;
	}

	// queue ID 0 is reserved for the admin queue, so we start allocating from 1.
	ctrlr->last_allocated_queue_id = 0;

	XNVME_DEBUG("INFO: ctrlr_init() OK");
	return ctrlr;

free_ctrlr_id:
	// attempt to free the controller ID if we failed to allocate a controller.
	// If this fails, then we don't know what ID should be free, and we should leave it alone.
	// TODO: Implement as a bitmask or find something more comprehensive to reallocate
	// controller IDs
	atomic_compare_exchange_strong(&g_xnvme_be_nvmf_ctrlr_id_counter, &ctrlr_id, ctrlr_id - 1);
	return NULL;
}

static int
xnvme_be_nvmf_ctrlr_term(void *ctrlr)
{
	struct xnvme_be_nvmf_ctrlr *nvmf_ctrlr = (void *)ctrlr;
	int err;

	XNVME_DEBUG("INFO: ctrlr_term() for NVMe-oF controller");

	if (nvmf_ctrlr) {
		if (nvmf_ctrlr->cm_state == XNVME_NVMF_CTRLR_STATE_CONNECTED) {
			err = xnvme_be_nvmf_ctrlr_disconnect(nvmf_ctrlr);
			if (err) {
				XNVME_DEBUG("FAILED: xnvme_be_nvmf_ctrlr_disconnect(), err: %d",
					    err);
				return err;
			}
		}

		err = xnvme_be_nvmf_ctrlr_destroy(nvmf_ctrlr);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_be_nvmf_ctrlr_destroy(), err: %d", err);
			return err;
		}

		free(nvmf_ctrlr);
	}

	return 0;
}

/*
 * Device functions for NVMe-oF backend
 */
void
xnvme_be_nvmf_dev_close(struct xnvme_dev *dev)
{
	XNVME_DEBUG("INFO: dev_close() for NVMe-oF device: %s", dev->ident.uri);
}

int
xnvme_be_nvmf_dev_open(struct xnvme_dev *dev)
{
	XNVME_DEBUG("INFO: dev_open() for NVMe-oF device: %s", dev->ident.uri);

	dev->ident.dtype = XNVME_DEV_TYPE_NVMF;
	dev->ident.csi = XNVME_SPEC_CSI_NVM;

	if (dev->opts.nsid) {
		dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
		dev->ident.nsid = dev->opts.nsid;
	} else {
		dev->ident.dtype = XNVME_DEV_TYPE_NVME_CONTROLLER;
		dev->ident.nsid = 0;
	}

	return 0;
}
#endif

struct xnvme_be_dev g_xnvme_be_nvmf_dev = {
#ifdef XNVME_BE_NVMF_ENABLED
	.dev_open = xnvme_be_nvmf_dev_open,
	.dev_close = xnvme_be_nvmf_dev_close,
	.id = "nvmf",
	.ctrlr_init = xnvme_be_nvmf_ctrlr_init,
	.ctrlr_term = xnvme_be_nvmf_ctrlr_term,
#else
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
	.id = "nosys",
#endif
};
