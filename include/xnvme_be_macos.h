// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_MACOS_H
#define __INTERNAL_XNVME_BE_MACOS_H

/**
 * Internal representation of XNVME_BE_MACOS state
 */
struct xnvme_be_macos_state {
	int fd;

	uint8_t _rsvd[124];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_macos_state) == XNVME_BE_STATE_NBYTES, "Incorrect size")

extern struct xnvme_be_sync g_xnvme_be_macos_sync_psync;

#endif /* __INTERNAL_XNVME_BE_MACOS */
