// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_QUEUE_H
#define __INTERNAL_XNVME_QUEUE_H
#include <sys/queue.h>

/**
 * Internal command-context representation, the difference between this and the 'struct
 * xnvme_cmd_ctx' in the public API, are that the last eight bytes of 'struct xnvme_cmd_ctx_entry'
 * has a SLIST_ENTRY link. This is not provided in the public API to avoid issues with the
 * availability of queue.h on various platforms, when consuming the xNVMe headers.
 */
struct xnvme_cmd_ctx_entry {
	struct xnvme_spec_cmd cmd;
	struct xnvme_spec_cpl cpl;

	struct xnvme_dev *dev;

	struct {
		struct xnvme_queue *queue;
		xnvme_queue_cb cb;
		void *cb_arg;
	} async;

	uint32_t opts;

	uint8_t be_rsvd[4];

	SLIST_ENTRY(xnvme_cmd_ctx_entry) link;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_ctx_entry) == 128, "Incorrect size")

struct xnvme_queue_base {
	struct xnvme_dev *dev; ///< Device on which the queue operates
	uint32_t capacity;     ///< Maximum number of outstanding commands
	uint32_t outstanding;  ///< Number of currently outstanding commands
	SLIST_HEAD(, xnvme_cmd_ctx_entry) pool;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_base) == 24, "Incorrect size")

struct xnvme_queue {
	struct xnvme_queue_base base;

	uint8_t be_rsvd[232]; ///< Auxilary backend data

	struct xnvme_cmd_ctx_entry pool_storage[];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue) == XNVME_BE_QUEUE_STATE_NBYTES, "Incorrect size")

#endif /* __INTERNAL_XNVME_QUEUE_H */
