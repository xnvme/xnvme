// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#include <xnvme_dev.h>

#include <xnvme_transport_debug.h>
#include <xnvme_transport_ctrlr.h>
#include <xnvme_transport_qpair.h>
#include <xnvme_transport_fabric.h>
#include <xnvme_transport_subsys.h>
#include <xnvme_transport_ns.h>

#include <xnvme_be_rdma.h>
#include <xnvme_be_rdma_mem.h>
#include <xnvme_be_rdma_ctrlr.h>

#define TRANSPORT_CONTROLLER_NSID 0
#define TRANSPORT_BROADCAST_NSID UINT32_MAX

#define TRANSPORT_DEBUG_CATEGORY TRANSPORT_DEBUG_CATEGORY_CORE

static inline void
_rdma_dev_dump_qpair(struct xnvme_transport_qpair *XNVME_DEBUG_UNUSED(qpair))
{
	TRANSPORT_DEBUG("INFO: \nqpair=%p\n"
			"\tqid: %u\n"
			"\tqsize: %u\n"
			"\tcntlid: %u\n",
			qpair, qpair->attr.qid, qpair->attr.qsize, qpair->attr.cntlid);
}

static inline void
_rdma_dev_dump(struct xnvme_dev *dev)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);

	TRANSPORT_DEBUG(
		"INFO: \ndev=%p\n\tdev->ident: \n\t\turi: %s, \n\t\tcsi: %d, \n\t\tdtype: %d, "
		"\n\t\tnsid: %u, \n\t\tsubnqn: %s",
		dev, dev->ident.uri, dev->ident.csi, dev->ident.dtype, dev->ident.nsid,
		dev->ident.subnqn);

	TRANSPORT_DEBUG("INFO: \ndev=%p \n"
			"\tstate.ctrlr: %p\n"
			"\tstate.ns: %p\n"
			"\tstate.admin_qpair: %p\n"
			"\tstate.sync_qpair: %p",
			dev, state->ctrlr, state->ns, state->admin_qpair, state->sync_qpair);

	if (state->admin_qpair) {
		_rdma_dev_dump_qpair(TO_TRANSPORT_QPAIR(state->admin_qpair));
	}
}

static inline int
_rdma_dev_is_discovery_controller(struct xnvme_dev *dev)
{
	return !dev->opts.subnqn || strcasecmp(dev->opts.subnqn, XNVME_DISCOVERY_NQN) == 0;
}

// Each device holds one reference on its namespace and one on its subsystem
static inline void
_rdma_dev_release_references(struct xnvme_dev *dev)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	struct xnvme_transport_ns *ns = state->ns;
	struct xnvme_transport_subsys *subsys = state->subsys;

	state->ns = NULL;
	state->subsys = NULL;

	if (ns) {
		xnvme_transport_ns_put(ns);
	}
	if (subsys) {
		xnvme_transport_subsys_put(subsys);
	}
}

/*
 * Device functions for NVMe-oF backend
 *
 * Closing cannot report failure, so every step is attempted and failures are logged.
 */
void
xnvme_be_rdma_dev_close(struct xnvme_dev *dev)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	int err;

	TRANSPORT_DEBUG("INFO: dev_close() for NVMe-oF device: %s", dev->ident.uri);

	if (dev->ident.dtype == XNVME_DEV_TYPE_NVME_NAMESPACE) {
		err = xnvme_transport_qpair_destroy(state->sync_qpair);
		if (err) {
			XNVME_DEBUG("FAILED: destroying the sync qpair, err: %d", err);
		}
		state->sync_qpair = NULL;
	}

	// The admin qpairs are shared and owned by the controller, subsystem or namespace
	_rdma_dev_release_references(dev);
	state->admin_qpair = NULL;

	err = xnvme_be_rdma_mem_term(dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_rdma_mem_term(), err: %d", err);
	}
}

static inline int
_rdma_dev_is_controller(struct xnvme_dev *dev)
{
	return dev->opts.nsid == TRANSPORT_CONTROLLER_NSID ||
	       dev->opts.nsid == TRANSPORT_BROADCAST_NSID;
}

static inline int
_rdma_dev_enable_admin_qpair(struct xnvme_transport_ctrlr *ctrlr,
			     struct xnvme_transport_qpair *qpair, uint16_t *mqes)
{
	int err;

