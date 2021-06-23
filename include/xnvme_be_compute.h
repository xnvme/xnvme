// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// Copyright (C) Niclas Hedam <n.hedam@samsung.com, nhed@itu.dk>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_COMPUTE_H
#define __INTERNAL_XNVME_BE_COMPUTE_H
struct xnvme_be_compute_state {
	int fd;
	int slot;

	char rsv[120];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_be_compute_state) == XNVME_BE_STATE_NBYTES,
	"Incorrect size"
);

extern struct xnvme_be_sync g_xnvme_be_compute_sync;
extern struct xnvme_be_dev g_xnvme_be_compute_dev;

#endif /* __INTERNAL_XNVME_BE_COMPUTE_H */
