// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_CBI_ASYNC_EMU_ENABLED
#include <errno.h>
#include <xnvme_queue.h>
#include <xnvme_dev.h>

/**
 * NOTE: this should be possible to do within a single cache-line... refactor pointers for re-use
 *       and use a flag to distinguish contig vs. iovec payload
 */
struct qpair_entry {
	struct xnvme_dev *dev;
	struct xnvme_cmd_ctx *ctx;

	void *data;
	void *meta;
	uint32_t data_nbytes;
	uint32_t data_vec_cnt;
	uint32_t meta_nbytes;
	uint32_t meta_vec_cnt;
	uint32_t is_vectored;

	STAILQ_ENTRY(qpair_entry) link;
};

struct qpair {
	STAILQ_HEAD(, qpair_entry) rp; ///< Request pool
	STAILQ_HEAD(, qpair_entry) sq; ///< Submission queue
	uint32_t capacity;
	struct qpair_entry elm[];
};

struct xnvme_queue_emu {
	struct xnvme_queue_base base;

	struct qpair *qp;

	uint8_t _rsvd[224];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_emu) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

static int
qpair_term(struct qpair *qp)
{
	free(qp);

	return 0;
}

static int
qpair_alloc(struct qpair **qp, uint32_t capacity)
{
	const size_t nbytes = capacity * sizeof(*(*qp)->elm) + sizeof(**qp);

	(*qp) = malloc(nbytes);
	if (!(*qp)) {
		return -errno;
	}
	memset((*qp), 0, nbytes);

	STAILQ_INIT(&(*qp)->sq);
	STAILQ_INIT(&(*qp)->rp);

	(*qp)->capacity = capacity;

	for (uint32_t i = 0; i < (*qp)->capacity; ++i) {
		STAILQ_INSERT_HEAD(&(*qp)->rp, &(*qp)->elm[i], link);
	}

	return 0;
}

static int
emu_term(struct xnvme_queue *q)
{
	struct xnvme_queue_emu *queue = (void *)q;

	qpair_term(queue->qp);

	return 0;
}

static int
emu_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	struct xnvme_queue_emu *queue = (void *)q;

	if (qpair_alloc(&(queue->qp), queue->base.capacity)) {
		XNVME_DEBUG("FAILED: qpair_alloc()");
		goto failed;
	}

	return 0;

failed:
	emu_term(q);

	return 1;
}

static int
emu_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_emu *queue = (void *)q;
	struct qpair *qp = queue->qp;
	unsigned completed = 0;

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;

	while (completed < max) {
		struct qpair_entry *entry = STAILQ_FIRST(&qp->sq);
		int err;

		STAILQ_REMOVE_HEAD(&qp->sq, link);

		err = entry->is_vectored
			      ? queue->base.dev->be.sync.cmd_iov(
					entry->ctx, entry->data, entry->data_vec_cnt,
					entry->data_nbytes, entry->meta, entry->meta_vec_cnt,
					entry->meta_nbytes)
			      : queue->base.dev->be.sync.cmd_io(entry->ctx, entry->data,
								entry->data_nbytes, entry->meta,
								entry->meta_nbytes);
		///< On submission-error; ctx.cpl is not filled, thus assigned below
		if (err) {
			entry->ctx->cpl.status.sc =
				entry->ctx->cpl.status.sc ? entry->ctx->cpl.status.sc : err;
			XNVME_DEBUG("FAILED: sync.cmd_io{v}(), err: %d", err);
		}

		entry->ctx->async.cb(entry->ctx, entry->ctx->async.cb_arg);
		STAILQ_INSERT_TAIL(&qp->rp, entry, link);

		++completed;
	};

	queue->base.outstanding -= completed;

	return completed;
}

static inline int
emu_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
	   size_t mbuf_nbytes)
{
	struct xnvme_queue_emu *queue = (void *)ctx->async.queue;
	struct qpair *qp = queue->qp;
	struct qpair_entry *entry;

	// Grab entry from rp and push into sq
	entry = STAILQ_FIRST(&qp->rp);
	if (!entry) {
		XNVME_DEBUG("FAILED: !STAILQ_FIRST(&qp->rp)");
		return -EIO;
	}
	STAILQ_REMOVE_HEAD(&qp->rp, link);

	entry->dev = ctx->dev;
	entry->ctx = ctx;

	entry->data = dbuf;
	entry->data_nbytes = dbuf_nbytes;
	entry->data_vec_cnt = 0;
	entry->meta = mbuf;
	entry->meta_nbytes = mbuf_nbytes;
	entry->meta_vec_cnt = 0;
	entry->is_vectored = false;

	STAILQ_INSERT_TAIL(&qp->sq, entry, link);

	ctx->async.queue->base.outstanding += 1;

	return 0;
}

static inline int
emu_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt, size_t dvec_nbytes,
	    struct iovec *mvec, size_t mvec_cnt, size_t mvec_nbytes)
{
	struct xnvme_queue_emu *queue = (void *)ctx->async.queue;
	struct qpair *qp = queue->qp;
	struct qpair_entry *entry;

	// Grab entry from rp and push into sq
	entry = STAILQ_FIRST(&qp->rp);
	if (!entry) {
		XNVME_DEBUG("FAILED: !STAILQ_FIRST(&qp->rp)");
		return -EIO;
	}
	STAILQ_REMOVE_HEAD(&qp->rp, link);

	entry->dev = ctx->dev;
	entry->ctx = ctx;

	entry->data = dvec;
	entry->data_nbytes = dvec_nbytes;
	entry->data_vec_cnt = dvec_cnt;
	entry->meta = mvec;
	entry->meta_nbytes = mvec_nbytes;
	entry->meta_vec_cnt = mvec_cnt;
	entry->is_vectored = true;

	STAILQ_INSERT_TAIL(&qp->sq, entry, link);

	ctx->async.queue->base.outstanding += 1;

	return 0;
}

#endif

struct xnvme_be_async g_xnvme_be_cbi_async_emu = {
	.id = "emu",
#ifdef XNVME_BE_CBI_ASYNC_EMU_ENABLED
	.cmd_io = emu_cmd_io,
	.cmd_iov = emu_cmd_iov,
	.poke = emu_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = emu_init,
	.term = emu_term,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
#endif
};
