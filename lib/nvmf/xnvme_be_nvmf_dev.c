// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_debug.h>
#include <xnvme_be_nvmf_fabric.h>
#include <xnvme_be_nvmf_mem.h>
#include <xnvme_be_nvmf_ns.h>
#include <xnvme_be_nvmf_qpair.h>
#include <xnvme_be_nvmf_subsys.h>

#define NVMF_CONTROLLER_NSID 0
#define NVMF_BROADCAST_NSID UINT32_MAX

#define NVMF_DEBUG_CATEGORY NVMF_DEBUG_CATEGORY_CORE

static inline void
_nvmf_dev_dump_qpair(struct xnvme_be_nvmf_qpair *XNVME_DEBUG_UNUSED(qpair))
{
	NVMF_DEBUG("INFO: \nqpair=%p\n"
		   "\tqid: %d\n"
		   "\tqsize: %d\n"
		   "\tcntlid: %d\n",
		   qpair, qpair->attr.qid, qpair->attr.qsize, qpair->attr.cntlid);
}

static inline void
_nvmf_dev_dump(struct xnvme_dev *dev)
{
	struct xnvme_be_nvmf_state *state = TO_NVMF_DEV_STATE(dev);

	NVMF_DEBUG("INFO: \ndev=%p\n\tdev->ident: \n\t\turi: %s, \n\t\tcsi: %d, \n\t\tdtype: %d, "
		   "\n\t\tnsid: %d, \n\t\tsubnqn: %s",
		   dev, dev->ident.uri, dev->ident.csi, dev->ident.dtype, dev->ident.nsid,
		   dev->ident.subnqn);

	NVMF_DEBUG("INFO: \ndev=%p \n"
		   "\tstate.ctrlr: %p\n"
		   "\tstate.ns: %p\n"
		   "\tstate.admin_qpair: %p\n"
		   "\tstate.sync_qpair: %p",
		   dev, state->ctrlr, state->ns, state->admin_qpair, state->sync_qpair);

	if (state->admin_qpair) {
		_nvmf_dev_dump_qpair(TO_NVMF_QPAIR(state->admin_qpair));
	}
}

