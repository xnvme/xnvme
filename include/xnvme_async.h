// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_ASYNC_H
#define __INTERNAL_XNVME_ASYNC_H

struct xnvme_async_ctx {
	uint32_t depth;		///< IO depth of the ASYNC CTX
	uint32_t outstanding;	///< Outstanding IO on the ASYNC CTX

	// Lower-layer context, e.g. for the implementation of xnvme_be_*_async_*
	void *be_ctx;
};

#endif /* __INTERNAL_XNVME_ASYNC_H */
