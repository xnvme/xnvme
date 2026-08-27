#ifndef _INTERNAL_XNVME_BE_NVMF_REQ_POOL_H
#define _INTERNAL_XNVME_BE_NVMF_REQ_POOL_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/queue.h>

struct xnvme_be_nvmf_req {
    void *context;
    uint16_t cid;
    uint8_t active;
    SLIST_ENTRY(xnvme_be_nvmf_req) next;
};

struct xnvme_be_nvmf_req_pool {
    uint64_t entries;
    uint64_t allocated;
    struct xnvme_be_nvmf_req *reqs;
    SLIST_HEAD(, xnvme_be_nvmf_req) free_list;
};

static inline int
xnvme_be_nvmf_req_pool_alloc(struct xnvme_be_nvmf_req_pool **pool, uint64_t entries)
{
    struct xnvme_be_nvmf_req_pool *p = calloc(1, sizeof(struct xnvme_be_nvmf_req_pool));

    if (!p) {
        return -ENOMEM;
    }
    
    p->entries = entries;
    
    p->reqs = calloc(entries, sizeof(struct xnvme_be_nvmf_req));
    if (!p->reqs) {
        free(p);
        return -ENOMEM;
    }

    SLIST_INIT(&p->free_list);
    for (int i = entries - 1; i >= 0; i--) {
        p->reqs[i].cid = i;
        p->reqs[i].context = NULL;
        SLIST_INSERT_HEAD(&p->free_list, &p->reqs[i], next);
    }
    
    *pool = p;
    return 0;
}

static inline int
xnvme_be_nvmf_req_pool_free(struct xnvme_be_nvmf_req_pool *pool)
{
    if (pool->allocated != 0) {
        return -EBUSY;
    }

    free(pool->reqs);
    free(pool);
    
    return 0;
}


static inline struct xnvme_be_nvmf_req *
xnvme_be_nvmf_req_get(struct xnvme_be_nvmf_req_pool *pool, uint64_t index)
{
    if (index >= pool->entries) {
        return NULL;
    }
    return &pool->reqs[index];
}

static inline struct xnvme_be_nvmf_req *
xnvme_be_nvmf_req_alloc(struct xnvme_be_nvmf_req_pool *pool, void *user_context)
{
    struct xnvme_be_nvmf_req *req = SLIST_FIRST(&pool->free_list);

    if (req) {
        SLIST_REMOVE_HEAD(&pool->free_list, next); // Using the SLIST_ENTRY next to get the next request
        req->context = user_context;
        req->active = 1;
        pool->allocated++;
    }

    return req;
}

static inline void
xnvme_be_nvmf_req_free(struct xnvme_be_nvmf_req_pool *pool, struct xnvme_be_nvmf_req *req)
{
    req->active = 0;
    SLIST_INSERT_HEAD(&pool->free_list, req, next);
    pool->allocated--;
}
#endif /* _INTERNAL_XNVME_BE_NVMF_REQ_POOL_H */