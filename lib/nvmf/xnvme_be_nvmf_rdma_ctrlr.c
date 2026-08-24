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

int
xnvme_be_nvmf_create_rdma_controller(struct xnvme_be_nvmf_ctrlr **ctrlr)
{
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr;

	rdma_ctrlr = calloc(1, sizeof(*rdma_ctrlr));
	if (!rdma_ctrlr) {
		XNVME_DEBUG("FAILED: calloc(), err: %d", errno);
		return -ENOMEM;
	}
	pthread_mutex_init(&rdma_ctrlr->lock, NULL);

	rdma_ctrlr->base.ops = &g_xnvme_be_nvmf_rdma_ctrlr_ops;
	*ctrlr = &rdma_ctrlr->base;

	return 0;

free_ctrlr:
	free(rdma_ctrlr);
	return -ENOMEM;
}

static int
_destroy_rdma_controller(struct xnvme_be_nvmf_ctrlr *ctrlr)
{
	struct xnvme_be_nvmf_rdma_ctrlr *rdma_ctrlr = TO_XNVME_NVMF_RDMA_CTRLR(ctrlr);

	return 0;
}

static struct xnvme_be_nvmf_ctrlr_ops g_xnvme_be_nvmf_rdma_ctrlr_ops = {
	.destroy = _destroy_rdma_controller,
};

#endif // XNVME_BE_NVMF_ENABLED
