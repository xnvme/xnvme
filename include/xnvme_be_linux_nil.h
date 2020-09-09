// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_NWRP_H
#define __INTERNAL_XNVME_BE_NWRP_H

#define XNVME_BE_NWRP_CTX_DEPTH_MAX 23

struct xnvme_async_ctx_nwrp {
	uint32_t depth;		///< IO depth
	uint32_t outstanding;	///< Outstanding IO on the context/ring/queue

	struct xnvme_req *reqs[XNVME_BE_NWRP_CTX_DEPTH_MAX];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_async_ctx_nwrp) == XNVME_BE_ACTX_NBYTES,
	"Incorrect size"
)

/**
 * Internal representation of XNVME_BE_NWRP state
 */
struct xnvme_be_nwrp_state {
	int fd;

	uint8_t pseudo;

	uint8_t poll_io;
	uint8_t poll_sq;

	uint8_t rsvd[121];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_be_nwrp_state) == XNVME_BE_STATE_NBYTES,
	"Incorrect size"
)

#endif /* __INTERNAL_XNVME_BE_NWRP */
