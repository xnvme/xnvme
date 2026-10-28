// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_HIP_H
#define __INTERNAL_XNVME_BE_UPCIE_HIP_H
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <xnvme_be.h>

#include <xnvme_be_upcie.h>
#include <upcie/upcie_hip.h>

struct xnvme_be_upcie_ctrlr;

struct xnvme_be_upcie_hip_ctrlr {
	struct xnvme_be_upcie_ctrlr *ctrlr; ///< NULL when the slot is free
	uint64_t reg_offset;
};

/**
 * State used across multiple instances of controllers/namespaces
 */
struct xnvme_be_upcie_hip_rte {
	struct hipmem_config hip_config;
	struct hipmem_heap hip_heap;
	struct dmamem dmem; ///< Shared translation; unused where each controller needs its own
	int dmem_is_shared;
	int dmem_in_range;
	struct xnvme_be_upcie_hip_ctrlr ctrlrs[XNVME_BE_CREF_MAX_ENTRIES];

	int is_initialized;
};

extern struct xnvme_be_upcie_hip_rte g_upcie_hip_rte;

extern struct xnvme_be_mem g_xnvme_be_upcie_hip_mem;
extern struct xnvme_be_dev g_xnvme_be_upcie_hip_dev;

#endif /* XNVME_BE_UPCIE_HIP_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_HIP_H */
