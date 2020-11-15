// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_queue.h>

int
xnvme_queue_term(struct xnvme_queue *queue)
{
	int err;

	if (!queue) {
		XNVME_DEBUG("FAILED: !dev");
		return -EINVAL;
	}

	err = queue->base.dev ? queue->base.dev->be.async.term(queue) : 0;
	if (err) {
		XNVME_DEBUG("FAILED: backend queue-termination failed with err: %d", err);
	}

	free(queue);

	return err;
}

int
xnvme_queue_init(struct xnvme_dev *dev, uint16_t depth, int opts, struct xnvme_queue **queue)
{
	int err;

	if (!dev) {
		XNVME_DEBUG("FAILED: !dev");
		return -EINVAL;
	}
	if (!(xnvme_is_pow2(depth) && (depth < 4096))) {
		XNVME_DEBUG("EINVAL: depth: %u", depth);
		return -EINVAL;
	}

	*queue = calloc(1, sizeof(**queue));
	if (!*queue) {
		XNVME_DEBUG("FAILED: calloc(queue), err: %s", strerror(errno));
		return -errno;
	}
	(*queue)->base.depth = depth;
	(*queue)->base.dev = dev;

	err = dev->be.async.init(*queue, opts);
	if (err) {
		XNVME_DEBUG("FAILED: backend-queue initialization with err: %d", err);
		free(*queue);
		return err;
	}

	return 0;
}

int
xnvme_queue_poke(struct xnvme_queue *queue, uint32_t max)
{
	if (!queue->base.outstanding) {
		return 0;
	}

	return queue->base.dev->be.async.poke(queue, max);
}

int
xnvme_queue_wait(struct xnvme_queue *queue)
{
	return queue->base.dev->be.async.wait(queue);
}

uint32_t
xnvme_queue_get_depth(struct xnvme_queue *queue)
{
	return queue->base.depth;
}

uint32_t
xnvme_queue_get_outstanding(struct xnvme_queue *queue)
{
	return queue->base.outstanding;
}
