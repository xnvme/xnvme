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
#include <xnvme_be_nvmf_ns.h>
#include <xnvme_be_nvmf_qpair.h>
#include <xnvme_be_nvmf_subsys.h>

#define NVMF_CONTROLLER_NSID 0
#define NVMF_BROADCAST_NSID UINT32_MAX

#define NVMF_DEBUG_CATEGORY NVMF_DEBUG_CATEGORY_CORE

static inline void
_dump_qpair(struct xnvme_be_nvmf_qpair *qpair)
{
	NVMF_DEBUG("INFO: \nqpair=%p\n"
		   "\tqid: %d\n"
		   "\tqsize: %d\n"
		   "\tcntlid: %d\n",
		   qpair, qpair->attr.qid, qpair->attr.qsize, qpair->cntlid);
}

static inline void
_dump_dev(struct xnvme_dev *dev)
{
	struct xnvme_be_nvmf_state *state = (struct xnvme_be_nvmf_state *)&dev->be.state;

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
		_dump_qpair((struct xnvme_be_nvmf_qpair *)state->admin_qpair);
	}
}

static inline int
_nvmf_dev_close_controller(struct xnvme_dev *XNVME_UNUSED(dev))
{
	return 0;
}

static inline int
_nvmf_dev_close_namespace(struct xnvme_dev *dev)
{
	struct xnvme_be_nvmf_state *state = (struct xnvme_be_nvmf_state *)&dev->be.state;
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
	NVMF_DEBUG("INFO: dev_close() for NVMe-oF device: %s", dev->ident.uri);

	if (dev->ident.dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
		_nvmf_dev_close_controller(dev);
	} else if (dev->ident.dtype == XNVME_DEV_TYPE_NVME_NAMESPACE) {
		_nvmf_dev_close_namespace(dev);
	}
}

static inline int
_nvmf_is_controller(struct xnvme_dev *dev)
{
	return dev->opts.nsid == NVMF_CONTROLLER_NSID || dev->opts.nsid == NVMF_BROADCAST_NSID;
}

static inline int
_nvmf_is_discovery(const char *subnqn)
{
	return !subnqn || strcasecmp(subnqn, XNVME_NVMF_DISCOVERY_NQN) == 0;
}

static inline int
_nvmf_dev_bind_to_controller(struct xnvme_dev *dev)
{
	struct xnvme_be_nvmf_state *state = (struct xnvme_be_nvmf_state *)&dev->be.state;
	struct xnvme_be_nvmf_ctrlr *ctrlr = state->ctrlr;

	state->subsys = NULL;
	state->ns = NULL;
	state->admin_qpair = ctrlr->admin_qpair;

	return 0;
}

