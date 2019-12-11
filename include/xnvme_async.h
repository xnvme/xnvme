// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_ASYNC_H
#define __INTERNAL_XNVME_ASYNC_H

struct xnvme_async_ctx {
	uint32_t depth;		///< IO depth
	uint32_t outstanding;	///< Outstanding IO on the context/ring/queue

	uint8_t be_rsvd[184];	///< Auxilary backend data
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_async_ctx) == 192, "Incorrect size")

#endif /* __INTERNAL_XNVME_ASYNC_H */
