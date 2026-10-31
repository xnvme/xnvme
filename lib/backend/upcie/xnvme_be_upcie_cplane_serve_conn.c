// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/*
 * What the server holds for each client connection, and its release.
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
#include <xnvme_be_upcie_cplane_serve_conn.h>
#include <xnvme_be_upcie_cplane_serve_mem.h>

void
xnvme_be_upcie_cplane_serve_conn_wipe(struct xnvme_be_upcie_cplane_serve_conn *conn)
{
	if (conn->in_fd >= 0) {
		close(conn->in_fd);
	}
	free(conn->allocs);
	free(conn->regs);
	memset(conn, 0, sizeof(*conn));
	conn->sock = -1;
	conn->in_fd = -1;
}

int
xnvme_be_upcie_cplane_serve_conn_regs_grow(struct xnvme_be_upcie_cplane_serve_conn *conn)
{
	struct xnvme_be_upcie_cplane_serve_registration *grown;
	int cap;

	if (conn->nregs < conn->regs_cap) {
		return 0;
	}

	cap = conn->regs_cap ? conn->regs_cap * 2 : 4;
	grown = realloc(conn->regs, sizeof(*grown) * (size_t)cap);
	if (!grown) {
		return -ENOMEM;
	}

	conn->regs = grown;
	conn->regs_cap = cap;

	return 0;
}

int
xnvme_be_upcie_cplane_serve_conn_allocs_grow(struct xnvme_be_upcie_cplane_serve_conn *conn)
{
	uint64_t *grown;
	int cap;

	if (conn->nallocs < conn->allocs_cap) {
		return 0;
	}

	cap = conn->allocs_cap ? conn->allocs_cap * 2 : XNVME_BE_UPCIE_CPLANE_SERVE_ALLOCS_INITIAL;
	grown = realloc(conn->allocs, sizeof(*grown) * (size_t)cap);
	if (!grown) {
		return -ENOMEM;
	}

	conn->allocs = grown;
	conn->allocs_cap = cap;

	return 0;
}

static int
_conn_reg_find(struct xnvme_be_upcie_cplane_serve_conn *conn, int devidx)
{
	for (int i = 0; i < conn->nregs; ++i) {
		if (conn->regs[i].dev == devidx) {
			return i;
		}
	}

	return -1;
}

void
xnvme_be_upcie_cplane_serve_conn_release(struct xnvme_be_upcie_cplane_serve_conn *conn, int devidx)
{
	struct xnvme_dev *dev = g_upcie_cplane_serve.devs[devidx];
	int last;

	/* Queues first, so the controller stops reaching memory before it is freed.
	 * Every controller the connection used releases it at once, and each
	 * removal moves another entry, so each one is found again under the lock. */
	for (;;) {
		int i, err;

		pthread_mutex_lock(&g_upcie_cplane_serve_lock);
		for (i = 0; (i < conn->nqpairs) && (conn->qpairs[i].dev != devidx); ++i) {
			;
		}
		if (i == conn->nqpairs) {
			pthread_mutex_unlock(&g_upcie_cplane_serve_lock);
			break;
		}
		err = xnvme_be_upcie_cplane_serve_ioqpair_free(dev, &conn->qpairs[i]);
		conn->qpairs[i] = conn->qpairs[--conn->nqpairs];
		pthread_mutex_unlock(&g_upcie_cplane_serve_lock);

		if (err) {
			XNVME_DEBUG("FAILED: free(slot(%d)); err(%d)", i, err);
		}
	}

	for (;;) {
		int i;

		pthread_mutex_lock(&g_upcie_cplane_serve_lock);
		i = _conn_reg_find(conn, devidx);
		if (i < 0) {
			pthread_mutex_unlock(&g_upcie_cplane_serve_lock);
			break;
		}
		xnvme_be_upcie_cplane_serve_unregister_mem(&conn->regs[i].reg);
		conn->regs[i] = conn->regs[--conn->nregs];
		pthread_mutex_unlock(&g_upcie_cplane_serve_lock);
	}

	pthread_mutex_lock(&g_upcie_cplane_serve_lock);
	conn->closing_devs &= ~(UINT64_C(1) << (unsigned)devidx);
	last = !conn->closing_devs;
	pthread_mutex_unlock(&g_upcie_cplane_serve_lock);

	if (!last) {
		return; /* Another controller has yet to release */
	}

	for (int i = 0; i < conn->nallocs; ++i) {
		int err;

		pthread_mutex_lock(&g_upcie_cplane_serve_lock);
		err = xnvme_be_upcie_cplane_serve_buf_free(conn->allocs[i]);
		pthread_mutex_unlock(&g_upcie_cplane_serve_lock);

		if (err) {
			XNVME_DEBUG("FAILED: reclaim(0x%" PRIx64 "); err(%d)", conn->allocs[i],
				    err);
		}
	}

	pthread_mutex_lock(&g_upcie_cplane_serve_lock);
	if (conn->sock >= 0) {
		close(conn->sock);
	}
	xnvme_be_upcie_cplane_serve_conn_wipe(conn);
	pthread_mutex_unlock(&g_upcie_cplane_serve_lock);
}

/* Counted by walking the table, which stays right on every path that fails halfway */
int
xnvme_be_upcie_cplane_serve_conn_count(void)
{
	int total = 0;

	for (int i = 0; i < XNVME_BE_UPCIE_CPLANE_SERVE_CONNS_MAX; ++i) {
		total += (g_upcie_cplane_serve.conns[i].sock >= 0) ? 1 : 0;
	}

	return total;
}

int
xnvme_be_upcie_cplane_serve_conn_count_of(int devidx)
{
	int total = 0;

	for (int i = 0; i < XNVME_BE_UPCIE_CPLANE_SERVE_CONNS_MAX; ++i) {
		const struct xnvme_be_upcie_cplane_serve_conn *conn =
			&g_upcie_cplane_serve.conns[i];

		total += ((conn->sock >= 0) && (conn->inited & (UINT64_C(1) << (unsigned)devidx)))
				 ? 1
				 : 0;
	}

	return total;
}

int
xnvme_be_upcie_cplane_serve_conn_nqueues(int devidx)
{
	int total = 0;

	if ((devidx < 0) || (devidx >= g_upcie_cplane_serve.ndevs)) {
		return 0;
	}

	for (int i = 0; i < XNVME_BE_UPCIE_CPLANE_SERVE_CONNS_MAX; ++i) {
		const struct xnvme_be_upcie_cplane_serve_conn *conn =
			&g_upcie_cplane_serve.conns[i];

		if (conn->sock < 0) {
			continue;
		}
		for (int q = 0; q < conn->nqpairs; ++q) {
			total += (conn->qpairs[q].dev == devidx) ? 1 : 0;
		}
	}

	return total;
}

#endif /* XNVME_BE_UPCIE_ENABLED */
