// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_TRANSPORT_NS_H
#define __INTERNAL_XNVME_TRANSPORT_NS_H

#include <stdint.h>
#include <string.h>
#include <sys/queue.h>

#include <xnvme_dev.h>

#include <xnvme_transport_base.h>
#include <xnvme_transport_ref.h>
#include <xnvme_transport_qpair.h>
#include <xnvme_transport_subsys.h>

struct xnvme_transport_ns {
	struct xnvme_transport_ref ref;
	uint32_t nsid;
	uint64_t qidmap; ///< Free I/O queue identifiers of the namespace's association, 1-based
	uint16_t mqes;   ///< CAP.MQES of the namespace controller's association, 0's based
	struct xnvme_transport_ctrlr *ctrlr;
	struct xnvme_transport_qpair *admin_qpair;
	struct xnvme_transport_subsys *subsys;
	SLIST_ENTRY(xnvme_transport_ns) entry;
};

int
xnvme_transport_ns_create(struct xnvme_transport_ctrlr *ctrlr, struct xnvme_dev *dev,
			  struct xnvme_transport_subsys *subsys, uint32_t nsid,
			  struct xnvme_transport_ns **ns);

static inline int
xnvme_transport_ns_get(struct xnvme_transport_ns *ns)
{
	return xnvme_transport_ref_get(&ns->ref);
}

static inline int
xnvme_transport_ns_put(struct xnvme_transport_ns *ns)
{
	return xnvme_transport_ref_put(&ns->ref);
}

static inline int
xnvme_transport_ns_alloc_next_qid(struct xnvme_transport_ns *ns, uint16_t *qid)
{
	int bit;

	bit = __builtin_ffsll(ns->qidmap);
	if (!bit) {
		return -EBUSY;
	}
	ns->qidmap &= ~(1ULL << (bit - 1));
	*qid = bit;

	return 0;
}

static inline void
xnvme_transport_ns_free_qid(struct xnvme_transport_ns *ns, uint16_t qid)
{
	ns->qidmap |= 1ULL << (qid - 1);
}

#endif /* __INTERNAL_XNVME_TRANSPORT_NS_H */