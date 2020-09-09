// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_LIOU_H
#define __INTERNAL_XNVME_BE_LIOU_H

struct xnvme_async_ctx_liou {
	uint32_t depth;		///< IO depth
	uint32_t outstanding;	///< Outstanding IO on the context/ring/queue

	struct io_uring ring;

	uint8_t poll_io;
	uint8_t poll_sq;

	uint8_t _rsvd[14];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_async_ctx_liou) == XNVME_BE_ACTX_NBYTES,
	"Incorrect size"
)

/**
 * Internal representation of XNVME_BE_LIOU state
 */
struct xnvme_be_liou_state {
	int fd;

	uint8_t pseudo;

	uint8_t poll_io;
	uint8_t poll_sq;

	uint8_t rsvd[121];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_be_liou_state) == XNVME_BE_STATE_NBYTES,
	"Incorrect size"
)

#endif /* __INTERNAL_XNVME_BE_LIOU */
