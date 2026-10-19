// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_SPDK_STATE_H
#define __INTERNAL_XNVME_BE_SPDK_STATE_H
#include <pthread.h>
#include <xnvme_be.h>

struct iovec;
struct spdk_nvme_ctrlr;
struct spdk_nvme_ns;
struct spdk_nvme_qpair;

struct xnvme_be_spdk_iov_payload {
	struct iovec *iov;
	size_t iov_cnt;
	size_t iov_pos;
	size_t iov_offset;
};
struct xnvme_be_spdk_state {
	struct spdk_nvme_ctrlr *ctrlr; ///< Pointer to attached controller
	struct spdk_nvme_ns *ns;       ///< Pointer to associated namespace
	struct spdk_nvme_qpair *qpair; ///< QPAIR for SYNC IO commands

	uint8_t attached;

	uint8_t _rsvd[7];

	struct xnvme_be_spdk_iov_payload payload;

	union {
		pthread_mutex_t qpair_lock; ///< LOCK for SYNC IO commands
		uint8_t _fill[64];
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_spdk_state) == XNVME_BE_STATE_NBYTES, "Incorrect size")
XNVME_BE_CTRLR_STATE_FIRST(xnvme_be_spdk_state);

#endif /* __INTERNAL_XNVME_BE_SPDK_STATE_H */
