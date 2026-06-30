// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_HIP_H
#define __INTERNAL_XNVME_BE_UPCIE_HIP_H
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <xnvme_be.h>

#include <xnvme_be_upcie.h>
#include <upcie/upcie_hip.h>

struct xnvme_hip_queue {
	struct nvme_qpair_hip qpair;
};

/**
 * State used across multiple instances of controllers/namespaces
 */
struct xnvme_be_upcie_hip_rte {
	struct hipmem_config hip_config;
	struct hipmem_heap hip_heap;
	int is_initialized;
};

extern struct xnvme_be_upcie_hip_rte g_upcie_hip_rte;

extern struct xnvme_be_mem g_xnvme_be_upcie_hip_mem;
extern struct xnvme_be_admin g_xnvme_be_upcie_hip_admin;
extern struct xnvme_be_sync g_xnvme_be_upcie_hip_sync;
extern struct xnvme_be_async g_xnvme_be_upcie_hip_async;
extern struct xnvme_be_dev g_xnvme_be_upcie_hip_dev;

#endif /* XNVME_BE_UPCIE_HIP_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_HIP_H */
