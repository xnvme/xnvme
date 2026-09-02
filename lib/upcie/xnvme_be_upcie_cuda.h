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
	struct dmamem dmem; ///< Registry wrapped for translation; allocation stays on the heap

	/* Where the server left its description of this heap, when the
	 * controller belongs to one. Zero when this process owns it. */
	uint64_t reg_offset;

	/* The controller the registration was made against, so it can be handed
	 * back while the connection carrying it is still open. */
	struct xnvme_be_upcie_ctrlr *reg_ctrlr;

	/* Installs a mapping per allocation where the IOMMU translates for the
	 * controller, since what the heap knows are physical addresses and the
	 * IOMMU will not take those. Left closed where the mode needs none. */
	struct dmamem_iommu_map_pa imp;
	int imp_open;

	/* Where the GPU finds the doorbells. Not necessarily the mapping this
	 * process rings them through: the kernel has to resolve the mapping to
	 * a physical address to put it in front of the GPU, and it cannot do
	 * that for every kind of mapping. NULL until one is registered. */
	void *db_base;
	void *db_page;    ///< The registered page within it
	void *db_own_map; ///< Non-NULL when db_base is a mapping to undo
	size_t db_own_nbytes;

	int is_initialized;
};

extern struct xnvme_be_upcie_cuda_rte g_upcie_cuda_rte;

extern struct xnvme_be_mem g_xnvme_be_upcie_cuda_mem;
extern struct xnvme_be_dev g_xnvme_be_upcie_cuda_dev;

#endif /* XNVME_BE_UPCIE_CUDA_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_CUDA_H */
