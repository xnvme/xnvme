// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_NVMF_H
#define __INTERNAL_XNVME_BE_NVMF_H

#include <errno.h>
#include <xnvme_be.h>
#include <libxnvme.h>
#include <xnvme_queue.h>
#include <pthread.h>

#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_qpair.h>

#ifndef container_of
#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#define XNVME_BE_NVMF_ADMIN_QUEUE_ID    0
#define XNVME_BE_NVMF_SYNC_QUEUE_ID     1
#define XNVME_BE_NVMF_IO_QUEUE_ID_START XNVME_BE_NVMF_SYNC_QUEUE_ID

#define NVME_CMD_CAPSULE_SIZE sizeof(struct xnvme_spec_cmd_common)
#define NVME_CPL_CAPSULE_SIZE sizeof(struct xnvme_spec_cpl)

struct xnvme_be_nvmf_transport_ops {
	int (*create_ctrlr)(struct xnvme_be_nvmf_ctrlr **ctrlr);
};

struct xnvme_be_nvmf_transport {
	const char *name;
	struct xnvme_be_nvmf_transport_ops ops;
};

struct xnvme_be_nvmf_state {
	void *ctrlr; ///< Pointer to attached controller (must be first: platform
		     ///< stores ctrlr at state[0])
	void *ns;    ///< Pointer to associated namespace
	void *qpair; ///< QPAIR for SYNC IO commands

	uint8_t _rsvd0[38];

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

#define _INTERNAL_NOT_IMPLEMENTED()                     \
	{                                               \
		XNVME_DEBUG("FAILED: Not implemented"); \
		return -ENOSYS;                         \
	}

#endif /* __INTERNAL_XNVME_BE_NVMF_H */
