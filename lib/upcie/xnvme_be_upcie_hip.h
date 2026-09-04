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
	struct dmamem dmem; ///< Shared translation; unused where each controller needs its own
	int dmem_is_shared; ///< dmem describes the heap for every controller

	/* One heap serves every controller this process drives, and each has to
	 * be told about it separately: a registration is made against a single
	 * controller, and the addresses it answers with are only good there. */
	struct xnvme_be_upcie_hip_ctrlr {
		struct xnvme_be_upcie_ctrlr *ctrlr; ///< NULL when the slot is free

		/* Where the server left its description of this heap. Handed
		 * back while the connection carrying it is still open. */
		uint64_t reg_offset;
	} ctrlrs[XNVME_BE_UPCIE_GPU_CTRLRS_MAX];

	int is_initialized;
};

extern struct xnvme_be_upcie_hip_rte g_upcie_hip_rte;

extern struct xnvme_be_mem g_xnvme_be_upcie_hip_mem;
extern struct xnvme_be_dev g_xnvme_be_upcie_hip_dev;

#endif /* XNVME_BE_UPCIE_HIP_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_HIP_H */
