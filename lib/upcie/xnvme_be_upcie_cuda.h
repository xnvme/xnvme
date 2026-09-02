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

/**
 * State used across multiple instances of controllers/namespaces
 */
struct xnvme_be_upcie_ctrlr;

struct xnvme_be_upcie_cuda_rte {
	CUcontext cu_ctx;
	struct cudamem_config cuda_config;
	struct cudamem_heap cuda_heap;
	struct dmamem dmem; ///< Shared translation; unused where each controller needs its own
	int dmem_is_shared; ///< dmem describes the heap for every controller

	/* One heap serves every controller this process opens, but what makes it
	 * reachable does not: a mapping is installed into one controller's
	 * domain and doorbells are one controller's BAR. So that part is kept
	 * per controller and looked up by the controller a queue is for. */
	struct xnvme_be_upcie_cuda_ctrlr {
		struct xnvme_be_upcie_ctrlr *ctrlr; ///< NULL when the slot is free

		uint64_t reg_offset; ///< Where the server left its description

		/* Where the GPU finds the doorbells. Not necessarily the
		 * mapping this process rings them through: the driver has to
		 * resolve it to a physical address to put it in front of the
		 * GPU, and it cannot do that for every kind of mapping. */
		void *db_base;
		void *db_page;    ///< The registered page within it
		void *db_own_map; ///< Non-NULL when db_base is ours to unmap
		size_t db_own_nbytes;
	} ctrlrs[XNVME_BE_UPCIE_GPU_CTRLRS_MAX];

	int is_initialized;
};

extern struct xnvme_be_upcie_cuda_rte g_upcie_cuda_rte;

/**
 * The slot holding what was set up for `ctrlr`, or NULL if it has none
 */
static inline struct xnvme_be_upcie_cuda_ctrlr *
_cuda_ctrlr_slot_of(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	for (int i = 0; i < XNVME_BE_UPCIE_GPU_CTRLRS_MAX; ++i) {
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
