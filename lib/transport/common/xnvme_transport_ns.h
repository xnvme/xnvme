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
	uint64_t nsid;
	struct xnvme_transport_ctrlr *ctrlr;
	struct xnvme_transport_qpair *admin_qpair;
	struct xnvme_transport_subsys *subsys;
	SLIST_ENTRY(xnvme_transport_ns) entry;
};

int
xnvme_transport_ns_create(struct xnvme_transport_ctrlr *ctrlr, struct xnvme_dev *dev,
			  struct xnvme_transport_subsys *subsys, uint16_t nsid,
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
#endif /* __INTERNAL_XNVME_TRANSPORT_NS_H */