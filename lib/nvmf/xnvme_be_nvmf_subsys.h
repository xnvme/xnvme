#ifndef __INTERNAL_XNVME_BE_NVMF_SUBSYS_H
#define __INTERNAL_XNVME_BE_NVMF_SUBSYS_H

#include <stdatomic.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <assert.h>

#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_ref.h>

struct xnvme_be_nvmf_subsys {
	struct xnvme_be_nvmf_ref ref;
	struct xnvme_be_nvmf_ctrlr *ctrlr;
	char *subnqn;
	int discovery_nsidx;
	SLIST_HEAD(, xnvme_be_nvmf_ns) namespaces;
	SLIST_ENTRY(xnvme_be_nvmf_subsys) entry;
};

int
xnvme_be_nvmf_subsys_create(struct xnvme_be_nvmf_ctrlr *ctrlr, const char *subnqn,
			    struct xnvme_be_nvmf_subsys **subsys);

struct xnvme_be_nvmf_ns *
xnvme_be_nvmf_subsys_find_first_ns(struct xnvme_be_nvmf_subsys *subsys, uint32_t nsid);

static inline int
xnvme_be_nvmf_subsys_get(struct xnvme_be_nvmf_subsys *subsys)
{
	return xnvme_be_nvmf_ref_get(&subsys->ref);
}

static inline int
xnvme_be_nvmf_subsys_put(struct xnvme_be_nvmf_subsys *subsys)
{
	return xnvme_be_nvmf_ref_put(&subsys->ref);
}
#endif /* __INTERNAL_XNVME_BE_NVMF_SUBSYS_H */