static inline int
_nvmf_dev_close_namespace(struct xnvme_dev *dev)
{
	struct xnvme_be_nvmf_state *state = TO_NVMF_DEV_STATE(dev);
	int refs = 0;

	if (state->ns) {
		refs = xnvme_be_nvmf_ns_put(state->ns);
		if (refs) {
			NVMF_DEBUG("INFO: ns still has %d references", refs);
			xnvme_be_nvmf_ns_get(state->ns); // taking reference to prevent close loop
							 // to free referenced memory
			return -EBUSY;
		}
		state->ns = NULL;
	}

	if (state->subsys) {
		refs = xnvme_be_nvmf_subsys_put(state->subsys);
		if (refs) {
			NVMF_DEBUG("INFO: subsys still has %d references", refs);
			xnvme_be_nvmf_subsys_get(
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
xnvme_be_nvmf_dev_close(struct xnvme_dev *dev)
{
	struct xnvme_be_nvmf_state *state = TO_NVMF_DEV_STATE(dev);
	int err;

	NVMF_DEBUG("INFO: dev_close() for NVMe-oF device: %s", dev->ident.uri);

	if (state->admin_qpair) {
		err = xnvme_be_nvmf_qpair_disconnect(state->admin_qpair);
		if (err) {
			NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_disconnect(), err: %d", err);
			return;
		}

		err = xnvme_be_nvmf_qpair_destroy(state->admin_qpair);
		if (err) {
			NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_destroy(), err: %d", err);
			return;
		}
		state->admin_qpair = NULL;
	}

	if (state->sync_qpair) {
		err = xnvme_be_nvmf_qpair_disconnect(state->sync_qpair);
		if (err) {
			NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_disconnect(), err: %d", err);
			return;
		}

		err = xnvme_be_nvmf_qpair_destroy(state->sync_qpair);
		if (err) {
			NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_destroy(), err: %d", err);
			return;
		}
		state->sync_qpair = NULL;
	}

	xnvme_be_nvmf_mem_term(dev);

	if (dev->ident.dtype == XNVME_DEV_TYPE_NVME_NAMESPACE) {
		_nvmf_dev_close_namespace(dev);
	}
}

static inline int
_nvmf_dev_is_controller(struct xnvme_dev *dev)
{
	return dev->opts.nsid == NVMF_CONTROLLER_NSID || dev->opts.nsid == NVMF_BROADCAST_NSID;
}

static inline int
_nvmf_dev_enable_admin_qpair(struct xnvme_be_nvmf_ctrlr *ctrlr, struct xnvme_be_nvmf_qpair *qpair)
{
	int err;

	err = xnvme_be_nvmf_qpair_connect(qpair);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_connect(), err: %d", err);
		return err;
	}

	err = xnvme_be_nvmf_fabric_enable(ctrlr, qpair, false);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_fabric_enable(), err: %d", err);
	}

	return err;
}

static inline int
_nvmf_dev_is_discovery_controller(struct xnvme_dev *dev)
{
	return !dev->opts.subnqn || strcasecmp(dev->opts.subnqn, XNVME_DISCOVERY_NQN) == 0;
}

/* Binds to the discovery controller, or to a subsystem controller when a subnqn is given. */
static inline int
_nvmf_dev_bind_to_controller(struct xnvme_dev *dev)
{
	struct xnvme_be_nvmf_state *state = TO_NVMF_DEV_STATE(dev);
	struct xnvme_be_nvmf_ctrlr *ctrlr = TO_NVMF_CTRLR(state->ctrlr);
	struct xnvme_be_nvmf_qpair_attr attr = XNVME_BE_NVMF_ADMIN_QPAIR_ATTR;
	struct xnvme_be_nvmf_subsys *subsys;
	int err;

	state->subsys = NULL;
	state->ns = NULL;

	if (_nvmf_dev_is_discovery_controller(dev)) {
		state->admin_qpair = ctrlr->admin_qpair;
		return 0;
	}

	subsys = xnvme_be_nvmf_ctrlr_find_first_subsys(ctrlr, dev->opts.subnqn);
	if (!subsys) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_find_first_subsys(), subnqn: %s",
			   dev->opts.subnqn);
		return -EINVAL;
	}

	NVMF_INFO("INFO: Binding to subsystem controller, subsystem: %p, subnqn: %s", subsys,
		  subsys->subnqn);

	state->subsys = subsys;

	if (!subsys->admin_qpair) {
		strncpy(dev->ident.subnqn, subsys->subnqn, sizeof(dev->ident.subnqn));

		err = xnvme_be_nvmf_qpair_create(ctrlr, dev, &attr, &subsys->admin_qpair);
		if (err) {
			NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_create(), err: %d", err);
			return err;
		}

		err = _nvmf_dev_enable_admin_qpair(ctrlr, subsys->admin_qpair);
		if (err) {
			return err;
		}
	}
	state->admin_qpair = subsys->admin_qpair;

	return 0;
}

/* The first device open identifies the subsystem by index (no subnqn), later opens by subnqn. */
static inline int
_nvmf_dev_bind_to_namespace(struct xnvme_dev *dev, const char *subnqn, int nsid)
{
	struct xnvme_be_nvmf_state *state = TO_NVMF_DEV_STATE(dev);
	struct xnvme_be_nvmf_subsys *subsys;
	struct xnvme_be_nvmf_ns *ns;
	int err;

	subsys = subnqn ? xnvme_be_nvmf_ctrlr_find_first_subsys(state->ctrlr, subnqn)
			: xnvme_be_nvmf_ctrlr_find_first_subsys_nsidx(state->ctrlr, nsid);
	if (!subsys) {
		// Subsystems are created while walking the discovery log
		NVMF_ERROR("FAILED: subsystem lookup, subnqn: %s, nsid: %d", subnqn ? subnqn : "-",
			   nsid);
		return -EINVAL;
	}

	ns = xnvme_be_nvmf_subsys_find_first_ns(subsys, nsid);
	if (!ns) {
		strncpy(dev->ident.subnqn, subsys->subnqn, sizeof(dev->ident.subnqn));

		err = xnvme_be_nvmf_ns_create(state->ctrlr, dev, subsys, nsid, &ns);
		if (err) {
			NVMF_ERROR("FAILED: xnvme_be_nvmf_ns_create(), err: %d", err);
			return err;
		}

		ns->admin_qpair->dev = dev;

		err = _nvmf_dev_enable_admin_qpair(state->ctrlr, ns->admin_qpair);
		if (err) {
			return err;
		}
	}

	NVMF_DEBUG("INFO: Bound to namespace: nsid=%u", ns->nsid);
	state->subsys = subsys;
	state->ns = ns;
	state->admin_qpair = ns->admin_qpair;

	return 0;
}

