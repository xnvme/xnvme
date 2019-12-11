// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_FIOC_H
#define __INTERNAL_XNVME_BE_FIOC_H

/**
 * Internal representation of XNVME_BE_FIOC state
 */
struct xnvme_be_fioc {
	struct xnvme_be be;

	int fd;
};

#endif /* __INTERNAL_XNVME_BE_FIOC */