static inline int
_nvmf_dev_bind_to_namespace(struct xnvme_dev *dev, const char *subnqn, int nsid)
{
	struct xnvme_be_nvmf_state *state = (struct xnvme_be_nvmf_state *)&dev->be.state;
	struct xnvme_be_nvmf_subsys *subsys;
	struct xnvme_be_nvmf_ns *ns;
	int err;

	if (!subnqn) {
		// This is coming from first device init, and NSID should index the subsystem
		subsys = xnvme_be_nvmf_ctrlr_find_first_subsys_nsidx(state->ctrlr, nsid);
		if (!subsys) {
			NVMF_ERROR(
				"FAILED: xnvme_be_nvmf_ctrlr_find_first_subsys_nsidx(), nsid: %d",
				nsid);
			return -EINVAL; // should never happen because subsystems should be created
					// when walking the discovery log
		}
	} else {
		// This is coming from subsequent device init, and subnqn should be provided
		subsys = xnvme_be_nvmf_ctrlr_find_first_subsys(state->ctrlr, subnqn);
		if (!subsys) {
			NVMF_ERROR("FAILED: xnvme_be_nvmf_ctrlr_find_first_subsys(), subnqn: %s",
				   subnqn);
			return -EINVAL; // should never happen because subsystems should be created
					// when walking the discovery log
		}
	}

	ns = xnvme_be_nvmf_subsys_find_first_ns(subsys, nsid);
	if (!ns) {
		NVMF_DEBUG("WARN: xnvme_be_nvmf_subsys_find_first_ns(), nsid: %d", nsid);
		err = xnvme_be_nvmf_ns_create(state->ctrlr, dev, subsys, nsid, &ns);
		if (err) {
			NVMF_ERROR("FAILED: xnvme_be_nvmf_ns_create(), err: %d", err);
			return err;
		}

		strncpy(dev->ident.subnqn, subsys->subnqn, sizeof(dev->ident.subnqn));

		ns->admin_qpair->dev = dev;
		NVMF_DEBUG("INFO: Set dev->ident.subnqn to %s", dev->ident.subnqn);

		err = xnvme_be_nvmf_ns_connect(ns);
		if (err) {
			NVMF_ERROR("FAILED: xnvme_be_nvmf_ns_connect(), err: %d", err);
			return err;
		}

		err = xnvme_be_nvmf_fabric_enable(state->ctrlr, ns->admin_qpair, false);
		if (err) {
			NVMF_ERROR("FAILED: xnvme_be_nvmf_fabric_enable(), err: %d", err);
			return err;
		}
	}

	NVMF_DEBUG("INFO: Binding to : nsid=%u", ns->nsid);
	NVMF_DEBUG("INFO: Bound to namespace: nsid=%u", ns->nsid);
	state->subsys = subsys;
	state->ns = ns;
	state->admin_qpair = ns->admin_qpair; // TODO: Incorrect

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
	NVMF_DEBUG("INFO: dev_open() for NVMe-oF device: dev=%p, (%s)", dev, dev->ident.uri);
	NVMF_DEBUG("INFO: dev->opts: \n\t\tnsid: 0x%x\n\t\tsubnqn: %s\n\t\thostnqn: %s",
		   dev->opts.nsid, dev->opts.subnqn, dev->opts.hostnqn);

	dev->ident.csi = XNVME_SPEC_CSI_NVM;
#if 0
	if (_nvmf_is_discovery(dev->opts.subnqn) || 
		_nvmf_is_controller(dev)) {  // TODO: This should not include the discovery check
#endif
	if (_nvmf_is_controller(dev)) {
		dev->ident.dtype = XNVME_DEV_TYPE_NVME_CONTROLLER;
	} else {
		dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
	}

	dev->ident.nsid = dev->opts.nsid;

	if (dev->ident.dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
		_nvmf_dev_bind_to_controller(dev);
	} else {
		_nvmf_dev_bind_to_namespace(dev, dev->opts.subnqn, dev->opts.nsid);
#if 0
		int err;
		// create a new admin queue pair for the namespace
		err = xnvme_be_nvmf_qpair_create(ctrlr, dev, &admin_attr, &admin_qpair);
		if (err) {
			NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_create(), err: %d", err);
			return err;
		}

		// Set the device pointer for the newly created admin queue pair
		err = xnvme_be_nvmf_qpair_connect(admin_qpair);
		if (err) {
			NVMF_ERROR("FAILED: xnvme_be_nvmf_qpair_connect(), err: %d", err);
			xnvme_be_nvmf_qpair_destroy(admin_qpair);
			return err;
		}
#endif
	}

	// The ctrlr is enabled once by xnvme_be_nvmf_dev_ctrlr_init(), right after
	// probe. dev_open only wires the per-device admin_qpair pointer, so a cref
	// hit does not re-enable an already-enabled controller.
	_dump_dev(dev);

	return 0;
}

struct xnvme_be_dev g_xnvme_be_nvmf_dev = {
	.id = "nvmf",
	.dev_open = xnvme_be_nvmf_dev_open,
	.dev_close = xnvme_be_nvmf_dev_close,
	.ctrlr_init = xnvme_be_nvmf_dev_ctrlr_init,
	.ctrlr_term = xnvme_be_nvmf_dev_ctrlr_term,
};
