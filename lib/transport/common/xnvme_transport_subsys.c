// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_transport_debug.h>
#include <xnvme_transport_ns.h>
#include <xnvme_transport_subsys.h>

#define TRANSPORT_DEBUG_CATEGORY TRANSPORT_DEBUG_CATEGORY_CORE_SUBSYS

static void
_transport_subsys_destroy(struct xnvme_transport_ref *ref)
{
	struct xnvme_transport_subsys *subsys =
		container_of(ref, struct xnvme_transport_subsys, ref);

	assert(SLIST_EMPTY(&subsys->namespaces));

	if (subsys->admin_qpair) {
		xnvme_transport_qpair_destroy(subsys->admin_qpair);
		subsys->admin_qpair = NULL;
	}

	free(subsys->subnqn);
	subsys->subnqn = NULL;

	SLIST_REMOVE(&subsys->ctrlr->subsystems, subsys, xnvme_transport_subsys, entry);

	free(subsys);
}

int
xnvme_transport_subsys_create(struct xnvme_transport_ctrlr *ctrlr, const char *subnqn,
			      struct xnvme_transport_subsys **subsys)
{
	struct xnvme_transport_subsys *new_subsys;

	if (!subnqn) {
		return -EINVAL;
	}

	new_subsys = calloc(1, sizeof(*new_subsys));
	if (!new_subsys) {
		return -ENOMEM;
	}

	// The initial reference is owned by the controller, devices take their own
	xnvme_transport_ref_init(&new_subsys->ref, _transport_subsys_destroy);
	new_subsys->ctrlr = ctrlr;
	new_subsys->discovery_nsidx = ++ctrlr->last_assigned_discovery_id;

	SLIST_INIT(&new_subsys->namespaces);

	new_subsys->subnqn = strdup(subnqn);
	if (!new_subsys->subnqn) {
		free(new_subsys);
		return -ENOMEM;
	}

	SLIST_INSERT_HEAD(&ctrlr->subsystems, new_subsys, entry);
	*subsys = new_subsys;

	TRANSPORT_DEBUG("INFO: Created new subsys for subnqn=%s, discovery_nsidx=%d",
			new_subsys->subnqn, new_subsys->discovery_nsidx);

	return 0;
}

struct xnvme_transport_ns *
xnvme_transport_subsys_find_first_ns(struct xnvme_transport_subsys *subsys, uint32_t nsid)
{
	struct xnvme_transport_ns *ns;

	SLIST_FOREACH(ns, &subsys->namespaces, entry)
	{
		if (ns->nsid == nsid) {
			return ns;
		}
	}

	return NULL;
}