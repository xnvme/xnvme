// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <libaio.h>

#ifndef __INTERNAL_XNVME_BE_LINUX_AIO_H
#define __INTERNAL_XNVME_BE_LINUX_AIO_H

struct xnvme_async_ctx_aio {
	uint32_t depth;         ///< IO depth
	uint32_t outstanding;   ///< Outstanding IO on the context/ring/queue

	io_context_t aio_ctx;
	struct io_event *aio_events;
	struct iocb **iocbs;
	struct iocb iocb;

	uint32_t entries;
	uint32_t queued;
	uint32_t head;
	uint32_t tail;

	uint8_t rsvd[76];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_async_ctx_aio) == XNVME_BE_ACTX_NBYTES,
	"Incorrect size"
)

#endif /* __INTERNAL_XNVME_BE_LINUX_AIO */
