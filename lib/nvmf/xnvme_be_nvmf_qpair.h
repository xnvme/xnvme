// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_NVMF_QPAIR_H
#define __INTERNAL_XNVME_BE_NVMF_QPAIR_H

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_debug.h>
#include <xnvme_be_nvmf_mem.h>

struct xnvme_be_nvmf_qpair;
struct xnvme_be_nvmf_transport_ops;
struct xnvme_spec_cpl;

struct xnvme_be_nvmf_req {
	SLIST_ENTRY(xnvme_be_nvmf_req) next;
	void *context;
	uint16_t cid;
	uint8_t complete;
	uint16_t status;
	uint8_t active;
	uint8_t padding[9];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_nvmf_req) == 32,
		    "Unexpected size for xnvme_be_nvmf_req");

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
	uint64_t pool_size =
		sizeof(struct xnvme_be_nvmf_req_pool) + entries * sizeof(struct xnvme_be_nvmf_req);

	p = calloc(1, pool_size);
	if (!p) {
		return -ENOMEM;
	}

	p->entries = entries;

	SLIST_INIT(&p->free_list);
	for (uint64_t i = 0; i < entries; i++) {
		p->reqs[i].cid     = i;
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
xnvme_be_nvmf_req_alloc(struct xnvme_be_nvmf_req_pool *pool, void *context)
{
	struct xnvme_be_nvmf_req *req = SLIST_FIRST(&pool->free_list);

	if (req) {
		SLIST_REMOVE_HEAD(&pool->free_list, next);
		req->context  = context;
		req->active   = 1;
		req->complete = 0;
		req->status   = 0;
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

static inline bool
xnvme_be_nvmf_req_is_complete(struct xnvme_be_nvmf_req *req)
{
	return req->complete;
}

static inline bool
xnvme_be_nvmf_req_has_error(struct xnvme_be_nvmf_req *req)
{
	return req->status != 0;
}

enum xnvme_nvmf_qpair_state {
	XNVME_NVMF_QPAIR_STATE_INVALID = 0,
	XNVME_NVMF_QPAIR_STATE_INIT,
	XNVME_NVMF_QPAIR_STATE_CONNECTING,
	XNVME_NVMF_QPAIR_STATE_CONNECTED, /* transport up; Fabric Connect sent, awaiting response
					   */
	XNVME_NVMF_QPAIR_STATE_READY, /* Fabric Connect Response received; queue ready for I/O */
	XNVME_NVMF_QPAIR_STATE_DISCONNECTED,
	XNVME_NVMF_QPAIR_STATE_ERROR,
	XNVME_NVMF_QPAIR_STATE_MAX = XNVME_NVMF_QPAIR_STATE_ERROR,
};

struct xnvme_be_nvmf_qpair_attr {
	uint16_t qid;
	uint16_t qsize;
	uint16_t cntlid;
	uint32_t capsule_size;
	uint32_t completion_size;
};

struct xnvme_be_nvmf_qpair {
	const struct xnvme_be_nvmf_transport_ops *ops;
	enum xnvme_nvmf_qpair_state state;
	struct xnvme_be_nvmf_qpair_attr attr;
	struct xnvme_be_nvmf_req_pool *req_pool;
	struct xnvme_be_nvmf_ctrlr *ctrlr;
	struct xnvme_dev *dev; ///< Pointer to the underlying xNVMe device
};

/* Attributes of every admin qpair. The controller ID is assigned by the Fabrics Connect. */
#define XNVME_BE_NVMF_ADMIN_QPAIR_ATTR                           \
	{                                                        \
		.qid             = XNVME_BE_NVMF_ADMIN_QUEUE_ID, \
		.qsize           = 8,                            \
		.capsule_size    = NVME_CMD_CAPSULE_SIZE,        \
		.completion_size = NVME_CPL_CAPSULE_SIZE,        \
		.cntlid          = NVMF_ADMINQ_DYNAMIC_CTRLR_ID, \
	}


/**
 * Core ctrlr/qpair lifecycle and the hot-path submit/poll wrappers.
 *
 * connect / disconnect / destroy
 *   Manage the lifetime of the underlying transport connection. On connect the
 *   transport must pre-post receive buffers before returning. These are owned
 *   by the transport; the protocol layer never calls post_recv for them.
 *
 * qpair_submit
 *   Post one capsule (capsule, nbytes) onto the send queue, with @cid going
 *   into the transport work request identifier. The CID is already set in the
 *   capsule. The caller may reuse the buffer as soon as `qpair_submit`
 *   returns. The transport does not interpret the bytes.
 *
 * qpair_poll
 *   Drive the transport's completion machinery. For every received response
 *   capsule it calls `xnvme_be_nvmf_qpair_complete()`, the only up-call from
 *   the transport into the core. Returns the number of
 *   completions processed, or a negative errno on error.
 */

int
xnvme_be_nvmf_qpair_create(struct xnvme_be_nvmf_ctrlr *ctrlr, struct xnvme_dev *dev,
			   struct xnvme_be_nvmf_qpair_attr *attr,
			   struct xnvme_be_nvmf_qpair **qpair);

int
xnvme_be_nvmf_qpair_connect(struct xnvme_be_nvmf_qpair *qpair);
int
xnvme_be_nvmf_qpair_disconnect(struct xnvme_be_nvmf_qpair *qpair);
int
xnvme_be_nvmf_qpair_destroy(struct xnvme_be_nvmf_qpair *qpair);

/**
 * The only up-call from the transport into the core.
 * Looks up the request by `cpl->cid` in `qpair->req_pool`, copies the
 * completion into the request's `xnvme_cmd_ctx.cpl`, and marks the request
 * done. Returns 0, or a negative errno for an unknown or inactive CID.
 */
int
xnvme_be_nvmf_qpair_complete(struct xnvme_be_nvmf_qpair *qpair, const struct xnvme_spec_cpl *cpl);

/**
 * Posts the command in @ctx, the hot-path data-plane entry. The request CID
 * replaces the command CID and is also the transport work request identifier.
 * On success @req_out is the in-flight request.
 */
static inline int
xnvme_be_nvmf_qpair_submit(struct xnvme_be_nvmf_qpair *qpair, struct xnvme_cmd_ctx *ctx,
			   struct xnvme_be_nvmf_req **req_out)
{
	struct xnvme_be_nvmf_req *req;
	int err;

	req = xnvme_be_nvmf_req_alloc(qpair->req_pool, ctx);
	if (!req) {
		return -EBUSY;
	}

	ctx->cmd.common.cid = req->cid;

	err = qpair->ops->qpair_submit(qpair, &ctx->cmd, sizeof(ctx->cmd), req->cid);
	if (err) {
		_NVMF_ERROR(NVMF_DEBUG_CATEGORY_CMD_IO,
			    "FAILED: qpair->ops->qpair_submit(), err: %d", err);
		xnvme_be_nvmf_req_free(qpair->req_pool, req);
		return err;
	}

	*req_out = req;

	return 0;
}

/**
 * Processes up to @max completions, the hot-path data-plane entry. Returns
 * the number of completions processed, or a negative errno.
 */
static inline int
xnvme_be_nvmf_qpair_poll(struct xnvme_be_nvmf_qpair *qpair, uint32_t max)
{
	return qpair->ops->qpair_poll(qpair, max);
}

static inline bool
xnvme_be_nvmf_qpair_has_error(struct xnvme_be_nvmf_qpair *qpair)
{
	return qpair->state == XNVME_NVMF_QPAIR_STATE_ERROR;
}

static inline bool
xnvme_be_nvmf_qpair_is_connected(struct xnvme_be_nvmf_qpair *qpair)
{
	return qpair->state == XNVME_NVMF_QPAIR_STATE_CONNECTED ||
	       qpair->state == XNVME_NVMF_QPAIR_STATE_READY;
}

static inline void
xnvme_be_nvmf_wait_for_completion(struct xnvme_be_nvmf_qpair *qpair, struct xnvme_be_nvmf_req *req)
{
	while (!xnvme_be_nvmf_req_is_complete(req)) {
		if (xnvme_be_nvmf_req_has_error(req))
			break;

		if (xnvme_be_nvmf_qpair_has_error(qpair)) {
			req->status = ECONNRESET;
			break;
		}

		xnvme_be_nvmf_qpair_poll(qpair, 1);
	}
}

static inline int
xnvme_be_nvmf_qpair_submit_sync(struct xnvme_be_nvmf_qpair *qpair, struct xnvme_cmd_ctx *ctx)
{
	struct xnvme_be_nvmf_req *req;
	int err;

	err = xnvme_be_nvmf_qpair_submit(qpair, ctx, &req);
	if (err) {
		return err;
	}

	xnvme_be_nvmf_wait_for_completion(qpair, req);

	err = req->status;
	xnvme_be_nvmf_req_free(qpair->req_pool, req);

	return err;
}

/**
 * Points @ctx's command at @dbuf with a keyed SGL, using the key of the
 * registered buffer containing it.
 */
static inline int
xnvme_be_nvmf_cmd_set_sgl(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes)
{
	uint32_t key;
	int err;

	err = xnvme_be_nvmf_registration_lookup_key(ctx->dev, dbuf, dbuf_nbytes, &key);
	if (err) {
		_NVMF_ERROR(NVMF_DEBUG_CATEGORY_CMD_IO,
			    "FAILED: xnvme_be_nvmf_registration_lookup_key(), err: %d", err);
		return err;
	}

	ctx->cmd.common.psdt = 0b10;
	xnvme_be_nvmf_keyed_sgl_init(&ctx->cmd.common.dptr.sgl, dbuf, dbuf_nbytes, key);

	return 0;
}

/**
 * Synchronous command with an optional data buffer, shared by the admin and sync backends.
 */
static inline int
xnvme_be_nvmf_cmd_sync(struct xnvme_be_nvmf_qpair *qpair, struct xnvme_cmd_ctx *ctx, void *dbuf,
		       size_t dbuf_nbytes)
{
	int err;

	err = xnvme_be_nvmf_cmd_set_sgl(ctx, dbuf, dbuf_nbytes);
	if (err) {
		_NVMF_ERROR(NVMF_DEBUG_CATEGORY_CMD,
			    "FAILED: xnvme_be_nvmf_cmd_set_sgl(), err: %d", err);
		return err;
	}

	return xnvme_be_nvmf_qpair_submit_sync(qpair, ctx);
}

#endif /* __INTERNAL_XNVME_BE_NVMF_QPAIR_H */