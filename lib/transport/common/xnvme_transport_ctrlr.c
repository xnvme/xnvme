// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stdatomic.h>

#include <xnvme_dev.h>

#include <xnvme_transport.h>
#include <xnvme_transport_base.h>
#include <xnvme_transport_debug.h>
#include <xnvme_transport_qpair.h>
#include <xnvme_transport_fabric.h>
#include <xnvme_transport_subsys.h>
#include <xnvme_transport_ctrlr.h>

#define TRANSPORT_DEBUG_CATEGORY TRANSPORT_DEBUG_CATEGORY_CORE_CTRLR

/**
 * Maximum number of attempts to probe for a device matching the provided URI.
 *
 * This is used to retry probing for a device in case of transient errors or
 * delays in the NVMe-oF subsystem.
 */
#define XNVME_TRANSPORT_MAX_PROBE_ATTEMPTS 3

int
xnvme_transport_ctrlr_connect(struct xnvme_transport_ctrlr *ctrlr, const char *uri)
{
	int err;
	if (!ctrlr || !uri)
		return -EINVAL;

	err = ctrlr->ops->ctrlr_connect(ctrlr, uri);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_ctrlr_connect(), err: %d", err);
		ctrlr->ctrlr_state = XNVME_TRANSPORT_CTRLR_STATE_ERROR;
		return err;
	}

	ctrlr->ctrlr_state = XNVME_TRANSPORT_CTRLR_STATE_CONNECTED;
	ctrlr->attached = 1;

	return err;
}

int
xnvme_transport_ctrlr_disconnect(struct xnvme_transport_ctrlr *ctrlr)
{
	return ctrlr->ops->ctrlr_disconnect(ctrlr);
}

int
xnvme_transport_ctrlr_create(struct xnvme_transport *transport,
			     struct xnvme_transport_ctrlr_attr *attr,
			     struct xnvme_transport_ctrlr **ctrlr)
{
	struct xnvme_transport_ctrlr *tmp = NULL;
	struct xnvme_transport_qpair_attr default_qpair_attr = XNVME_TRANSPORT_ADMIN_QPAIR_ATTR;
	int err;

	err = transport->ctrlr_probe(&tmp);
	if (err) {
		TRANSPORT_ERROR("FAILED: transport->ctrlr_probe(), err: %d", err);
		return err;
	}

	tmp->ctrlr_state = XNVME_TRANSPORT_CTRLR_STATE_INIT;
	tmp->last_allocated_queue_id = XNVME_TRANSPORT_IO_QUEUE_ID_START;
	tmp->attached = 0;

	err = xnvme_transport_qpair_create(tmp, attr->dev, &default_qpair_attr, &tmp->admin_qpair);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_create() for admin_qpair, err: %d",
				err);
		xnvme_transport_ctrlr_destroy(tmp);
		;
		return err;
	}

	*ctrlr = tmp;
	return err;
}

int
xnvme_transport_ctrlr_destroy(struct xnvme_transport_ctrlr *ctrlr)
{
	struct xnvme_transport_subsys *subsys;
	int err;

	if (!ctrlr) {
		TRANSPORT_ERROR("FAILED: NULL ctrlr");
		return -EINVAL;
	}

	// Release the controller's reference on every subsystem, devices hold their own
	subsys = SLIST_FIRST(&ctrlr->subsystems);
	while (subsys) {
		struct xnvme_transport_subsys *next = SLIST_NEXT(subsys, entry);

		xnvme_transport_subsys_put(subsys);
		subsys = next;
	}

	err = ctrlr->ops->ctrlr_teardown(ctrlr);
	if (err) {
		TRANSPORT_ERROR("FAILED: ctrlr->ops->ctrlr_teardown(), err: %d", err);
	}

	/*
	 * Free unconditionally, even when teardown fails, so a failed
	 * teardown does not leak the ctrlr.
	 */
	ctrlr->ops->ctrlr_free(ctrlr);

	return err;
}

static inline int
_transport_ctrlr_probe_transport(struct xnvme_transport *transport,
				 struct xnvme_transport_ctrlr_attr *attr,
				 struct xnvme_transport_ctrlr **ctrlr)
{
	struct xnvme_transport_ctrlr *tmp_ctrlr;
	int err;

	err = xnvme_transport_ctrlr_create(transport, attr, &tmp_ctrlr);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_ctrlr_create(), err: %d", err);
		return err;
	}

	err = xnvme_transport_ctrlr_connect(tmp_ctrlr, attr->dev->ident.uri);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_ctrlr_connect(), err: %d", err);
		goto destroy_controller;
	}

	*ctrlr = tmp_ctrlr;

	return 0;

destroy_controller:
	xnvme_transport_ctrlr_destroy(tmp_ctrlr);
	return err;
}

static inline void
_transport_ctrlr_dump(struct xnvme_transport_ctrlr *XNVME_DEBUG_UNUSED(ctrlr))
{
	TRANSPORT_DEBUG("INFO: ctrlr: \n"
			"\tctrlr_id: %d\n"
			"\tctrlr_state: %d\n"
			"\tattached: %d",
			ctrlr->admin_qpair->attr.cntlid, ctrlr->ctrlr_state, ctrlr->attached);
}

/**
 * Probes every registered transport for @dev->ident.uri.
 *
 * Runs up to XNVME_TRANSPORT_MAX_PROBE_ATTEMPTS rounds, each walking every
 * registered transport. On success `*ctrlr` is a connected controller.
 */
int
xnvme_transport_ctrlr_probe(struct xnvme_dev *dev, struct xnvme_transport *transport,
			    struct xnvme_transport_ctrlr **ctrlr)
{
	struct xnvme_transport_ctrlr *tmp_ctrlr = NULL;
	struct xnvme_transport_ctrlr_attr attr = {
		.dev = dev,
	};
	int err = -ENXIO;

