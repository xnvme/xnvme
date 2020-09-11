// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_FBSD_H
#define __INTERNAL_XNVME_BE_FBSD_H

/**
 * Internal representation of XNVME_BE_FBSD state
 */
struct xnvme_be_fbsd_state {
	int fd;

	uint8_t _rsvd[124];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_be_fbsd_state) == XNVME_BE_STATE_NBYTES,
	"Incorrect size"
)

#endif /* __INTERNAL_XNVME_BE_FBSD */
