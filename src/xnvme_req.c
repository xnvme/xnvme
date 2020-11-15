// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>

void
xnvme_req_pool_free(struct xnvme_req_pool *pool)
{
	free(pool);
}

int
xnvme_req_pool_alloc(struct xnvme_req_pool **pool, uint32_t capacity)
{
	const size_t nbytes = capacity * sizeof(*(*pool)->elm) + sizeof(**pool);

	(*pool) = malloc(nbytes);
	if (!(*pool)) {
		return -errno;
	}
	memset((*pool), 0, nbytes);

	SLIST_INIT(&(*pool)->head);

	(*pool)->capacity = capacity;

	return 0;
}

int
xnvme_req_pool_init(struct xnvme_req_pool *pool,
		    struct xnvme_queue *queue,
		    xnvme_queue_cb cb,
		    void *cb_arg)
{
	for (uint32_t i = 0; i < pool->capacity; ++i) {
		pool->elm[i].pool = pool;
		pool->elm[i].async.queue = queue;
		pool->elm[i].async.cb = cb;
		pool->elm[i].async.cb_arg = cb_arg;

		SLIST_INSERT_HEAD(&pool->head, &pool->elm[i], link);
	}

	return 0;
}

void
xnvme_req_pr(const struct xnvme_req *req, int XNVME_UNUSED(opts))
{
	printf("xnvme_req: ");

	if (!req) {
		printf("~\n");
		return;
	}

	printf("{cdw0: 0x%x, sc: 0x%x, sct: 0x%x}\n", req->cpl.cdw0,
	       req->cpl.status.sc, req->cpl.status.sct);
}

void
xnvme_req_clear(struct xnvme_req *req)
{
	memset(req, 0x0, sizeof(*req));
}
