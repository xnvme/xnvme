// Copyright (C) Mads Ynddal <m.ynddal@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_RAMDISK_H
#define __INTERNAL_XNVME_BE_RAMDISK_H
struct xnvme_be_ramdisk_state {
	void *ramdisk;

	uint8_t _rsvd[120];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_ramdisk_state) == XNVME_BE_STATE_NBYTES,
		    "Incorrect size");

int
xnvme_be_ramdisk_supported(struct xnvme_dev *dev, uint32_t opts);

size_t
_xnvme_be_ramdisk_dev_get_size(struct xnvme_dev *dev);

extern struct xnvme_be_admin g_xnvme_be_ramdisk_admin;
extern struct xnvme_be_sync g_xnvme_be_ramdisk_sync;
extern struct xnvme_be_mem g_xnvme_be_ramdisk_mem;
extern struct xnvme_be_dev g_xnvme_be_ramdisk_dev;

#endif /* __INTERNAL_XNVME_BE_RAMDISK_H */
