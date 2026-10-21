// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_LIBVFN_STATE_H
#define __INTERNAL_XNVME_BE_LIBVFN_STATE_H
#include <xnvme_be.h>

struct xnvme_be_libvfn_ctrlr;

/**
 * Per-device state embedded in xnvme_dev.be.state.
 * The first field (ctrlr) is the cref handle written by the platform.
 */
struct xnvme_be_libvfn_state {
	struct xnvme_be_libvfn_ctrlr *ctrlr; ///< Shared controller

	uint8_t _rvds[120];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_libvfn_state) == XNVME_BE_STATE_NBYTES,
		    "Incorrect size")
XNVME_BE_CTRLR_STATE_FIRST(xnvme_be_libvfn_state);

#endif /* __INTERNAL_XNVME_BE_LIBVFN_STATE_H */
