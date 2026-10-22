// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <stdatomic.h>
#include <errno.h>

#include <xnvme_dev.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_debug.h>
#include <xnvme_be_nvmf_fabric.h>
#include <xnvme_be_nvmf_qpair.h>
#include <xnvme_be_nvmf_subsys.h>
#include <xnvme_be_nvmf_transport.h>

#include <xnvme_be_nvmf_ctrlr.h>

#define NVMF_DEBUG_CATEGORY NVMF_DEBUG_CATEGORY_CORE_CTRLR

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

extern struct xnvme_be_nvmf_transport g_xnvme_be_nvmf_rdma_transport;

struct xnvme_be_nvmf_transport *g_xnvme_be_nvmf_transports[] = {&g_xnvme_be_nvmf_rdma_transport,
								NULL};

int
xnvme_be_nvmf_ctrlr_connect(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *uri)
{
	int err;
	if (!ctrlr || !uri)
		return -EINVAL;

	err = ctrlr->ops->ctrlr_connect(ctrlr, uri);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_connect(), err: %d", err);
		ctrlr->ctrlr_state = XNVME_NVMF_CTRLR_STATE_ERROR;
		return err;
	}

	ctrlr->ctrlr_state = XNVME_NVMF_CTRLR_STATE_CONNECTED;
	ctrlr->attached = 1;

	return err;
}

int
xnvme_be_nvmf_ctrlr_disconnect(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	return ctrlr->ops->ctrlr_disconnect(ctrlr);
}

int
xnvme_be_nvmf_ctrlr_create(struct xnvme_be_nvmf_transport *transport,
			   struct xnvme_be_nvmf_ctrlr_attr *attr,
			   struct xnvme_be_nvmf_ctrlr **ctrlr)
{
	struct xnvme_be_nvmf_ctrlr *tmp = NULL;
	struct xnvme_be_nvmf_qpair_attr default_qpair_attr = XNVME_BE_NVMF_ADMIN_QPAIR_ATTR;
	int err;
	
	err = transport->ops->ctrlr_alloc(&tmp);
	if (err) {
		NVMF_ERROR("FAILED: transport->ops->ctrlr_alloc(), err: %d", err);
		return err;
	}

	tmp->ctrlr_state = XNVME_NVMF_CTRLR_STATE_INIT;
	tmp->last_allocated_queue_id = XNVME_BE_NVMF_IO_QUEUE_ID_START;
	tmp->attached = 0;

	err = xnvme_be_nvmf_qpair_create(tmp, attr->dev, &default_qpair_attr, &tmp->admin_qpair);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_create() for admin_qpair, err: %d", err);
		xnvme_be_nvmf_ctrlr_destroy(tmp);;
		return err;
	}

	*ctrlr = tmp;
	return err;
}

int
xnvme_be_nvmf_ctrlr_destroy(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	int err;

	if (!ctrlr) {
		NVMF_ERROR("FAILED: NULL ctrlr");
		return -EINVAL;
	}

	err = ctrlr->ops->ctrlr_teardown(ctrlr);
	if (err) {
		NVMF_ERROR("FAILED: ctrlr->ops->ctrlr_teardown(), err: %d", err);
	}

	/*
	 * Free unconditionally, even when teardown fails, so a failed
	 * teardown does not leak the ctrlr.
	 */
	ctrlr->ops->ctrlr_free(ctrlr);

	return err;
}

static inline int
_nvmf_ctrlr_probe_transport(struct xnvme_be_nvmf_transport *transport,
			    struct xnvme_be_nvmf_ctrlr_attr *attr,
			    struct xnvme_be_nvmf_ctrlr **ctrlr)
{
	struct xnvme_be_nvmf_ctrlr *tmp_ctrlr;
	int err;

