// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_LINUX_LIBURING_H
#define __INTERNAL_XNVME_BE_LINUX_LIBURING_H
#include <liburing.h>

#define XNVME_QUEUE_IOU_CQE_BATCH_MAX 8
#define XNVME_QUEUE_IOU_BIGSQE        (0x1 << 2)

struct xnvme_queue_liburing {
	struct xnvme_queue_base base;

	struct io_uring ring;

	uint8_t poll_io;
	uint8_t poll_sq;

	uint8_t _rsvd[14];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_liburing) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

int
xnvme_be_linux_liburing_check_support(void);

int
xnvme_be_linux_liburing_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			       void *mbuf, size_t mbuf_nbytes);

int
xnvme_be_linux_liburing_poke(struct xnvme_queue *queue, uint32_t max);

int
xnvme_be_linux_liburing_wait(struct xnvme_queue *queue);

int
xnvme_be_linux_liburing_init(struct xnvme_queue *queue, int opts);

int
xnvme_be_linux_liburing_term(struct xnvme_queue *queue);

#endif /* __INTERNAL_XNVME_BE_LINUX_LIBURING_H */
