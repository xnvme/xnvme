// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_TRANSPORT_SUBSYS_H
#define __INTERNAL_XNVME_TRANSPORT_SUBSYS_H

#include <assert.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include <xnvme_transport_ref.h>
#include <xnvme_transport_ctrlr.h>

struct xnvme_transport_subsys {
	struct xnvme_transport_ref ref;
	struct xnvme_transport_ctrlr *ctrlr;
	char *subnqn;
	int discovery_nsidx;
	struct xnvme_transport_qpair *admin_qpair;
	SLIST_HEAD(, xnvme_transport_ns) namespaces;
	SLIST_ENTRY(xnvme_transport_subsys) entry;
};

int
xnvme_transport_subsys_create(struct xnvme_transport_ctrlr *ctrlr, const char *subnqn,
			      struct xnvme_transport_subsys **subsys);

struct xnvme_transport_ns *
xnvme_transport_subsys_find_first_ns(struct xnvme_transport_subsys *subsys, uint32_t nsid);

static inline int
xnvme_transport_subsys_get(struct xnvme_transport_subsys *subsys)
{
	return xnvme_transport_ref_get(&subsys->ref);
}

static inline int
xnvme_transport_subsys_put(struct xnvme_transport_subsys *subsys)
{
	return xnvme_transport_ref_put(&subsys->ref);
}
#endif /* __INTERNAL_XNVME_TRANSPORT_SUBSYS_H */