	err = xnvme_be_nvmf_ctrlr_create(transport, attr, &tmp_ctrlr);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_create(), err: %d", err);
		return err;
	}

	err = xnvme_be_nvmf_ctrlr_connect(tmp_ctrlr, attr->dev->ident.uri);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_connect(), err: %d", err);
		goto destroy_controller;
	}

	*ctrlr = tmp_ctrlr;

	return 0;

destroy_controller:
	xnvme_be_nvmf_ctrlr_destroy(tmp_ctrlr);
	return err;
}

static inline void
_nvmf_ctrlr_dump(struct xnvme_be_nvmf_ctrlr *XNVME_DEBUG_UNUSED(ctrlr))
{
	NVMF_DEBUG("INFO: ctrlr: \n"
		   "\tctrlr_id: %d\n"
		   "\tctrlr_state: %d\n"
		   "\tattached: %d",
		   ctrlr->admin_qpair->attr.cntlid, ctrlr->ctrlr_state, ctrlr->attached);
}

/**
 * Probes every registered transport for @dev->ident.uri.
 *
 * Runs up to XNVME_BE_NVMF_MAX_PROBE_ATTEMPTS rounds, each walking every
 * registered transport. On success `*ctrlr` is a connected controller.
 */
int
xnvme_be_nvmf_ctrlr_probe(struct xnvme_dev *dev, struct xnvme_be_nvmf_ctrlr **ctrlr)
{
	struct xnvme_be_nvmf_ctrlr *tmp_ctrlr = NULL;
	struct xnvme_be_nvmf_ctrlr_attr attr = {
		.dev = dev,
	};
	int err = -ENXIO;

	for (int i = 0; !tmp_ctrlr; ++i) {
		// If the maximum number of attempts is reached, return an error.
		if (XNVME_BE_NVMF_MAX_PROBE_ATTEMPTS == i) {
			NVMF_ERROR("FAILED: max attempts exceeded");
			return -ENXIO;
		}

		FOR_EACH_NVMF_TRANSPORT(transport)
		{
			NVMF_INFO("INFO: Attempting to probe transport: %s", (*transport)->name);

			err = _nvmf_ctrlr_probe_transport(*transport, &attr, &tmp_ctrlr);
			if (!err) {
				NVMF_INFO("INFO: Successfully connected to transport: %s",
					  dev->ident.uri);
				NVMF_INFO("INFO: transport->probe() successful, device is "
					  "reachable and supports NVMe-oF");
				break;
			} else {
				NVMF_INFO("INFO: transport->probe() failed for transport: %s, "
					  "err: %d",
					  (*transport)->name, err);
			}
		}
	}

	*ctrlr = tmp_ctrlr;
	return 0;
}

/**
 * Sets CC.EN and waits on CSTS.RDY, through Fabrics Property Get and Set on
 * the admin qpair. Forwards to `xnvme_be_nvmf_fabric_enable()` (fabric.c).
 */
int
xnvme_be_nvmf_ctrlr_enable(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	if (!ctrlr || !ctrlr->admin_qpair) {
		NVMF_ERROR("FAILED: Invalid ctrlr or missing admin_qpair");
		return -EINVAL;
	}

	return xnvme_be_nvmf_fabric_enable(ctrlr, ctrlr->admin_qpair, true);
}

int
xnvme_be_nvmf_ctrlr_reg(struct xnvme_be_nvmf_ctrlr *ctrlr, void *buf, size_t nbytes, void **handle,
			uint32_t *key)
{
	if (!ctrlr || !ctrlr->ops || !ctrlr->ops->ctrlr_reg) {
		NVMF_ERROR("FAILED: No ctrlr_reg operation defined for controller");
		return -ENOSYS;
	}

	return ctrlr->ops->ctrlr_reg(ctrlr, buf, nbytes, handle, key);
}

