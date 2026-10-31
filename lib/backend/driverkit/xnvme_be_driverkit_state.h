// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_DRIVERKIT_STATE_H
#define __INTERNAL_XNVME_BE_DRIVERKIT_STATE_H
#include <xnvme_be.h>

struct buffer;
struct xnvme_be_driverkit_ctrlr;

/**
 * Per-device state embedded in xnvme_dev.be.state.
 * The first field (ctrlr) is the cref handle written by the platform.
 */
struct xnvme_be_driverkit_state {
	struct xnvme_be_driverkit_ctrlr *ctrlr; ///< Shared controller

	uint64_t qid_sync;

	struct buffer *buffers;

	uint8_t _rsvd[104];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_driverkit_state) == XNVME_BE_STATE_NBYTES,
		    "Incorrect size")
XNVME_BE_CTRLR_STATE_FIRST(xnvme_be_driverkit_state);

#endif /* __INTERNAL_XNVME_BE_DRIVERKIT_STATE_H */
