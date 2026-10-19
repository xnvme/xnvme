// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_RAMDISK_STATE_H
#define __INTERNAL_XNVME_BE_RAMDISK_STATE_H
#include <xnvme_be.h>

struct xnvme_be_ramdisk_state {
	void *ramdisk;

	uint8_t _rsvd[120];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_ramdisk_state) == XNVME_BE_STATE_NBYTES,
		    "Incorrect size");

#endif /* __INTERNAL_XNVME_BE_RAMDISK_STATE_H */
