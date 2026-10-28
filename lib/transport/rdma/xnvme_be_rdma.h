// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_RDMA_H
#define __INTERNAL_XNVME_BE_RDMA_H

#include <xnvme_transport_base.h>

#define TO_RDMA_QUEUE(queue)       container_of(&(queue)->base, struct xnvme_be_rdma_queue, base)
#define TO_RDMA_QUEUE_QPAIR(queue) (TO_RDMA_QUEUE(queue)->qpair)

struct xnvme_transport_qpair;

struct xnvme_be_rdma_queue {
	struct xnvme_queue_base base;
	struct xnvme_transport_qpair *qpair;
	uint8_t be_rsvd[224]; ///< Auxilary backend data
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_rdma_queue) == sizeof(struct xnvme_queue),
		    "Incorrect size of xnvme_be_rdma_queue");

extern struct xnvme_be_admin g_xnvme_be_rdma_admin;
extern struct xnvme_be_async g_xnvme_be_rdma_async;
extern struct xnvme_be_dev g_xnvme_be_rdma_dev;
extern struct xnvme_be_mem g_xnvme_be_rdma_mem;
extern struct xnvme_be_sync g_xnvme_be_rdma_sync;

#endif /* __INTERNAL_XNVME_BE_RDMA_H */
