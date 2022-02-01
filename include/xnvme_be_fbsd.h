// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_FBSD_H
#define __INTERNAL_XNVME_BE_FBSD_H

#define XNVME_BE_FBSD_CTRLR_PREFIX "nvme"
#define XNVME_BE_FBSD_NS_PREFIX    "ns"

/**
 * Internal representation of XNVME_BE_FBSD state
 */
struct xnvme_be_fbsd_state {
	struct {
		int ns;
		int ctrlr;
	} fd;

	uint8_t _rsvd[120];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_fbsd_state) == XNVME_BE_STATE_NBYTES, "Incorrect size")

int
xnvme_be_fbsd_nvme_get_nsid_and_ctrlr_fd(int fd, uint32_t *nsid, int *ctrlr_fd);

extern struct xnvme_be_admin g_xnvme_be_fbsd_admin_nvme;
extern struct xnvme_be_sync g_xnvme_be_fbsd_sync_nvme;
extern struct xnvme_be_sync g_xnvme_be_fbsd_sync_psync;
extern struct xnvme_be_dev g_xnvme_be_fbsd_dev;

#endif /* __INTERNAL_XNVME_BE_FBSD */
