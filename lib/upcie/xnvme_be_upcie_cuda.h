// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_CUDA_H
#define __INTERNAL_XNVME_BE_UPCIE_CUDA_H
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_be.h>

#include <xnvme_be_upcie.h>
#include <upcie/upcie_cuda.h>

/**
 * State used across multiple instances of controllers/namespaces
 */
struct xnvme_be_upcie_cuda_rte {
	CUcontext cu_ctx;
	struct cudamem_config cuda_config;
	struct cudamem_heap cuda_heap;
	int is_initialized;
	struct cudamem_mapping *mappings;
};

extern struct xnvme_be_upcie_cuda_rte g_upcie_cuda_rte;

extern struct xnvme_be_mem g_xnvme_be_upcie_cuda_mem;
extern struct xnvme_be_admin g_xnvme_be_upcie_cuda_admin;
extern struct xnvme_be_sync g_xnvme_be_upcie_cuda_sync;
extern struct xnvme_be_async g_xnvme_be_upcie_cuda_async;
extern struct xnvme_be_dev g_xnvme_be_upcie_cuda_dev;

#endif /* XNVME_BE_UPCIE_CUDA_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_CUDA_H */
