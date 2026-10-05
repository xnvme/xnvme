#ifndef _INTERNAL_XNVME_BE_NVMF_NS_H
#define _INTERNAL_XNVME_BE_NVMF_NS_H

#include <stdint.h>
#include <string.h>
#include <sys/queue.h>

#include <xnvme_dev.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_qpair.h>
#include <xnvme_be_nvmf_subsys.h>

struct xnvme_be_nvmf_ns {
    struct xnvme_be_nvmf_ref ref;
    uint64_t nsid;
    uint64_t ctrlid;
    struct xnvme_be_nvmf_ctrlr *ctrlr;
    struct xnvme_be_nvmf_qpair *admin_qpair;
    struct xnvme_be_nvmf_subsys *subsys;
    SLIST_ENTRY(xnvme_be_nvmf_ns) entry;
};

int
xnvme_be_nvmf_ns_create(struct xnvme_be_nvmf_ctrlr *ctrlr, struct xnvme_dev *dev, struct xnvme_be_nvmf_subsys *subsys, uint16_t nsid, struct xnvme_be_nvmf_ns **ns);


static inline int
xnvme_be_nvmf_ns_connect(struct xnvme_be_nvmf_ns *ns)
{
    return ns->admin_qpair ? xnvme_be_nvmf_qpair_connect(ns->admin_qpair) : 0;
}


static inline int
xnvme_be_nvmf_ns_disconnect(struct xnvme_be_nvmf_ns *ns)
{
    return ns->admin_qpair ? xnvme_be_nvmf_qpair_disconnect(ns->admin_qpair) : 0;
}

static inline int
xnvme_be_nvmf_ns_get(struct xnvme_be_nvmf_ns *ns)
{
    return xnvme_be_nvmf_ref_get(&ns->ref);
}

static inline int
xnvme_be_nvmf_ns_put(struct xnvme_be_nvmf_ns *ns)
{
    return xnvme_be_nvmf_ref_put(&ns->ref);
}
#endif /* _INTERNAL_XNVME_BE_NVMF_NS_H */