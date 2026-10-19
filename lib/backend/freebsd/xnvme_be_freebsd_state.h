// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_FREEBSD_STATE_H
#define __INTERNAL_XNVME_BE_FREEBSD_STATE_H
#include <xnvme_be.h>
#include <xnvme_be_posix_state.h>

/**
 * Internal representation of XNVME_BE_FREEBSD state
 */
struct xnvme_be_freebsd_state {
	struct xnvme_be_posix_state posix; ///< The namespace
	int ctrlr_fd;

	uint8_t poll_io;

	uint8_t _rsvd[119];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_freebsd_state) == XNVME_BE_STATE_NBYTES,
		    "Incorrect size")
XNVME_BE_POSIX_STATE_FIRST(xnvme_be_freebsd_state);

#endif /* __INTERNAL_XNVME_BE_FREEBSD_STATE_H */
