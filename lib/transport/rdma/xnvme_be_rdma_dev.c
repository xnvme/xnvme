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
#include <xnvme_be_cbi.h>
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
#include <xnvme_be_rdma_transport.h>

#define TRANSPORT_CONTROLLER_NSID 0
#define TRANSPORT_BROADCAST_NSID UINT32_MAX

#define TRANSPORT_DEBUG_CATEGORY TRANSPORT_DEBUG_CATEGORY_CORE

static inline void
_rdma_dev_dump_qpair(struct xnvme_transport_qpair *XNVME_DEBUG_UNUSED(qpair))
{
	TRANSPORT_DEBUG("INFO: \nqpair=%p\n"
			"\tqid: %d\n"
			"\tqsize: %d\n"
			"\tcntlid: %d\n",
			qpair, qpair->attr.qid, qpair->attr.qsize, qpair->attr.cntlid);
}

static inline void
_rdma_dev_dump(struct xnvme_dev *dev)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);

	TRANSPORT_DEBUG(
		"INFO: \ndev=%p\n\tdev->ident: \n\t\turi: %s, \n\t\tcsi: %d, \n\t\tdtype: %d, "
		"\n\t\tnsid: %d, \n\t\tsubnqn: %s",
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
_rdma_dev_close_namespace(struct xnvme_dev *dev)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	int refs = 0;

	if (state->ns) {
		refs = xnvme_transport_ns_put(state->ns);
		if (refs) {
			TRANSPORT_DEBUG("INFO: ns still has %d references", refs);
			xnvme_transport_ns_get(state->ns); // taking reference to prevent close
							   // loop to free referenced memory
			return -EBUSY;
		}
		state->ns = NULL;
	}

	if (state->subsys) {
		refs = xnvme_transport_subsys_put(state->subsys);
		if (refs) {
			TRANSPORT_DEBUG("INFO: subsys still has %d references", refs);
			xnvme_transport_subsys_get(
				state->subsys); // taking reference to prevent close loop to free
						// referenced memory
			return -EBUSY;
		}
		state->subsys = NULL;
	}

	return 0;
}

/*
 * Device functions for NVMe-oF backend
 */
void
xnvme_be_rdma_dev_close(struct xnvme_dev *dev)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	int err;

	TRANSPORT_DEBUG("INFO: dev_close() for NVMe-oF device: %s", dev->ident.uri);

	if (state->admin_qpair) {
		err = xnvme_transport_qpair_disconnect(state->admin_qpair);
		if (err) {
			TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_disconnect(), err: %d",
					err);
			return;
		}

		err = xnvme_transport_qpair_destroy(state->admin_qpair);
		if (err) {
			TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_destroy(), err: %d", err);
			return;
		}
		state->admin_qpair = NULL;
	}

	if (state->sync_qpair) {
		err = xnvme_transport_qpair_disconnect(state->sync_qpair);
		if (err) {
			TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_disconnect(), err: %d",
					err);
			return;
		}

		err = xnvme_transport_qpair_destroy(state->sync_qpair);
		if (err) {
			TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_destroy(), err: %d", err);
			return;
		}
		state->sync_qpair = NULL;
	}

	xnvme_be_rdma_mem_term(dev);

	if (dev->ident.dtype == XNVME_DEV_TYPE_NVME_NAMESPACE) {
		_rdma_dev_close_namespace(dev);
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
			     struct xnvme_transport_qpair *qpair)
{
	int err;

	err = xnvme_transport_qpair_connect(qpair);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_connect(), err: %d", err);
		return err;
	}

	err = xnvme_transport_fabric_enable(ctrlr, qpair, false);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_fabric_enable(), err: %d", err);
	}

	return err;
}

static inline int
_rdma_dev_is_discovery_controller(struct xnvme_dev *dev)
{
	return !dev->opts.subnqn || strcasecmp(dev->opts.subnqn, XNVME_DISCOVERY_NQN) == 0;
}

