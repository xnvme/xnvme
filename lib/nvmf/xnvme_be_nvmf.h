// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_NVMF_H
#define __INTERNAL_XNVME_BE_NVMF_H

#include <errno.h>
#include <pthread.h>
#include <stddef.h>

#include <libxnvme.h>

#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_queue.h>


#ifndef container_of
#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#define XNVME_BE_NVMF_ADMIN_QUEUE_ID    0
#define XNVME_BE_NVMF_SYNC_QUEUE_ID     1
#define XNVME_BE_NVMF_IO_QUEUE_ID_START XNVME_BE_NVMF_SYNC_QUEUE_ID

#define NVME_CMD_CAPSULE_SIZE sizeof(struct xnvme_spec_cmd_common)
#define NVME_CPL_CAPSULE_SIZE sizeof(struct xnvme_spec_cpl)

struct xnvme_be_nvmf_qpair;

struct xnvme_be_nvmf_queue {
	struct xnvme_queue_base base;
	uint64_t completions_pending;
	struct xnvme_be_nvmf_qpair *qpair;
	uint8_t be_rsvd[216]; ///< Auxilary backend data
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_queue) == sizeof(struct xnvme_queue),
		    "Incorrect size of xnvme_be_nvmf_queue");

struct xnvme_be_nvmf_state {
	void *ctrlr;       ///< Pointer to attached controller (must be first: platform
			   		   ///< stores ctrlr at state[0])
	void *subsys;
	void *ns;          ///< Pointer to associated namespace
	void *admin_qpair; ///< Admin queue pair
	void *sync_qpair;  ///< Synchronous IO queue pair
	uint8_t _rsvd0[22];

	union {
		pthread_mutex_t lock; ///< Controller lock for thread-safe operations
		uint8_t _fill[64];
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_state) == XNVME_BE_STATE_NBYTES,
		    "Incorrect size of size");

extern struct xnvme_be_admin g_xnvme_be_nvmf_admin;
extern struct xnvme_be_async g_xnvme_be_nvmf_async;
extern struct xnvme_be_dev g_xnvme_be_nvmf_dev;
extern struct xnvme_be_mem g_xnvme_be_nvmf_mem;
extern struct xnvme_be_sync g_xnvme_be_nvmf_sync;

#define XNVME_NVMF_DISCOVERY_NQN "nqn.2014-08.org.nvmexpress.discovery"

#endif /* __INTERNAL_XNVME_BE_NVMF_H */
