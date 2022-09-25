// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_LINUX_LIBAIO_H
#define __INTERNAL_XNVME_BE_LINUX_LIBAIO_H
#include <libaio.h>

struct xnvme_queue_libaio {
	struct xnvme_queue_base base;

	io_context_t aio_ctx;
	struct io_event *aio_events;

	uint8_t poll_io;
	uint8_t rsvd[212];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_libaio) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

#endif /* __INTERNAL_XNVME_BE_LINUX_LIBAIO_H */