/* Binds to the discovery controller, or to a subsystem controller when a subnqn is given. */
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

	state->subsys = subsys;

	if (!subsys->admin_qpair) {
		strncpy(dev->ident.subnqn, subsys->subnqn, sizeof(dev->ident.subnqn));

		err = xnvme_transport_qpair_create(ctrlr, dev, &attr, &subsys->admin_qpair);
		if (err) {
			TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_create(), err: %d", err);
			return err;
		}

		err = _rdma_dev_enable_admin_qpair(ctrlr, subsys->admin_qpair);
		if (err) {
			return err;
		}
	}
	state->admin_qpair = subsys->admin_qpair;

	return 0;
}

/* The first device open identifies the subsystem by index (no subnqn), later opens by subnqn. */
static inline int
_rdma_dev_bind_to_namespace(struct xnvme_dev *dev, const char *subnqn, int nsid)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	struct xnvme_transport_subsys *subsys;
	struct xnvme_transport_ns *ns;
	int err;

	subsys = subnqn ? xnvme_transport_ctrlr_find_first_subsys(state->ctrlr, subnqn)
			: xnvme_transport_ctrlr_find_first_subsys_nsidx(state->ctrlr, nsid);
	if (!subsys) {
		// Subsystems are created while walking the discovery log
		TRANSPORT_ERROR("FAILED: subsystem lookup, subnqn: %s, nsid: %d",
				subnqn ? subnqn : "-", nsid);
		return -EINVAL;
	}

	ns = xnvme_transport_subsys_find_first_ns(subsys, nsid);
	if (!ns) {
		strncpy(dev->ident.subnqn, subsys->subnqn, sizeof(dev->ident.subnqn));

		err = xnvme_transport_ns_create(state->ctrlr, dev, subsys, nsid, &ns);
		if (err) {
			TRANSPORT_ERROR("FAILED: xnvme_transport_ns_create(), err: %d", err);
			return err;
		}

		ns->admin_qpair->dev = dev;

		err = _rdma_dev_enable_admin_qpair(state->ctrlr, ns->admin_qpair);
		if (err) {
			return err;
		}
	}

	TRANSPORT_DEBUG("INFO: Bound to namespace: nsid=%u", ns->nsid);
	state->subsys = subsys;
	state->ns = ns;
	state->admin_qpair = ns->admin_qpair;

	return 0;
}

static inline int
_rdma_dev_create_sync_queue(struct xnvme_dev *dev)
{
	struct xnvme_transport_state *state = TO_TRANSPORT_DEV_STATE(dev);
	struct xnvme_transport_ctrlr *ctrlr = TO_TRANSPORT_CTRLR(state->ctrlr);
	struct xnvme_transport_qpair *adminq = TO_TRANSPORT_QPAIR(state->admin_qpair);
	struct xnvme_transport_qpair_attr attr = {
		.qid = ctrlr->last_allocated_queue_id++,
		.qsize = 8, // TODO: Tune this value as needed
		.capsule_size = NVME_CMD_CAPSULE_SIZE,
		.completion_size = NVME_CPL_CAPSULE_SIZE,
		.cntlid = adminq->attr.cntlid,
	};
	struct xnvme_transport_qpair *qpair;
	int err;

	err = xnvme_transport_qpair_create(ctrlr, dev, &attr, &qpair);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_transport_qpair_create(), err: %d", err);
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
			return err;
		}
	} else {
		err = _rdma_dev_bind_to_namespace(dev, dev->opts.subnqn, dev->opts.nsid);
		if (err) {
			TRANSPORT_ERROR("FAILED: _rdma_dev_bind_to_namespace(), err: %d", err);
			return err;
		}
	}

	err = xnvme_be_rdma_mem_init(dev);
	if (err) {
		TRANSPORT_ERROR("FAILED: xnvme_be_rdma_mem_init(), err: %d", err);
		return err;
	}

	if (!_rdma_dev_is_discovery_controller(dev)) {
		err = _rdma_dev_create_sync_queue(dev);
		if (err) {
			TRANSPORT_ERROR("FAILED: _rdma_dev_create_sync_queue(), err: %d", err);
			return err;
		}
	}

	_rdma_dev_dump(dev);

	return 0;
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
