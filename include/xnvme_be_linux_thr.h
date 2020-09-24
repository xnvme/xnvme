// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_LINUX_THR_H
#define __INTERNAL_XNVME_BE_LINUX_THR_H

struct _entry {
	struct xnvme_spec_cmd cmd;
	struct xnvme_dev *dev;
	void *dbuf;
	size_t dbuf_nbytes;
	void *mbuf;
	size_t mbuf_nbytes;
	struct xnvme_req *req;
	STAILQ_ENTRY(_entry) link;
};

struct _qp {
	STAILQ_HEAD(, _entry) rp;	///< Request pool
	STAILQ_HEAD(, _entry) sq;	///< Submission queue
	STAILQ_HEAD(, _entry) cq;	///< Completion queue
	uint32_t capacity;
	struct _entry elm[];
};

struct xnvme_async_ctx_thr {
	uint32_t depth;		///< IO depth
	uint32_t outstanding;	///< Outstanding IO on the context/ring/qp

	struct _qp *qp;

	uint8_t _rsvd[176];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_async_ctx_thr) == XNVME_BE_ACTX_NBYTES,
	"Incorrect size"
)

#endif /* __INTERNAL_XNVME_BE_LINUX_THR */
