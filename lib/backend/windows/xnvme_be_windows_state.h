// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_WINDOWS_STATE_H
#define __INTERNAL_XNVME_BE_WINDOWS_STATE_H
#include <xnvme_be.h>

/**
 * Internal representation of XNVME_BE_WINDOWS_STATE
 */
struct xnvme_be_windows_state {
	void *sync_handle;
	void *async_handle;
	int fd;
	uint8_t _rsvd[108]; ///< reserved space
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_windows_state) == XNVME_BE_STATE_NBYTES,
		    "Incorrect size")

#endif /* __INTERNAL_XNVME_BE_WINDOWS_STATE_H */
