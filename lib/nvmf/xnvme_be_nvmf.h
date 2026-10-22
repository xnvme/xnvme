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

#define TO_NVMF_DEV_STATE(dev)     ((struct xnvme_be_nvmf_state *)(dev)->be.state)
#define TO_NVMF_CTRLR(ctrlr)       ((struct xnvme_be_nvmf_ctrlr *)(ctrlr))
#define TO_NVMF_QPAIR(qpair)       ((struct xnvme_be_nvmf_qpair *)(qpair))
#define TO_NVMF_QUEUE(queue)       container_of(&(queue)->base, struct xnvme_be_nvmf_queue, base)
#define TO_NVMF_QUEUE_QPAIR(queue) (TO_NVMF_QUEUE(queue)->qpair)

#define XNVME_BE_NVMF_ADMIN_QUEUE_ID    0
#define XNVME_BE_NVMF_SYNC_QUEUE_ID     1
#define XNVME_BE_NVMF_IO_QUEUE_ID_START XNVME_BE_NVMF_SYNC_QUEUE_ID

#define NVMF_ADMINQ_DYNAMIC_CTRLR_ID 0xffff

#define NVME_CMD_CAPSULE_SIZE sizeof(struct xnvme_spec_cmd_common)
#define NVME_CPL_CAPSULE_SIZE sizeof(struct xnvme_spec_cpl)

struct xnvme_be_nvmf_qpair;

struct xnvme_be_nvmf_queue {
	struct xnvme_queue_base base;
	struct xnvme_be_nvmf_qpair *qpair;
	uint8_t be_rsvd[224]; ///< Auxilary backend data
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_queue) == sizeof(struct xnvme_queue),
		    "Incorrect size of xnvme_be_nvmf_queue");

struct xnvme_be_nvmf_state {
	void *ctrlr; ///< Pointer to attached controller (must be first: platform
		     ///< stores ctrlr at state[0])
	void *subsys;
	void *ns;          ///< Pointer to associated namespace
	void *admin_qpair; ///< Admin queue pair
	void *sync_qpair;  ///< Synchronous IO queue pair
	void *util_rbtree; ///< Pointer to utility red-black tree structure
	void *mem_cache;   ///< Last registered buffer found by a key lookup
	uint8_t _rsvd0[6];
	uint8_t _fill[64];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_state) == XNVME_BE_STATE_NBYTES,
		    "Incorrect size of size");

extern struct xnvme_be_admin g_xnvme_be_nvmf_admin;
extern struct xnvme_be_async g_xnvme_be_nvmf_async;
extern struct xnvme_be_dev g_xnvme_be_nvmf_dev;
extern struct xnvme_be_mem g_xnvme_be_nvmf_mem;
extern struct xnvme_be_sync g_xnvme_be_nvmf_sync;

static inline void
xnvme_be_nvmf_keyed_sgl_init(struct xnvme_spec_sgl_descriptor *sgl, void *buf, size_t len,
			     uint64_t key)
{
	// Set the SGL descriptor type and subtype
	sgl->keyed.type    = XNVME_SPEC_SGL_DESCR_TYPE_KEYED_DATA_BLOCK;
	sgl->keyed.subtype = XNVME_SPEC_SGL_DESCR_SUBTYPE_ADDRESS;

	// set the SGL descriptor to point to the internal buffer
	sgl->addr      = (uintptr_t)buf;
	sgl->keyed.len = len;
	sgl->keyed.key = key;
}

#endif /* __INTERNAL_XNVME_BE_NVMF_H */