static inline int
_nvmf_dev_create_sync_queue(struct xnvme_dev *dev)
{
	struct xnvme_be_nvmf_state *state = TO_NVMF_DEV_STATE(dev);
	struct xnvme_be_nvmf_ctrlr *ctrlr = TO_NVMF_CTRLR(state->ctrlr);
	struct xnvme_be_nvmf_qpair *adminq = TO_NVMF_QPAIR(state->admin_qpair);
	struct xnvme_be_nvmf_qpair_attr attr = {
		.qid = ctrlr->last_allocated_queue_id++,
		.qsize = 8, // TODO: Tune this value as needed
		.capsule_size = NVME_CMD_CAPSULE_SIZE,
		.completion_size = NVME_CPL_CAPSULE_SIZE,
		.cntlid = adminq->attr.cntlid,
	};
	struct xnvme_be_nvmf_qpair *qpair;
	int err;

	err = xnvme_be_nvmf_qpair_create(ctrlr, dev, &attr, &qpair);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_create(), err: %d", err);
		return err;
	}
	state->sync_qpair = qpair;

	err = xnvme_be_nvmf_qpair_connect(qpair);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_connect(), err: %d", err);
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
xnvme_be_nvmf_dev_open(struct xnvme_dev *dev)
{
	int err;

	NVMF_DEBUG("INFO: dev_open() for NVMe-oF device: dev=%p, (%s:%s)", dev, dev->ident.uri,
		   dev->opts.subnqn);
	NVMF_DEBUG("INFO: dev->opts: \n\t\tnsid: 0x%x\n\t\tsubnqn: %s\n\t\thostnqn: %s",
		   dev->opts.nsid, dev->opts.subnqn, dev->opts.hostnqn);

	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	if (_nvmf_dev_is_controller(dev)) {
		dev->ident.dtype = XNVME_DEV_TYPE_NVME_CONTROLLER;
	} else {
		dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
	}

	dev->ident.nsid = dev->opts.nsid;

	if (dev->ident.dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
		err = _nvmf_dev_bind_to_controller(dev);
		if (err) {
			NVMF_ERROR("FAILED: _nvmf_dev_bind_to_controller(), err: %d", err);
			return err;
		}
	} else {
		err = _nvmf_dev_bind_to_namespace(dev, dev->opts.subnqn, dev->opts.nsid);
		if (err) {
			NVMF_ERROR("FAILED: _nvmf_dev_bind_to_namespace(), err: %d", err);
			return err;
		}
	}

	err = xnvme_be_nvmf_mem_init(dev);
	if (err) {
		NVMF_ERROR("FAILED: xnvme_be_nvmf_mem_init(), err: %d", err);
		return err;
	}

	if (!_nvmf_dev_is_discovery_controller(dev)) {
		err = _nvmf_dev_create_sync_queue(dev);
		if (err) {
			NVMF_ERROR("FAILED: _nvmf_dev_create_sync_queue(), err: %d", err);
			return err;
		}
	}

	_nvmf_dev_dump(dev);

	return 0;
}

struct xnvme_be_dev g_xnvme_be_nvmf_dev = {
	.id = "nvmf",
	.dev_open = xnvme_be_nvmf_dev_open,
	.dev_close = xnvme_be_nvmf_dev_close,
	.ctrlr_init = xnvme_be_nvmf_dev_ctrlr_init,
	.ctrlr_term = xnvme_be_nvmf_dev_ctrlr_term,
};
