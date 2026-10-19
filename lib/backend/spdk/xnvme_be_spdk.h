// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_SPDK_H
#define __INTERNAL_XNVME_BE_SPDK_H
#include <pthread.h>
#include <spdk/stdinc.h>
#include <spdk/env.h>
#include <spdk/nvme.h>
#include <xnvme_be.h>
#include <xnvme_queue.h>

#define XNVME_BE_SPDK_QPAIR_MAX 64
#define XNVME_BE_SPDK_ALIGN     0x1000

struct xnvme_queue_spdk {
	struct xnvme_queue_base base;

	struct spdk_nvme_qpair *qpair;

	struct xnvme_be_spdk_iov_payload *iov_payloads;

	uint8_t rsvd[216];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_spdk) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

XNVME_STATIC_ASSERT(sizeof(struct spdk_nvme_ctrlr_data) == sizeof(struct xnvme_spec_idfy_ctrlr),
		    "Incorrect size")
XNVME_STATIC_ASSERT(sizeof(struct spdk_nvme_ns_data) == sizeof(struct xnvme_spec_idfy_ns),
		    "Incorrect size")

#include <xnvme_be_spdk_state.h>

struct spdk_nvme_qpair *
xnvme_be_spdk_alloc_io_qpair(struct spdk_nvme_ctrlr *ctrlr,
			     const struct spdk_nvme_io_qpair_opts *opts, size_t opts_size);

extern struct xnvme_be_admin g_xnvme_be_spdk_admin;
extern struct xnvme_be_sync g_xnvme_be_spdk_sync;
extern struct xnvme_be_async g_xnvme_be_spdk_async;
extern struct xnvme_be_mem g_xnvme_be_spdk_mem;
extern struct xnvme_be_dev g_xnvme_be_spdk_dev;

#endif /* __INTERNAL_XNVME_BE_SPDK */
