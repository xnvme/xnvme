// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_STATE_H
#define __INTERNAL_XNVME_BE_UPCIE_STATE_H
#include <xnvme_be.h>

struct dmamem;
struct xnvme_be_upcie_ctrlr;
struct xnvme_be_upcie_iova_range_dmem;

/**
 * Per-device state embedded in xnvme_dev.be.state.
 *
 * The first field (ctrlr) is the cref handle written by the platform. The
 * second says where data buffers for this device live: the host heap for
 * `upcie`, the GPU heap for `upcie-cuda` and `upcie-hip`, assigned by the
 * backend's dev_open. That is what lets the admin, sync and async paths be
 * shared across the three, since they differ only in which dmamem a payload
 * translates through.
 */
struct xnvme_be_upcie_state {
	struct xnvme_be_upcie_ctrlr *ctrlr;           ///< Shared controller
	struct dmamem *dmem;                          ///< Where this device's data buffers live
	struct xnvme_be_upcie_iova_range_dmem *range; ///< Owned; NULL where no range is needed

	uint8_t _rvds[104];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_upcie_state) == XNVME_BE_STATE_NBYTES, "Incorrect size")
XNVME_BE_CTRLR_STATE_FIRST(xnvme_be_upcie_state);

#endif /* __INTERNAL_XNVME_BE_UPCIE_STATE_H */
