// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_CUDA_H
#define __INTERNAL_XNVME_BE_UPCIE_CUDA_H
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_be.h>

#include <xnvme_be_upcie.h>
#include <upcie/upcie_cuda.h>

struct xnvme_cuda_queue {
	struct nvme_qpair_cuda qpair;
};

struct xnvme_be_upcie_ctrlr;

struct xnvme_be_upcie_cuda_ctrlr {
	struct xnvme_be_upcie_ctrlr *ctrlr; ///< NULL when the slot is free
	uint64_t reg_offset;
	void *db_base;
	void *db_page;
	void *db_own_map; ///< Non-NULL when db_base is ours to unmap
	size_t db_own_nbytes;
};

/**
 * State used across multiple instances of controllers/namespaces
 */
struct xnvme_be_upcie_cuda_rte {
	CUcontext cu_ctx;
	CUdevice cu_dev;
	int cu_ctx_retained;

	struct cudamem_config cuda_config;
	struct cudamem_heap cuda_heap;
	struct dmamem dmem; ///< Shared translation; unused where each controller needs its own
	int dmem_is_shared;
	int dmem_in_range;
	struct xnvme_be_upcie_cuda_ctrlr ctrlrs[XNVME_BE_CREF_MAX_ENTRIES];

	int is_initialized;
};

extern struct xnvme_be_upcie_cuda_rte g_upcie_cuda_rte;

static inline struct xnvme_be_upcie_cuda_ctrlr *
_cuda_ctrlr_slot_of(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	for (int i = 0; i < XNVME_BE_CREF_MAX_ENTRIES; ++i) {
		if (g_upcie_cuda_rte.ctrlrs[i].ctrlr == ctrlr) {
			return &g_upcie_cuda_rte.ctrlrs[i];
		}
	}

	return NULL;
}

extern struct xnvme_be_mem g_xnvme_be_upcie_cuda_mem;
extern struct xnvme_be_dev g_xnvme_be_upcie_cuda_dev;

#endif /* XNVME_BE_UPCIE_CUDA_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_CUDA_H */