	err = xnvme_transport_qpair_connect(qpair);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_connect(), err: %d", err);
		return err;
	}

	err = xnvme_transport_fabric_enable(ctrlr, qpair, false, mqes);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_fabric_enable(), err: %d", err);
	}

	return err;
}

// Binds to the discovery controller, or to a subsystem controller when a subnqn is given.
static inline int
_rdma_dev_bind_to_controller(struct xnvme_dev *dev)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	struct xnvme_transport_ctrlr *ctrlr = TO_TRANSPORT_CTRLR(state->ctrlr);
	struct xnvme_transport_qpair_attr attr = XNVME_TRANSPORT_ADMIN_QPAIR_ATTR;
	struct xnvme_transport_subsys *subsys;
	int err;

	state->subsys = NULL;
	state->ns = NULL;

	if (_rdma_dev_is_discovery_controller(dev)) {
		state->admin_qpair = ctrlr->admin_qpair;
		snprintf(dev->ident.subnqn, sizeof(dev->ident.subnqn), "%s", XNVME_DISCOVERY_NQN);

		return 0;
	}

	subsys = xnvme_transport_ctrlr_find_first_subsys(ctrlr, dev->opts.subnqn);
	if (!subsys) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_ctrlr_find_first_subsys(), subnqn: %s",
				dev->opts.subnqn);
		return -EINVAL;
	}

	TRANSPORT_INFO("INFO: Binding to subsystem controller, subsystem: %p, subnqn: %s", subsys,
		       subsys->subnqn);

	// Every qpair this device connects uses the subnqn, also when the admin qpair exists
	snprintf(dev->ident.subnqn, sizeof(dev->ident.subnqn), "%s", subsys->subnqn);

	if (!subsys->admin_qpair) {
		err = xnvme_transport_qpair_create(ctrlr, dev, &attr, &subsys->admin_qpair);
		if (err) {
			TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_create(), err: %d", err);
			return err;
		}

		err = _rdma_dev_enable_admin_qpair(ctrlr, subsys->admin_qpair, NULL);
		if (err) {
			// Do not leave a broken qpair for the next open to pick up
			xnvme_transport_qpair_destroy(subsys->admin_qpair);
			subsys->admin_qpair = NULL;
			return err;
		}
	}

	xnvme_transport_subsys_get(subsys);
	state->subsys = subsys;
	state->admin_qpair = subsys->admin_qpair;

	return 0;
}

/*
 * The first device open identifies the subsystem by index (no subnqn), later opens by subnqn.
 *
 * Without a subnqn, @nsid also serves as the 1-based index of a subsystem found in the discovery
 * log, so discovery probing (e.g. `xnvme info` and enumeration) can reach every subsystem without
 * knowing its subnqn. This is kept for compatibility with those tools.
 */
static inline int
_rdma_dev_bind_to_namespace(struct xnvme_dev *dev, const char *subnqn, uint32_t nsid)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	struct xnvme_transport_subsys *subsys;
	struct xnvme_transport_ns *ns;
	int err;

	subsys = subnqn ? xnvme_transport_ctrlr_find_first_subsys(state->ctrlr, subnqn)
			: xnvme_transport_ctrlr_find_first_subsys_nsidx(state->ctrlr, nsid);
	if (!subsys) {
		// Subsystems are created while walking the discovery log
		TRANSPORT_ERROR("FAILED: subsystem lookup, subnqn: %s, nsid: %u",
				subnqn ? subnqn : "-", nsid);
		return -EINVAL;
	}

	// Every qpair this device connects uses the subnqn, also when the namespace exists
	snprintf(dev->ident.subnqn, sizeof(dev->ident.subnqn), "%s", subsys->subnqn);

	ns = xnvme_transport_subsys_find_first_ns(subsys, nsid);
	if (ns) {
		xnvme_transport_ns_get(ns);
	} else {
		// The device owns the initial reference of a namespace it creates
		err = xnvme_transport_ns_create(state->ctrlr, dev, subsys, nsid, &ns);
		if (err) {
			TRANSPORT_ERROR("FAILED: xnvme_transport_ns_create(), err: %d", err);
			return err;
		}

		err = _rdma_dev_enable_admin_qpair(state->ctrlr, ns->admin_qpair, &ns->mqes);
		if (err) {
			// Do not leave a broken namespace for the next open to pick up
			xnvme_transport_ns_put(ns);
			return err;
		}
	}
	state->ns = ns;

	xnvme_transport_subsys_get(subsys);
	state->subsys = subsys;

	TRANSPORT_DEBUG("INFO: Bound to namespace: nsid=%u", ns->nsid);
	state->admin_qpair = ns->admin_qpair;

	return 0;
}

