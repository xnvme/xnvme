// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * What a client holds, and what happens when it stops
 *
 * A disconnect is the only reliable signal there is, so what each client was
 * given is remembered here and released from one place. The counters are the
 * server's account of itself, which is what a test can assert against.
 */
#include <errno.h>

#include <libxnvme.h>
#include <libxnvme_cplane.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_cplane_serve.h>

/**
 * Give a connection slot back to the table
 *
 * The one place a slot is reset, so the allocation list is released exactly
 * where the slot is, rather than at each of the three sites that free one.
 */
void
serve_conn_wipe(struct serve_conn *conn)
{
	free(conn->allocs);
	memset(conn, 0, sizeof(*conn));
	conn->sock = -1;
}

/**
 * Make room for one more allocation on this connection
 *
 * @return 0 when there is room, negative errno when there cannot be
 */
int
serve_conn_allocs_grow(struct serve_conn *conn)
{
	uint64_t *grown;
	int cap;

	if (conn->nallocs < conn->allocs_cap) {
		return 0;
	}

	cap = conn->allocs_cap ? conn->allocs_cap * 2 : SERVE_ALLOCS_INITIAL;
	grown = realloc(conn->allocs, sizeof(*grown) * (size_t)cap);
	if (!grown) {
		return -ENOMEM;
	}

	conn->allocs = grown;
	conn->allocs_cap = cap;

	return 0;
}

/**
 * Give back what one controller lent a departing connection
 *
 * Runs on that controller's thread, since giving a queue back means telling
 * the controller so, and one thread per controller is what keeps two deletions
 * off one admin queue. A connection holding queues on several is asked of each
 * in turn, and whichever finishes last frees the slot.
 *
 * Callers hold nothing.
 */
void
serve_conn_release(struct serve_conn *conn, int devidx)
{
	struct xnvme_dev *dev = serve.devs[devidx];
	int last;

	/* Queues first: the controller has to stop being able to reach an
	 * address before that address stops meaning anything. */
	for (int i = 0; i < conn->nqpairs;) {
		int err;

		if (conn->qpairs[i].dev != devidx) {
			++i;
			continue;
		}

		pthread_mutex_lock(&serve_lock);
		err = serve_ioqpair_free(dev, &conn->qpairs[i]);
		conn->qpairs[i] = conn->qpairs[--conn->nqpairs];
		pthread_mutex_unlock(&serve_lock);

		if (err) {
			XNVME_DEBUG("FAILED: free(slot(%d)); err(%d)", i, err);
		}
	}

	pthread_mutex_lock(&serve_lock);
	conn->closing_devs &= ~(1U << (unsigned)devidx);
	last = !conn->closing_devs;
	pthread_mutex_unlock(&serve_lock);

	if (!last) {
		return; ///< Another controller has yet to be told
	}

	for (int i = 0; i < conn->nallocs; ++i) {
		int err;

		pthread_mutex_lock(&serve_lock);
		err = xnvme_be_upcie_cplane_free_buf(conn->allocs[i]);
		pthread_mutex_unlock(&serve_lock);

		if (err) {
			XNVME_DEBUG("FAILED: reclaim(0x%" PRIx64 "); err(%d)", conn->allocs[i],
				    err);
		}
	}

	pthread_mutex_lock(&serve_lock);
	if (conn->sock >= 0) {
		close(conn->sock);
	}
	serve_conn_wipe(conn);
	pthread_mutex_unlock(&serve_lock);
}

/**
 * Connections in hand, and queues held on one controller
 *
 * Walked rather than counted up as things happen: a tally maintained by hand
 * has to be right in every path that adds or drops a connection, including the
 * ones that fail halfway, and it was wrong that way once already. A slot with
 * no socket is free, which is what release leaves behind, so the walk needs no
 * second flag to tell live from finished. Callers hold serve_lock.
 */
int
serve_nclients(void)
{
	int total = 0;

	for (int i = 0; i < SERVE_CONNS_MAX; ++i) {
		total += (serve.conns[i].sock >= 0) ? 1 : 0;
	}

	return total;
}

/**
 * Connections that have been given this controller's descriptors
 */
int
serve_nclients_of(int devidx)
{
	int total = 0;

	for (int i = 0; i < SERVE_CONNS_MAX; ++i) {
		const struct serve_conn *conn = &serve.conns[i];

		total += ((conn->sock >= 0) && (conn->inited & (1U << (unsigned)devidx))) ? 1 : 0;
	}

	return total;
}

int
serve_nqueues(int devidx)
{
	int total = 0;

	if ((devidx < 0) || (devidx >= serve.ndevs)) {
		return 0;
	}

	for (int i = 0; i < SERVE_CONNS_MAX; ++i) {
		const struct serve_conn *conn = &serve.conns[i];

		if (conn->sock < 0) {
			continue;
		}
		for (int q = 0; q < conn->nqpairs; ++q) {
			total += (conn->qpairs[q].dev == devidx) ? 1 : 0;
		}
	}

	return total;
}

/**
 * Tell the reader a connection is its own again
 */
void
serve_wake(void)
{
	const char one = 'x';
	ssize_t nbytes;

	do {
		nbytes = write(serve.wake[1], &one, 1);
	} while ((nbytes < 0) && (errno == EINTR));

	/* A full pipe is a wake-up already pending, which is all this needed. */
	(void)nbytes;
}

/**
 * Hand a connection to a controller's thread
 *
 * Callers hold serve_lock, so that the connection cannot be read from again
 * before the thread has answered.
 */
void
serve_owe(struct serve_conn *conn, int devidx)
{
	struct serve_admin *admin = &serve.admin[devidx];

	conn->owed = 1;
	conn->owed_dev = devidx;

	pthread_mutex_lock(&admin->lock);
	admin->queue[admin->nqueued++] = conn;
	pthread_cond_signal(&admin->cond);
	pthread_mutex_unlock(&admin->lock);
}

#endif /* XNVME_BE_UPCIE_ENABLED */
