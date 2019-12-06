// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_LIOU_H
#define __INTERNAL_XNVME_BE_LIOU_H

#define XNVME_BE_LIOU_VECS_LEN 100

/**
 * Internal representation of XNVME_BE_LIOU state
 */
struct xnvme_be_liou {
	struct xnvme_be be;

	int fd;

	struct iovec vecs[XNVME_BE_LIOU_VECS_LEN];	///< Registered buffers
	size_t nvecs;					///< Number of buffers
};

#endif /* __INTERNAL_XNVME_BE_LIOU */
