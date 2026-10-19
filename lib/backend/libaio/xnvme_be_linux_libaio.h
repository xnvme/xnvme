// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_LINUX_LIBAIO_H
#define __INTERNAL_XNVME_BE_LINUX_LIBAIO_H
#include <libaio.h>

struct xnvme_queue_libaio {
	struct xnvme_queue_base base;

	io_context_t aio_ctx;
	struct io_event *aio_events;

	uint8_t poll_io;
	int efd; // Completion event FD

	uint8_t rsvd[204];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_libaio) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

#endif /* __INTERNAL_XNVME_BE_LINUX_LIBAIO_H */
