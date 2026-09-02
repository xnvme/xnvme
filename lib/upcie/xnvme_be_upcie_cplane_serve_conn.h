// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_CONN_H
#define __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_CONN_H
#include <libxnvme.h>
#include <xnvme_be.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_cplane_serve_ioqpair.h>

/**
 * Queues one connection may hold, across every controller it uses
 */
#define XNVME_BE_UPCIE_CPLANE_SERVE_IOQPAIRS_PER_CLIENT 64

/**
 * Allocations a connection has room for before it grows the list
 */
#define XNVME_BE_UPCIE_CPLANE_SERVE_ALLOCS_INITIAL 32

/**
 * A region of a client's own memory, and the controller `dev` it was made reachable for
 */
struct xnvme_be_upcie_cplane_serve_registration {
	struct xnvme_be_upcie_cplane_registration reg;
	int dev;
};

/**
 * One client connection, and everything the client was given
 *
 * `inited` and `closing_devs` hold a bit per controller index. While `owed` is
 * set the connection belongs to the thread of controller `owed_dev`, which
 * answers its request, and the reader leaves it alone.
 *
 * A closed connection is `closing`; the thread of each controller in
 * `closing_devs` deletes the queues it lent, since one thread per controller
 * keeps two deletions off one admin queue, and the thread clearing the last
 * bit frees the slot.
 */
struct xnvme_be_upcie_cplane_serve_conn {
	int sock;        ///< -1 when the slot is free
	uint64_t inited; ///< Controllers whose descriptors it was given

	struct xnvme_be_upcie_cplane_serve_ioqpair
		qpairs[XNVME_BE_UPCIE_CPLANE_SERVE_IOQPAIRS_PER_CLIENT];
	int nqpairs;
	uint64_t *allocs; ///< Heap offsets handed out
	int nallocs;
	int allocs_cap;
	struct xnvme_be_upcie_cplane_serve_registration *regs; ///< Client memory made reachable
	int nregs;
	int regs_cap;

	int in_fd;                  ///< Descriptor the request came with; -1 for none
	struct nvme_cplane_msg msg; ///< The request being answered

	int owed;     ///< Handed to a controller's thread
	int owed_dev; ///< Which controller's thread has it

	int closing;           ///< The client went away
	uint64_t closing_devs; ///< Controllers yet to release what they lent
};

/**
 * Free the allocation list of `conn` and mark its slot free
 */
void
xnvme_be_upcie_cplane_serve_conn_wipe(struct xnvme_be_upcie_cplane_serve_conn *conn);

/**
 * Make room on `conn` for one more registration
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_serve_conn_regs_grow(struct xnvme_be_upcie_cplane_serve_conn *conn);

/**
 * Make room on `conn` for one more allocation
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_serve_conn_allocs_grow(struct xnvme_be_upcie_cplane_serve_conn *conn);

/**
 * Delete the queues controller `devidx` lent to a closed `conn`, on that controller's thread
 *
 * The call that clears the last bit of `closing_devs` also frees the
 * connection's allocations and its slot. Callers hold nothing.
 */
void
xnvme_be_upcie_cplane_serve_conn_release(struct xnvme_be_upcie_cplane_serve_conn *conn,
					 int devidx);

/**
 * Connections in the table; callers hold g_upcie_cplane_serve_lock
 */
int
xnvme_be_upcie_cplane_serve_conn_count(void);

/**
 * Connections given the descriptors of controller `devidx`; callers hold g_upcie_cplane_serve_lock
 */
int
xnvme_be_upcie_cplane_serve_conn_count_of(int devidx);

/**
 * Queues the connections hold on controller `devidx`; callers hold g_upcie_cplane_serve_lock
 */
int
xnvme_be_upcie_cplane_serve_conn_nqueues(int devidx);

#endif /* XNVME_BE_UPCIE_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_CONN_H */
