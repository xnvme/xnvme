#include <xnvme_be_nvmf_debug.h>
#include <xnvme_be_nvmf_ns.h>

#include <xnvme_be_nvmf_subsys.h>

#define NVMF_DEBUG_CATEGORY NVMF_DEBUG_CATEGORY_CORE_SUBSYS

static void
_nvmf_subsys_destroy(void *ctx)
{
	struct xnvme_be_nvmf_subsys *subsys = (struct xnvme_be_nvmf_subsys *)ctx;

	assert(SLIST_EMPTY(&subsys->namespaces));

	free(subsys->subnqn);
	subsys->subnqn = NULL;

	SLIST_REMOVE(&subsys->ctrlr->subsystems, subsys, xnvme_be_nvmf_subsys, entry);

	free(subsys);
}

int
xnvme_be_nvmf_subsys_create(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *subnqn,
			    struct xnvme_be_nvmf_subsys **subsys)
{
	struct xnvme_be_nvmf_subsys *new_subsys;

	new_subsys = calloc(1, sizeof(*new_subsys));
	if (!new_subsys) {
		return -ENOMEM;
	}

	xnvme_be_nvmf_ref_init(&new_subsys->ref, _nvmf_subsys_destroy, new_subsys);
	new_subsys->ctrlr = ctrlr;
	new_subsys->discovery_nsidx = ++ctrlr->last_assigned_discovery_id;
	if (!subnqn) {
		free(new_subsys);
		return -EINVAL;
	}

	SLIST_INIT(&new_subsys->namespaces);

	new_subsys->subnqn = strdup(subnqn);
	if (!new_subsys->subnqn) {
		free(new_subsys);
		return -ENOMEM;
	}

	SLIST_INSERT_HEAD(&ctrlr->subsystems, new_subsys, entry);
	*subsys = new_subsys;

	NVMF_DEBUG("INFO: Created new subsys for subnqn=%s, discovery_nsidx=%d",
		   new_subsys->subnqn, new_subsys->discovery_nsidx);

	return 0;
}

struct xnvme_be_nvmf_ns *
xnvme_be_nvmf_subsys_find_first_ns(struct xnvme_be_nvmf_subsys *subsys, uint32_t nsid)
{
	struct xnvme_be_nvmf_ns *ns;

	SLIST_FOREACH(ns, &subsys->namespaces, entry)
	{
		if (ns->nsid == nsid) {
			return ns;
		}
	}

	return NULL;
}