// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_LINUX_THR_H
#define __INTERNAL_XNVME_BE_LINUX_THR_H

struct _entry {
	struct xnvme_dev *dev;
	struct xnvme_cmd_ctx *ctx;
	void *dbuf;
	size_t dbuf_nbytes;
	void *mbuf;
	size_t mbuf_nbytes;
	STAILQ_ENTRY(_entry) link;
};

struct _qp {
	STAILQ_HEAD(, _entry) rp;	///< Request pool
	STAILQ_HEAD(, _entry) sq;	///< Submission queue
	STAILQ_HEAD(, _entry) cq;	///< Completion queue
	uint32_t capacity;
	struct _entry elm[];
};

struct xnvme_queue_thr {
	struct xnvme_queue_base base;

	struct _qp *qp;

	uint8_t _rsvd[224];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_queue_thr) == XNVME_BE_QUEUE_STATE_NBYTES,
	"Incorrect size"
)

#endif /* __INTERNAL_XNVME_BE_LINUX_THR */
