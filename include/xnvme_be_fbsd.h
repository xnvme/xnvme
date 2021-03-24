// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_FBSD_H
#define __INTERNAL_XNVME_BE_FBSD_H

#define XNVME_FREEBSD_CTRLR_SCAN _PATH_DEV "nvme%1u%[^\n]"
#define XNVME_FREEBSD_NS_SCAN _PATH_DEV "nvme%1uns%1u%[^\n]"

#define XNVME_FREEBSD_CTRLR_FMT _PATH_DEV "nvme%1u"
#define XNVME_FREEBSD_NS_FMT _PATH_DEV "nvme%1uns%1u"

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

extern struct xnvme_be_dev g_xnvme_be_fbsd_dev;

#endif /* __INTERNAL_XNVME_BE_FBSD */
