// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_ASYNC_EMU_ENABLED
#include <errno.h>
#include <xnvme_queue.h>
#include <xnvme_dev.h>

struct _emu_entry {
	struct xnvme_dev *dev;
	struct xnvme_cmd_ctx *ctx;
	void *dbuf;
	size_t dbuf_nbytes;
	void *mbuf;
	size_t mbuf_nbytes;
	STAILQ_ENTRY(_emu_entry) link;
};

struct _emu_qp {
	STAILQ_HEAD(, _emu_entry) rp;	///< Request pool
	STAILQ_HEAD(, _emu_entry) sq;	///< Submission queue
	uint32_t capacity;
	struct _emu_entry elm[];
};

struct xnvme_queue_emu {
	struct xnvme_queue_base base;

	struct _emu_qp *qp;

	uint8_t _rsvd[224];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_queue_emu) == XNVME_BE_QUEUE_STATE_NBYTES,
	"Incorrect size"
)

int
_emu_qp_term(struct _emu_qp *qp)
{
	free(qp);

	return 0;
}

int
_emu_qp_alloc(struct _emu_qp **qp, uint32_t capacity)
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

int
_posix_async_emu_term(struct xnvme_queue *q)
{
	struct xnvme_queue_emu *queue = (void *)q;

	_emu_qp_term(queue->qp);

	return 0;
}

int
_posix_async_emu_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	struct xnvme_queue_emu *queue = (void *)q;

	if (_emu_qp_alloc(&(queue->qp), queue->base.capacity)) {
		XNVME_DEBUG("FAILED: _emu_qp_alloc()");
		goto failed;
	}

	return 0;

failed:
	_posix_async_emu_term(q);

	return 1;
}

int
_posix_async_emu_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_emu *queue = (void *)q;
	struct _emu_qp *qp = queue->qp;
	unsigned completed = 0;

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;

	while (completed < max) {
		struct _emu_entry *entry = STAILQ_FIRST(&qp->sq);
		int err;

		STAILQ_REMOVE_HEAD(&qp->sq, link);

		err = queue->base.dev->be.sync.cmd_io(entry->ctx, entry->dbuf, entry->dbuf_nbytes,
						      entry->mbuf, entry->mbuf_nbytes);
		if (err) {
			XNVME_DEBUG("FAILED: err: %d", err);
		}

		entry->ctx->async.cb(entry->ctx, entry->ctx->async.cb_arg);
		STAILQ_INSERT_TAIL(&qp->rp, entry, link);

		++completed;
	};

	queue->base.outstanding -= completed;

	return completed;
}

static inline int
_posix_async_emu_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			size_t mbuf_nbytes)
{
	struct xnvme_queue_emu *queue = (void *)ctx->async.queue;
	struct _emu_qp *qp = queue->qp;
	struct _emu_entry *entry;

	if (queue->base.outstanding == queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

	// Grab entry from rp and push into sq
	entry = STAILQ_FIRST(&qp->rp);
	if (!entry) {
		XNVME_DEBUG("FAILED: should not happen");
		return -EIO;
	}
	STAILQ_REMOVE_HEAD(&qp->rp, link);

	entry->dev = ctx->dev;
	entry->ctx = ctx;
	entry->dbuf = dbuf;
	entry->dbuf_nbytes = dbuf_nbytes;
	entry->mbuf = mbuf;
	entry->mbuf_nbytes = mbuf_nbytes;

	STAILQ_INSERT_TAIL(&qp->sq, entry, link);

	ctx->async.queue->base.outstanding += 1;

	return 0;
}
#endif

struct xnvme_be_async g_xnvme_be_posix_async_emu = {
	.id = "emu",
#ifdef XNVME_BE_ASYNC_EMU_ENABLED
	.cmd_io = _posix_async_emu_cmd_io,
	.poke = _posix_async_emu_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = _posix_async_emu_init,
	.term = _posix_async_emu_term,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
#endif
};
