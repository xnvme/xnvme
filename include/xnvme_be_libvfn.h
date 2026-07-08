// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_LIBVFN_H
#define __INTERNAL_XNVME_BE_LIBVFN_H
#include <errno.h>
#include <pthread.h>

#include <vfn/nvme.h>

#include <xnvme_be.h>
#include <xnvme_queue.h>

#define XNVME_BE_LIBVFN_NQUEUES_MAX 64

/**
 * Shared controller state, one per physical controller, managed by cref.
 */
struct xnvme_be_libvfn_ctrlr {
	struct nvme_ctrl *ctrl;
	unsigned long long qidmap; ///< Queue identifier bit map
	int *efds;                 ///< Completion event FDs (one per IRQ vector)
	int nefds;                 ///< Number of completion event FDs

	struct nvme_sq *sq_sync; ///< Shared submission queue for synchronous IOs
	struct nvme_cq *cq_sync; ///< Shared completion queue for synchronous IOs
};

/**
 * Per-device state embedded in xnvme_dev.be.state.
 * The first field (ctrlr) is the cref handle written by the platform.
 */
struct xnvme_be_libvfn_state {
	struct xnvme_be_libvfn_ctrlr *ctrlr; ///< Shared controller (first field for platform)

	uint8_t _rvds[120];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_libvfn_state) == XNVME_BE_STATE_NBYTES,
		    "Incorrect size")

int
_xnvme_be_libvfn_create_ioqpair(struct xnvme_be_libvfn_state *state, int qd, int flags);
int
_xnvme_be_libvfn_delete_ioqpair(struct xnvme_be_libvfn_state *state, unsigned int qid);

extern struct xnvme_be_admin g_xnvme_be_libvfn_admin;
extern struct xnvme_be_sync g_xnvme_be_libvfn_sync;
extern struct xnvme_be_async g_xnvme_be_libvfn_async;
extern struct xnvme_be_mem g_xnvme_be_libvfn_mem;
extern struct xnvme_be_dev g_xnvme_be_libvfn_dev;

#endif /* __INTERNAL_XNVME_BE_LIBVFN */
