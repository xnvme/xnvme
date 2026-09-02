// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_HIP_H
#define __INTERNAL_XNVME_BE_UPCIE_HIP_H
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <xnvme_be.h>

#include <xnvme_be_upcie.h>
#include <upcie/upcie_hip.h>

/**
 * State used across multiple instances of controllers/namespaces
 */
struct xnvme_be_upcie_ctrlr;

struct xnvme_be_upcie_hip_rte {
	struct hipmem_config hip_config;
	struct hipmem_heap hip_heap;
	struct dmamem dmem; ///< Registry wrapped for translation; allocation stays on the heap

	/* Where the server left its description of this heap, when the
	 * controller belongs to one. Zero when this process owns it. */
	uint64_t reg_offset;

	/* The controller the registration was made against, so it can be handed
	 * back while the connection carrying it is still open. */
	struct xnvme_be_upcie_ctrlr *reg_ctrlr;

	int is_initialized;
};

extern struct xnvme_be_upcie_hip_rte g_upcie_hip_rte;

extern struct xnvme_be_mem g_xnvme_be_upcie_hip_mem;
extern struct xnvme_be_dev g_xnvme_be_upcie_hip_dev;

#endif /* XNVME_BE_UPCIE_HIP_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_HIP_H */
