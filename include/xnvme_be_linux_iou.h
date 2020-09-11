// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_LINUX_IOU_H
#define __INTERNAL_XNVME_BE_LINUX_IOU_H
#include <liburing.h>

struct xnvme_async_ctx_linux_iou {
	uint32_t depth;		///< IO depth
	uint32_t outstanding;	///< Outstanding IO on the context/ring/queue

	struct io_uring ring;

	uint8_t poll_io;
	uint8_t poll_sq;

	uint8_t _rsvd[14];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_async_ctx_linux_iou) == XNVME_BE_ACTX_NBYTES,
	"Incorrect size"
)

int
xnvme_be_linux_iou_check_support(void);

#endif /* __INTERNAL_XNVME_BE_LINUX_IOU */
