// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_SPDK_H
#define __INTERNAL_XNVME_BE_SPDK_H
#include <pthread.h>
#include <spdk/stdinc.h>
#include <spdk/env.h>
#include <spdk/nvme.h>

#define XNVME_BE_SPDK_QPAIR_MAX 64
#define XNVME_BE_SPDK_ALIGN 0x1000

/**
 * Internal representation of XNVME_BE_SPDK state
 */
struct xnvme_be_spdk {
	struct xnvme_be be;
	struct xnvme_ident ident;

	struct spdk_nvme_transport_id trid;
	struct spdk_env_opts opts;
	struct spdk_nvme_ctrlr *ctrlr;
	struct spdk_nvme_ctrlr_data ctrlr_data;

	struct spdk_nvme_ns *ns;
	struct spdk_nvme_ns_data ns_data;
	int attached;

	int vam_outstanding;		///< Outstanding SYNC ADMIN commands
	struct spdk_nvme_qpair *qpair;	///< QPAIR for SYNC IO commands
	pthread_mutex_t qpair_lock;	///< LOCK for SYNC IO commands
};

#endif /* __INTERNAL_XNVME_BE_SPDK */