int
xnvme_be_nvmf_ctrlr_dereg(struct xnvme_be_nvmf_ctrlr *ctrlr, void *handle)
{
	if (!ctrlr || !ctrlr->ops || !ctrlr->ops->ctrlr_dereg) {
		NVMF_ERROR("FAILED: No ctrlr_dereg operation defined for controller");
		return -ENOSYS;
	}

	return ctrlr->ops->ctrlr_dereg(ctrlr, handle);
}

/**
 * Glue-facing entry point wired as `xnvme_be_dev.ctrlr_init`.
 *
 * Probes for a controller matching @dev->ident.uri and enables it once. A
 * cref hit is handled by the caller (the platform inserts the returned
 * pointer as a cref before calling `dev_open`).
 */
void *
xnvme_be_nvmf_dev_ctrlr_init(struct xnvme_dev *dev)
{
	struct xnvme_be_nvmf_state *state = TO_NVMF_DEV_STATE(dev);
	struct xnvme_be_nvmf_ctrlr *ctrlr = NULL;
	int err;

	NVMF_INFO("INFO: dev_ctrlr_init() for NVMe-oF device: %s, ns=%u, discovery=%s",
		  dev->ident.uri, dev->ident.nsid, dev->ident.nsid == 0 ? "yes" : "no");

	if (state->ctrlr) {
		NVMF_INFO("INFO: Controller already initialized, reusing existing controller");
		return state->ctrlr;
	}

	err = xnvme_be_nvmf_ctrlr_probe(dev, &ctrlr);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_probe(), err: %d", err);
		errno = -err;
		return NULL;
	}

	ctrlr->last_assigned_discovery_id = 0;

	_nvmf_ctrlr_dump(ctrlr);

	err = xnvme_be_nvmf_ctrlr_enable(ctrlr);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_enable(), err: %d", err);
		xnvme_be_nvmf_ctrlr_disconnect(ctrlr);
		xnvme_be_nvmf_ctrlr_destroy(ctrlr);
		errno = -err;
		return NULL;
	}

	NVMF_INFO("INFO: dev_ctrlr_init() OK");
	return ctrlr;
}

int
xnvme_be_nvmf_dev_ctrlr_term(void *ctrlr)
{
	struct xnvme_be_nvmf_ctrlr *nvmf_ctrlr = TO_NVMF_CTRLR(ctrlr);
	int err;

	NVMF_INFO("INFO: dev_ctrlr_term() for NVMe-oF controller");

	if (nvmf_ctrlr) {
		if (nvmf_ctrlr->ctrlr_state == XNVME_NVMF_CTRLR_STATE_CONNECTED) {
			err = xnvme_be_nvmf_ctrlr_disconnect(nvmf_ctrlr);
			if (err) {
				NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_disconnect(), err: %d",
					   err);
				return err;
			}
		}

		err = xnvme_be_nvmf_ctrlr_destroy(nvmf_ctrlr);
		if (err) {
			NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_destroy(), err: %d", err);
			return err;
		}
	}

	return 0;
}

struct xnvme_be_nvmf_subsys *
xnvme_be_nvmf_ctrlr_find_first_subsys(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *subnqn)
{
	struct xnvme_be_nvmf_subsys *subsys;

	SLIST_FOREACH(subsys, &ctrlr->subsystems, entry)
	{
		if (strcmp(subsys->subnqn, subnqn) == 0) {
			return subsys;
		}
	}

	return NULL;
}

struct xnvme_be_nvmf_subsys *
xnvme_be_nvmf_ctrlr_find_first_subsys_nsidx(struct xnvme_be_nvmf_ctrlr *ctrlr, int nsidx)
{
	struct xnvme_be_nvmf_subsys *subsys;

	SLIST_FOREACH(subsys, &ctrlr->subsystems, entry)
	{
		NVMF_DEBUG("INFO: Checking subsys with discovery_nsidx=%d against nsidx=%d",
			   subsys->discovery_nsidx, nsidx);
		if (subsys->discovery_nsidx == nsidx) {
			return subsys;
		}
	}

	return NULL;
}
