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

#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>

#include <xnvme_be_nvmf.h>

/**
 * Initialize a new controller for the device URI.
 *
 * Inits the environment, if needed, and probes for a device matching the URI.
 */
static void *
xnvme_be_nvmf_ctrlr_init(struct xnvme_dev *dev)
{
	return NULL;
}

static int
xnvme_be_nvmf_ctrlr_term(void *ctrlr)
{
	return -ENOSYS;
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
