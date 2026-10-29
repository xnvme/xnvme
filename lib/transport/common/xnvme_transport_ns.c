// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_transport_ns.h>

static void
_transport_ns_destroy(struct xnvme_transport_ref *ref)
{
	struct xnvme_transport_ns *ns = container_of(ref, struct xnvme_transport_ns, ref);

	if (ns->admin_qpair) {
		xnvme_transport_qpair_destroy(ns->admin_qpair);
	}

	SLIST_REMOVE(&ns->subsys->namespaces, ns, xnvme_transport_ns, entry);
	xnvme_transport_subsys_put(ns->subsys);

	free(ns);
}

int
xnvme_transport_ns_create(struct xnvme_transport_ctrlr *ctrlr, struct xnvme_dev *dev,
			  struct xnvme_transport_subsys *subsys, uint32_t nsid,
			  struct xnvme_transport_ns **ns)
{
	struct xnvme_transport_ns *new_ns;
	struct xnvme_transport_qpair_attr admin_attr = XNVME_TRANSPORT_ADMIN_QPAIR_ATTR;
	int err;

	new_ns = calloc(1, sizeof(*new_ns));
	if (!new_ns) {
		return -ENOMEM;
	}

	xnvme_transport_ref_init(&new_ns->ref, _transport_ns_destroy);
	new_ns->nsid = nsid;

	err = xnvme_transport_qpair_create(ctrlr, dev, &admin_attr, &new_ns->admin_qpair);
	if (err) {
		free(new_ns);
		return err;
	}
	new_ns->ctrlr = ctrlr;

	xnvme_transport_subsys_get(subsys);
	SLIST_INSERT_HEAD(&subsys->namespaces, new_ns, entry);
	new_ns->subsys = subsys;
	new_ns->nsid = nsid;

	*ns = new_ns;
	return 0;
}