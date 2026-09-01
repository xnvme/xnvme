#ifndef _INTERNAL_XNVME_BE_NVMF_REQ_POOL_H
#define _INTERNAL_XNVME_BE_NVMF_REQ_POOL_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/queue.h>

enum xnvme_be_nvmf_req_type {
    XNVME_BE_NVMF_REQ_TYPE_INTERNAL,
    XNVME_BE_NVMF_REQ_TYPE_USER,
    XNVME_BE_NVMF_REQ_TYPE_MAX = XNVME_BE_NVMF_REQ_TYPE_USER,
};

enum xnvme_be_nvmf_req_cmpl_sts {
    XNVME_BE_NVMF_REQ_CMPL_STS_PENDING,
    XNVME_BE_NVMF_REQ_CMPL_STS_SEND_SUCCESS,
    XNVME_BE_NVMF_REQ_CMPL_STS_SEND_ERROR,
    XNVME_BE_NVMF_REQ_CMPL_STS_RECV_SUCCESS,
    XNVME_BE_NVMF_REQ_CMPL_STS_RECV_ERROR,
    XNVME_BE_NVMF_REQ_CMPL_STS_MAX = XNVME_BE_NVMF_REQ_CMPL_STS_RECV_ERROR,
};

struct xnvme_be_nvmf_req {
    SLIST_ENTRY(xnvme_be_nvmf_req) next;
    void *context;
    uint16_t cid;
    uint8_t type : 1;
    uint8_t cmpl_sts : 3;
    uint8_t reserved : 4;
    uint16_t status;
    uint8_t active;
    uint8_t padding[9];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_req) == 32, "Unexpected size for xnvme_be_nvmf_req");

struct xnvme_be_nvmf_req_pool {
    uint64_t entries;
    uint64_t allocated;
    SLIST_HEAD(, xnvme_be_nvmf_req) free_list;
    struct xnvme_be_nvmf_req reqs[];
};

static inline int
xnvme_be_nvmf_req_pool_alloc(struct xnvme_be_nvmf_req_pool **pool, uint64_t entries)
{
    struct xnvme_be_nvmf_req_pool *p;
    uint64_t pool_size = sizeof(struct xnvme_be_nvmf_req_pool) + entries * sizeof(struct xnvme_be_nvmf_req);

    p = calloc(1, pool_size);
    if (!p) {
        return -ENOMEM;
    }
    
    p->entries = entries;

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

    free(pool);
    
    return 0;
}

// TODO: Consider moving to errno-based for better error reporting
static inline struct xnvme_be_nvmf_req *
xnvme_be_nvmf_req_get(struct xnvme_be_nvmf_req_pool *pool, uint64_t index)
{
    if (index >= pool->entries) {
        return NULL;
    }

    if (!pool->reqs[index].active) {
        return NULL;
    }

    return &pool->reqs[index];
}

static inline struct xnvme_be_nvmf_req *
_xnvme_be_nvmf_req_alloc_helper(struct xnvme_be_nvmf_req_pool *pool, enum xnvme_be_nvmf_req_type type)
{
    struct xnvme_be_nvmf_req *req = SLIST_FIRST(&pool->free_list);

    if (req) {
        SLIST_REMOVE_HEAD(&pool->free_list, next); // Using the SLIST_ENTRY next to get the next request
        req->type = type;
        req->active = 1;
        pool->allocated++;
    }

    return req;
}


static inline struct xnvme_be_nvmf_req *
xnvme_be_nvmf_req_alloc(struct xnvme_be_nvmf_req_pool *pool)
{
    return _xnvme_be_nvmf_req_alloc_helper(pool, XNVME_BE_NVMF_REQ_TYPE_USER);
}

static inline struct xnvme_be_nvmf_req *
xnvme_be_nvmf_req_internal_alloc(struct xnvme_be_nvmf_req_pool *pool)
{
    return _xnvme_be_nvmf_req_alloc_helper(pool, XNVME_BE_NVMF_REQ_TYPE_INTERNAL);
}

static inline void
xnvme_be_nvmf_req_free(struct xnvme_be_nvmf_req_pool *pool, struct xnvme_be_nvmf_req *req)
{
    req->active = 0;
    SLIST_INSERT_HEAD(&pool->free_list, req, next);
    pool->allocated--;
}
#endif /* _INTERNAL_XNVME_BE_NVMF_REQ_POOL_H */