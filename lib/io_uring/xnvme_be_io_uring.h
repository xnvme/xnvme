// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_IO_URING_H
#define __INTERNAL_XNVME_BE_IO_URING_H
#include <liburing.h>

#define XNVME_QUEUE_IOU_CQE_BATCH_MAX 8
#define XNVME_QUEUE_IOU_BIGSQE        (0x1 << 2)

struct xnvme_queue_io_uring {
	struct xnvme_queue_base base;

	struct io_uring ring;

	uint8_t poll_io;
	uint8_t poll_sq;
	uint8_t batching;
	int efd; // Completion event FD

	uint8_t _rsvd[5];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_io_uring) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

int
xnvme_be_io_uring_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			 size_t mbuf_nbytes);

int
xnvme_be_io_uring_poke(struct xnvme_queue *queue, uint32_t max);

int
xnvme_be_io_uring_wait(struct xnvme_queue *queue);

int
xnvme_be_io_uring_init(struct xnvme_queue *queue, int opts);

int
xnvme_be_io_uring_term(struct xnvme_queue *queue);

int
xnvme_be_io_uring_get_completion_fd(struct xnvme_queue *queue);

#endif /* __INTERNAL_XNVME_BE_IO_URING_H */
