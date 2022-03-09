// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_QUEUE_H
#define __INTERNAL_XNVME_QUEUE_H

struct xnvme_queue_base {
	struct xnvme_dev *dev; ///< Device on which the queue operates
	uint32_t capacity;     ///< Maximum number of outstanding commands
	uint32_t outstanding;  ///< Number of currently outstanding commands
	SLIST_HEAD(, xnvme_cmd_ctx) pool;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_base) == 24, "Incorrect size")

struct xnvme_queue {
	struct xnvme_queue_base base;

	uint8_t be_rsvd[232]; ///< Auxilary backend data

	struct xnvme_cmd_ctx pool_storage[];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue) == XNVME_BE_QUEUE_STATE_NBYTES, "Incorrect size")

#endif /* __INTERNAL_XNVME_QUEUE_H */
