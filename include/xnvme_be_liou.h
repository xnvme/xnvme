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

	uint8_t rsvd[22];
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

	uint8_t poll_io;
	uint8_t poll_sq;
	uint8_t pseudo;

	uint8_t rsvd[121];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_be_liou_state) == XNVME_BE_STATE_NBYTES,
	"Incorrect size"
)

int
xnvme_be_liou_async_init(struct xnvme_dev *dev, struct xnvme_async_ctx **ctx,
			 uint16_t depth);

int
xnvme_be_liou_async_term(struct xnvme_async_ctx *ctx);

int
xnvme_be_liou_async_poke(struct xnvme_async_ctx *ctx,
			 uint32_t max);

int
xnvme_be_liou_async_wait(struct xnvme_async_ctx *ctx);

void *
xnvme_be_liou_buf_alloc(const struct xnvme_dev *dev, size_t nbytes,
			uint64_t *phys);

void
xnvme_be_liou_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf);

int
xnvme_be_liou_cmd_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
		       void *dbuf, size_t dbuf_nbytes, void *mbuf,
		       size_t mbuf_nbytes, int opts, struct xnvme_req *req);

void
xnvme_be_liou_dev_close(struct xnvme_dev *dev);

#endif /* __INTERNAL_XNVME_BE_LIOU */
