// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_H
#define __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_H
#include <libxnvme.h>
#include <libxnvme_cplane.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_cplane_serve_conn.h>

/**
 * Connections one server holds at once; one per client process
 */
#define XNVME_BE_UPCIE_CPLANE_SERVE_CONNS_MAX 64

/**
 * Controllers one server serves; a connection tracks them a bit apiece
 */
#define XNVME_BE_UPCIE_CPLANE_SERVE_DEVS_MAX XNVME_CPLANE_MAX_CTRLRS
XNVME_STATIC_ASSERT(XNVME_BE_UPCIE_CPLANE_SERVE_DEVS_MAX <= 64,
		    "XNVME_BE_UPCIE_CPLANE_SERVE_DEVS_MAX exceeds the connection bitmaps")

/**
 * A controller's thread, and the connections waiting on it
 *
 * The one thread per controller is what serialises the requests that reach
 * that controller's admin queue.
 */
struct xnvme_be_upcie_cplane_serve_admin {
	pthread_t tid;
	pthread_mutex_t lock; ///< Guards `queue`, `nqueued` and `stop`
	pthread_cond_t cond;
	struct xnvme_be_upcie_cplane_serve_conn *queue[XNVME_BE_UPCIE_CPLANE_SERVE_CONNS_MAX];
	int nqueued;
	int stop;
	int started;
	int devidx;
};

/**
 * Everything one server holds; one server per process
 *
 * `wake` is a pipe a controller's thread writes when it has answered, so the
 * reader looks at the connection again without waiting out its poll.
 * `nsq_total` and `ncq_total` are asked of the controller once, when serving
 * starts, so a status request costs no admin command.
 */
extern struct xnvme_be_upcie_cplane_serve_state {
	struct xnvme_dev **devs;
	const struct xnvme_be_upcie_cplane_share *shares;
	int ndevs;

	struct xnvme_be_upcie_cplane_serve_conn conns[XNVME_BE_UPCIE_CPLANE_SERVE_CONNS_MAX];
	struct xnvme_be_upcie_cplane_serve_admin admin[XNVME_BE_UPCIE_CPLANE_SERVE_DEVS_MAX];

	int wake[2];        ///< Read and write end of the reader's wake-up pipe
	uint32_t nsq_total; ///< I/O submission queues the controller allocated
	uint32_t ncq_total; ///< I/O completion queues the controller allocated
	int running;        ///< Whether a server runs in this process
} g_upcie_cplane_serve;

/**
 * Guards the connection table
 *
 * Taken outside xnvme_lock(), never inside it. It is not xnvme_lock() itself,
 * since that is held for as long as an admin command runs, and the reader takes
 * this for every request.
 */
extern pthread_mutex_t g_upcie_cplane_serve_lock;

/**
 * Tell the reader that a controller's thread has answered a connection
 */
void
xnvme_be_upcie_cplane_serve_wake(void);

/**
 * Hand `conn` to the thread of controller `devidx`; callers hold g_upcie_cplane_serve_lock
 */
void
xnvme_be_upcie_cplane_serve_owe(struct xnvme_be_upcie_cplane_serve_conn *conn, int devidx);

#endif /* XNVME_BE_UPCIE_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_H */
