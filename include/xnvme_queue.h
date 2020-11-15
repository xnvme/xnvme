// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_QUEUE_H
#define __INTERNAL_XNVME_QUEUE_H

struct xnvme_queue_base {
	uint32_t depth;		///< IO depth
	uint32_t outstanding;	///< Outstanding IO on the context/ring/queue
	struct xnvme_dev *dev;  ///< Device on which the queue operates
};

struct xnvme_queue {
	struct xnvme_queue_base base;

	uint8_t be_rsvd[176];	///< Auxilary backend data
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue) == 192, "Incorrect size")

#endif /* __INTERNAL_XNVME_QUEUE_H */
