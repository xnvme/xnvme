#ifndef __INTERNAL_XNVME_BE_NVMF_REF_H
#define __INTERNAL_XNVME_BE_NVMF_REF_H

typedef void (*xnvme_be_nvmf_ref_destructor)(void *ctx);

struct xnvme_be_nvmf_ref {
    int refs;  ///< Reference count, not thread-safe
    xnvme_be_nvmf_ref_destructor destructor;
    void *ctx;
};

static inline void
xnvme_be_nvmf_ref_init(struct xnvme_be_nvmf_ref *ref, xnvme_be_nvmf_ref_destructor destructor, void *ctx)
{
    ref->refs = 1;
    ref->destructor = destructor;
    ref->ctx = ctx;
}

static inline int
xnvme_be_nvmf_ref_get(struct xnvme_be_nvmf_ref *ref)
{
    return ++ref->refs;
}

static inline int
xnvme_be_nvmf_ref_put(struct xnvme_be_nvmf_ref *ref)
{
    if (--ref->refs == 0) {
        if (ref->destructor) {
            ref->destructor(ref->ctx);
        }
        return 0;
    }
    return ref->refs;
}

#endif /* __INTERNAL_XNVME_BE_NVMF_REF_H */