// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_LIBURING_H
#define __INTERNAL_XNVME_BE_LIBURING_H
#include <libxnvme.h>
#include <xnvme_be.h>

extern struct xnvme_be_async g_xnvme_be_liburing_async;
extern struct xnvme_be_async g_xnvme_be_liburing_async_ucmd;

#ifdef XNVME_BE_LIBURING_ENABLED
#include <liburing.h>
#include <xnvme_queue.h>

#define XNVME_QUEUE_IOU_CQE_BATCH_MAX 8
#define XNVME_QUEUE_IOU_BIGSQE        (0x1 << 2)

struct xnvme_queue_liburing {
	struct xnvme_queue_base base;

	struct io_uring ring;

	uint8_t poll_io;
	uint8_t poll_sq;
	uint8_t batching;
	int efd; // Completion event FD

	uint8_t _rsvd[5];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_liburing) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

int
xnvme_be_liburing_init(struct xnvme_queue *queue, int opts);

int
xnvme_be_liburing_term(struct xnvme_queue *queue);

int
xnvme_be_liburing_get_completion_fd(struct xnvme_queue *queue);
#endif

#endif /* __INTERNAL_XNVME_BE_LIBURING_H */