	for (int i = 0; !tmp_ctrlr; ++i) {
		// If the maximum number of attempts is reached, return an error.
		if (XNVME_TRANSPORT_MAX_PROBE_ATTEMPTS == i) {
			TRANSPORT_ERROR("FAILED: max attempts exceeded");
			return -ENXIO;
		}

		TRANSPORT_INFO("INFO: Attempting to probe transport: %s", transport->name);

		err = _transport_ctrlr_probe_transport(transport, &attr, &tmp_ctrlr);
		if (err) {
			TRANSPORT_INFO("INFO: transport->probe() failed for transport: %s, "
				       "err: %d",
				       transport->name, err);
		}
	}

	TRANSPORT_INFO("INFO: Successfully connected to transport: [%s:%s]", transport->name,
		       dev->ident.uri);
	*ctrlr = tmp_ctrlr;
	return 0;
}

/**
 * Sets CC.EN and waits on CSTS.RDY, through Fabrics Property Get and Set on
 * the admin qpair. Forwards to `xnvme_transport_fabric_enable()` (fabric.c).
 */
int
xnvme_transport_ctrlr_enable(struct xnvme_transport_ctrlr *ctrlr)
{
	if (!ctrlr || !ctrlr->admin_qpair) {
		TRANSPORT_ERROR("FAILED: Invalid ctrlr or missing admin_qpair");
		return -EINVAL;
	}

	return xnvme_transport_fabric_enable(ctrlr, ctrlr->admin_qpair, true);
}

int
xnvme_transport_ctrlr_reg(struct xnvme_transport_ctrlr *ctrlr, void *buf, size_t nbytes,
			  void **handle, uint32_t *key)
{
	if (!ctrlr || !ctrlr->ops || !ctrlr->ops->ctrlr_reg) {
		TRANSPORT_ERROR("FAILED: No ctrlr_reg operation defined for controller");
		return -ENOSYS;
	}

	return ctrlr->ops->ctrlr_reg(ctrlr, buf, nbytes, handle, key);
}

int
xnvme_transport_ctrlr_dereg(struct xnvme_transport_ctrlr *ctrlr, void *handle)
{
	if (!ctrlr || !ctrlr->ops || !ctrlr->ops->ctrlr_dereg) {
		TRANSPORT_ERROR("FAILED: No ctrlr_dereg operation defined for controller");
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
xnvme_transport_dev_ctrlr_init(struct xnvme_dev *dev, struct xnvme_transport *transport)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	struct xnvme_transport_ctrlr *ctrlr = NULL;
	int err;

	TRANSPORT_INFO("INFO: dev_ctrlr_init() for NVMe-oF device: %s, ns=%u, discovery=%s",
		       dev->ident.uri, dev->ident.nsid, dev->ident.nsid == 0 ? "yes" : "no");

	if (state->ctrlr) {
		TRANSPORT_INFO(
			"INFO: Controller already initialized, reusing existing controller");
		return state->ctrlr;
	}

	err = xnvme_transport_ctrlr_probe(dev, transport, &ctrlr);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_ctrlr_probe(), err: %d", err);
		errno = -err;
		return NULL;
	}

	ctrlr->last_assigned_discovery_id = 0;

	_transport_ctrlr_dump(ctrlr);

	err = xnvme_transport_ctrlr_enable(ctrlr);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_ctrlr_enable(), err: %d", err);
		xnvme_transport_ctrlr_disconnect(ctrlr);
		xnvme_transport_ctrlr_destroy(ctrlr);
		errno = -err;
		return NULL;
	}

	TRANSPORT_INFO("INFO: dev_ctrlr_init() OK");
	return ctrlr;
}

int
xnvme_transport_dev_ctrlr_term(void *ctrlr)
{
	struct xnvme_transport_ctrlr *transport_ctrlr = TO_TRANSPORT_CTRLR(ctrlr);
	int err;

	TRANSPORT_INFO("INFO: dev_ctrlr_term() for NVMe-oF controller");

	if (transport_ctrlr) {
		if (transport_ctrlr->ctrlr_state == XNVME_TRANSPORT_CTRLR_STATE_CONNECTED) {
			err = xnvme_transport_ctrlr_disconnect(transport_ctrlr);
			if (err) {
				TRANSPORT_ERROR(
					"FAILED: xnvme_transport_ctrlr_disconnect(), err: %d",
					err);
				return err;
			}
		}

		err = xnvme_transport_ctrlr_destroy(transport_ctrlr);
		if (err) {
			TRANSPORT_ERROR("FAILED: xnvme_transport_ctrlr_destroy(), err: %d", err);
			return err;
		}
	}

	return 0;
}

struct xnvme_transport_subsys *
xnvme_transport_ctrlr_find_first_subsys(struct xnvme_transport_ctrlr *ctrlr, const char *subnqn)
{
	struct xnvme_transport_subsys *subsys;

	SLIST_FOREACH(subsys, &ctrlr->subsystems, entry)
	{
		if (strcmp(subsys->subnqn, subnqn) == 0) {
			return subsys;
		}
	}

	return NULL;
}

struct xnvme_transport_subsys *
xnvme_transport_ctrlr_find_first_subsys_nsidx(struct xnvme_transport_ctrlr *ctrlr, int nsidx)
{
	struct xnvme_transport_subsys *subsys;

	SLIST_FOREACH(subsys, &ctrlr->subsystems, entry)
	{
		TRANSPORT_DEBUG("INFO: Checking subsys with discovery_nsidx=%d against nsidx=%d",
				subsys->discovery_nsidx, nsidx);
		if (subsys->discovery_nsidx == nsidx) {
			return subsys;
		}
	}

	return NULL;
}
