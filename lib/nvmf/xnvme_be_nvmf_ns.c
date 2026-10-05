#include <xnvme_be_nvmf_ns.h>

static void
_nvmf_ns_destroy(void *ctx)
{
    struct xnvme_be_nvmf_ns *ns = (struct xnvme_be_nvmf_ns *)ctx;
    
    if (ns->admin_qpair) {
        xnvme_be_nvmf_qpair_destroy(ns->admin_qpair);
    }

    SLIST_REMOVE(&ns->subsys->namespaces, ns, xnvme_be_nvmf_ns, entry);
    xnvme_be_nvmf_subsys_put(ns->subsys);

    free(ns);
}

int
xnvme_be_nvmf_ns_create(struct xnvme_be_nvmf_ctrlr *ctrlr, struct xnvme_dev *dev, struct xnvme_be_nvmf_subsys *subsys, uint16_t nsid, struct xnvme_be_nvmf_ns **ns)
{
    struct xnvme_be_nvmf_ns *new_ns;

    new_ns = (struct xnvme_be_nvmf_ns *)malloc(sizeof(struct xnvme_be_nvmf_ns));
    if (!new_ns) {
        return -ENOMEM;
    }

    xnvme_be_nvmf_ref_init(&new_ns->ref, _nvmf_ns_destroy, new_ns);
    struct xnvme_be_nvmf_qpair_attr attr = {
        .qid = XNVME_BE_NVMF_ADMIN_QUEUE_ID,
        .qsize = 8,
        .capsule_size = NVME_CMD_CAPSULE_SIZE,
        .completion_size = NVME_CPL_CAPSULE_SIZE,
    };
    int err;

    new_ns->nsid = nsid;

    err = xnvme_be_nvmf_qpair_create(ctrlr, dev, &attr, &new_ns->admin_qpair);
    if (err) {        xnvme_be_nvmf_qpair_destroy(new_ns->admin_qpair);
        free(new_ns);
        return err;
    }
    new_ns->ctrlr = ctrlr;

    xnvme_be_nvmf_subsys_get(subsys);
    SLIST_INSERT_HEAD(&subsys->namespaces, new_ns, entry);
    new_ns->subsys = subsys;
    new_ns->nsid = nsid;

    *ns = new_ns;
    return 0;
}