static inline int
_rdma_dev_create_sync_queue(struct xnvme_dev *dev)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	struct xnvme_transport_qpair *qpair;
	int err;

	err = xnvme_transport_qpair_create_io(state->ns, dev, 8, &qpair);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_create_io(), err: %d", err);
		return err;
	}
	state->sync_qpair = qpair;

	err = xnvme_transport_qpair_connect(qpair);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_connect(), err: %d", err);
		return err;
	}

	return 0;
}

static inline void
_rdma_dev_disable_io(struct xnvme_dev *dev)
{
	dev->be.async = (struct xnvme_be_async)XNVME_BE_NOSYS_QUEUE;
	dev->be.sync = (struct xnvme_be_sync)XNVME_BE_NOSYS_SYNC;
}

/*
 * Open an NVMe-oF device
 *
 * Note: dev->ident.uri is the only field that can be trusted to be correctly populated by the
 * caller. Other fields in dev->ident may not be reliable and should be verified or populated by
 * the backend.
 *
 * Other fields could be filled by other backends during the probing process, and may leave the
 * device in an inconsistent state.
 */
int
xnvme_be_rdma_dev_open(struct xnvme_dev *dev)
{
	int err;

	TRANSPORT_DEBUG("INFO: dev_open() for NVMe-oF device: dev=%p, (%s:%s)", dev,
			dev->ident.uri, dev->opts.subnqn);
	TRANSPORT_DEBUG("INFO: dev->opts: \n\t\tnsid: 0x%x\n\t\tsubnqn: %s\n\t\thostnqn: %s",
			dev->opts.nsid, dev->opts.subnqn, dev->opts.hostnqn);

	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	if (_rdma_dev_is_controller(dev)) {
		dev->ident.dtype = XNVME_DEV_TYPE_NVME_CONTROLLER;
	} else {
		dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
	}

	dev->ident.nsid = dev->opts.nsid;

	if (dev->ident.dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
		err = _rdma_dev_bind_to_controller(dev);
		if (err) {
			TRANSPORT_ERROR("FAILED: _rdma_dev_bind_to_controller(), err: %d", err);
			goto close_device;
		}
	} else {
		err = _rdma_dev_bind_to_namespace(dev, dev->opts.subnqn, dev->opts.nsid);
		if (err) {
			TRANSPORT_ERROR("FAILED: _rdma_dev_bind_to_namespace(), err: %d", err);
			goto close_device;
		}
	}

	err = xnvme_be_rdma_mem_init(dev);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_be_rdma_mem_init(), err: %d", err);
		goto close_device;
	}

	// Only namespaces have I/O queues
	if (dev->ident.dtype == XNVME_DEV_TYPE_NVME_NAMESPACE) {
		err = _rdma_dev_create_sync_queue(dev);
		if (err) {
			TRANSPORT_ERROR("FAILED: _rdma_dev_create_sync_queue(), err: %d", err);
			goto close_device;
		}
	} else {
		_rdma_dev_disable_io(dev);
	}

	_rdma_dev_dump(dev);

	return 0;

close_device:
	xnvme_be_rdma_dev_close(dev);

	return err;
}

static void *
xnvme_be_rdma_ctrlr_init(struct xnvme_dev *dev)
{
	return xnvme_transport_dev_ctrlr_init(dev, &g_xnvme_be_rdma_transport);
}

static int
xnvme_be_rdma_ctrlr_term(void *ctrlr)
{
	return xnvme_transport_dev_ctrlr_term(ctrlr);
}

struct xnvme_be_dev g_xnvme_be_rdma_dev = {
	.id = "rdma",
	.dev_open = xnvme_be_rdma_dev_open,
	.dev_close = xnvme_be_rdma_dev_close,
	.ctrlr_init = xnvme_be_rdma_ctrlr_init,
	.ctrlr_term = xnvme_be_rdma_ctrlr_term,
};
