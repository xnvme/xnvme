// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_TRANSPORT_BASE_H
#define __INTERNAL_XNVME_TRANSPORT_BASE_H

#include <errno.h>
#include <pthread.h>
#include <stddef.h>

#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_queue.h>

#include <xnvme_be_state.h>

#ifndef container_of
#define container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#define TO_TRANSPORT_CTRLR(ctrlr)   ((struct xnvme_transport_ctrlr *)(ctrlr))
#define TO_TRANSPORT_QPAIR(qpair)   ((struct xnvme_transport_qpair *)(qpair))
#define TO_TRANSPORT_DEV_STATE(dev) (&xnvme_be_state(dev)->rdma_state)

#define XNVME_TRANSPORT_ADMIN_QUEUE_ID 0

#define TRANSPORT_ADMINQ_DYNAMIC_CTRLR_ID 0xffff

// The keyed SGL length field is 24 bits wide
#define TRANSPORT_KEYED_SGL_MAX_LEN 0xFFFFFFUL

#define NVME_CMD_CAPSULE_SIZE sizeof(struct xnvme_spec_cmd_common)
#define NVME_CPL_CAPSULE_SIZE sizeof(struct xnvme_spec_cpl)

static inline void
xnvme_transport_keyed_sgl_init(struct xnvme_spec_sgl_descriptor *sgl, void *buf, size_t len,
			       uint64_t key)
{
	uint64_t keyed = 0;

	keyed = xnvme_bits_set(keyed, XNVME_SPEC_SGL_DESCRIPTOR_KEYED_TYPE,
			       XNVME_SPEC_SGL_DESCR_TYPE_KEYED_DATA_BLOCK);
	keyed = xnvme_bits_set(keyed, XNVME_SPEC_SGL_DESCRIPTOR_KEYED_SUBTYPE,
			       XNVME_SPEC_SGL_DESCR_SUBTYPE_ADDRESS);
	keyed = xnvme_bits_set(keyed, XNVME_SPEC_SGL_DESCRIPTOR_KEYED_LEN, len);
	keyed = xnvme_bits_set(keyed, XNVME_SPEC_SGL_DESCRIPTOR_KEYED_KEY, key);

	// set the SGL descriptor to point to the internal buffer
	sgl->addr  = (uintptr_t)buf;
	sgl->keyed = keyed;
}

#endif /* __INTERNAL_XNVME_TRANSPORT_BASE_H */
