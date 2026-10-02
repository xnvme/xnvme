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
#include <xnvme_be_nvmf_qpair.h>
#include <xnvme_be_nvmf_debug.h>

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

/*
 * Device functions for NVMe-oF backend
 */
void
xnvme_be_nvmf_dev_close(struct xnvme_dev *dev)
{
	NVMF_DEBUG("INFO: dev_close() for NVMe-oF device: %s", dev->ident.uri);
}

int
xnvme_be_nvmf_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_nvmf_state *state = (struct xnvme_be_nvmf_state *)&dev->be.state;
	struct xnvme_be_nvmf_ctrlr *ctrlr = state->ctrlr;
	struct xnvme_be_nvmf_qpair *admin_qpair = NULL;

	NVMF_DEBUG("INFO: dev_open() for NVMe-oF device: dev=%p, %s, dtype: %d, nsid: 0x%x, "
		   "subnqn: %s",
		   dev, dev->ident.uri, dev->ident.dtype, dev->ident.nsid, dev->ident.subnqn);

	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	if (strlen(dev->ident.subnqn) == 0 ||
	    strcasecmp(dev->ident.subnqn, XNVME_NVMF_DISCOVERY_NQN) == 0 || dev->ident.nsid == 0 ||
	    dev->ident.nsid == UINT32_MAX) {
		dev->ident.dtype = XNVME_DEV_TYPE_NVME_CONTROLLER;
	} else {
		dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
	}

	dev->ident.nsid = dev->opts.nsid;

	if (dev->ident.dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
		// reuse the controller admin queue pair
		admin_qpair = ctrlr->admin_qpair;
	} else {
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
	state->admin_qpair = (void *)admin_qpair;
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
