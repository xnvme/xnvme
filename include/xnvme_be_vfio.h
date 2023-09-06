// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_VFIO_H
#define __INTERNAL_XNVME_BE_VFIO_H
#include <errno.h>
#include <pthread.h>

#include <vfn/nvme.h>

#include <xnvme_be.h>
#include <xnvme_queue.h>

struct xnvme_be_vfio_state {
	struct nvme_ctrl *ctrl;
	unsigned long long qidmap; // Queue identifier bit map

	struct nvme_sq *sq_sync; // Submission queue for synchronous IOs
	struct nvme_cq *cq_sync; // Completion queue for synchronous IOs

	uint8_t _rsvd[94];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_vfio_state) == XNVME_BE_STATE_NBYTES, "Incorrect size")

int
_xnvme_be_vfio_create_ioqpair(struct xnvme_be_vfio_state *state, int qd, int flags);
int
_xnvme_be_vfio_delete_ioqpair(struct xnvme_be_vfio_state *state, unsigned int qid);

extern struct xnvme_be_admin g_xnvme_be_vfio_admin;
extern struct xnvme_be_sync g_xnvme_be_vfio_sync;
extern struct xnvme_be_async g_xnvme_be_vfio_async;
extern struct xnvme_be_mem g_xnvme_be_vfio_mem;
extern struct xnvme_be_dev g_xnvme_be_vfio_dev;

#endif /* __INTERNAL_XNVME_BE_VFIO */
