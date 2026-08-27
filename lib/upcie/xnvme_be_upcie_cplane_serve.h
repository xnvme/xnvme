// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * What the pieces of the server share
 *
 * The server is split by what a reader is looking for: what a client holds
 * and how it comes back, how a queue is taken off a controller, and the
 * sockets and threads around both. This is the state those have in common.
 */
#ifndef __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_H
#define __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_H
#include <libxnvme.h>
#include <libxnvme_cplane.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_be_upcie.h>
/**
 * Connections one server will hold at once
 *
 * One per client process rather than one per client and controller, since a
 * client reaches every controller over the one socket. A connection is not a
 * thread either, so this bounds a table rather than anything the machine has
 * to carry.
 *
 * A flat number, because nothing about the hardware predicts it. How many
 * queues the controllers have says how many a client can be given, which is
 * what the per-connection limit is for; it says nothing about how many clients
 * there are, since most take one queue and some take none at all.
 */
#define SERVE_CONNS_MAX 64

/**
 * Controllers one server will serve
 *
 * The ceiling is the bitmap: a connection tracks which controllers it has
 * initialised, and which still owe it cleanup, one bit apiece in a uint32_t.
 */
#define SERVE_DEVS_MAX 16

/* A connection tracks controllers one bit apiece, in a uint32_t, so raising
 * the ceiling past what that holds would shift past the end and lose the
 * tracking rather than fail to build. */
XNVME_STATIC_ASSERT(SERVE_DEVS_MAX <= 32, "SERVE_DEVS_MAX exceeds the connection bitmaps")

/**
 * Queues one connection may hold, across every controller it uses
 *
 * Per client rather than per client and controller, which is what a connection
 * now is. A process driving several controllers wants a queue on each, often
 * one per thread, so this has to cover the whole spread and not a single
 * controller's share of it.
 */
#define SERVE_IOQPAIRS_PER_CLIENT 64

/**
 * Allocations a connection starts room for, and grows from
 *
 * Not a ceiling: a client allocates a buffer per queue entry, so the count
 * follows its queue depth and its queue count, and any constant here is a
 * guess at somebody else's configuration. qublk allocates one per tag at
 * startup, which is nqueues times qdepth and is 64 before anyone asks for
 * anything unusual. What bounds this is the heap, which refuses when it is
 * full.
 */
#define SERVE_ALLOCS_INITIAL 32

/**
 * A queue this client was given, and what it takes to give it back
 *
 * Kept with the connection rather than in a table of its own: a queue belongs
 * to exactly one client for exactly as long as that client does, so the
 * connection record is where the lookup already is.
 */
struct serve_ioqpair {
	struct nvme_qpair qpair;
	size_t sq_offset;
	size_t cq_offset;
	size_t prp_offset;

	/* Which controller it came off. Queue identifiers are the controller's
	 * own, so two of them hand out the same one, and a client holding both
	 * would otherwise give back whichever matched first. */
	int dev;
};

/**
 * One client connection
 *
 * `nread` is what makes a single reader safe: a request is accumulated across
 * as many wake-ups as it takes, so a peer that stalls mid-message is simply a
 * connection that is not ready yet.
 *
 * `owed` says the connection has been handed to its controller's thread and is
 * not to be read from meanwhile, since that thread is the one that will answer.
 */
struct serve_conn {
	int sock; ///< -1 when the slot is free

	/* Controllers this connection has been given the descriptors for, one
	 * bit apiece. The heap comes with the first and is the same every
	 * time; BAR0 is the controller's own, so it is per bit. */
	uint32_t inited;

	struct serve_ioqpair qpairs[SERVE_IOQPAIRS_PER_CLIENT];
	int nqpairs;
	uint64_t *allocs; ///< Heap offsets handed out, grown as they are
	int nallocs;
	int allocs_cap;

	struct nvme_cplane_msg msg; ///< The request being read
	size_t nread;               ///< How much of it has arrived

	int owed;     ///< Handed to a controller's thread; not to be read from
	int owed_dev; ///< Which thread has it

	/* Gone. Its queues are for the controllers that lent them to release,
	 * one bit apiece; the thread that clears the last bit frees the slot.
	 * Two threads must not both be deleting queues on one controller, so
	 * each is asked separately rather than one doing it for all. */
	int closing;
	uint32_t closing_devs;
};

/**
 * A controller's thread, and the connections waiting on it
 *
 * The queue is what serialises the admin queue, which is why there is no lock
 * over it: one thread drains it, so two commands are never in flight on one
 * controller.
 */
struct serve_admin {
	pthread_t tid;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	struct serve_conn *queue[SERVE_CONNS_MAX];
	int nqueued;
	int stop;
	int started;
	int devidx;
};

/**
 * Everything one server holds, so that any connection can answer for all of it
 *
 * A status request may ask about a controller other than the one it connected
 * to, and the counts it reports are of connections and queues held elsewhere in
 * the table. Both need the whole picture, and this is it.
 */
extern struct serve_state {
	struct xnvme_dev **devs;
	const struct xnvme_be_upcie_cplane_export *exports;
	int ndevs;

	struct serve_conn conns[SERVE_CONNS_MAX];
	struct serve_admin admin[SERVE_DEVS_MAX];

	/* Written by a controller's thread when it has finished with a
	 * connection, read by the reader, so that a reply does not wait for the
	 * poll timeout before the next request is looked at. */
	int wake[2];

	/* What the controller allocated, asked once: it cannot change while
	 * the controller is up, and a status probe should not cost an admin
	 * command. */
	uint32_t nsq_total;
	uint32_t ncq_total;

	/* Process-wide, so two servers in one process would report each
	 * other's numbers. One at a time. */
	int running;
} serve;

/**
 * Guards the connection table and the DMA heap
 *
 * One heap serves the whole runtime and its free list has no lock of its own,
 * and the table is one table. Both are short to touch.
 *
 * A controller's thread never holds it across an admin command, so a Format
 * holds nothing here. It does hold it across creating and deleting a queue,
 * because allocating the memory and telling the controller about it are one
 * call, and the allocation is the heap's. That bounds the reader by a Create
 * or Delete round trip rather than by an arbitrary command, which is worth
 * knowing but has not been worth splitting the call in two for.
 */
extern pthread_mutex_t serve_lock;

/**
 * What a client is told about a queue allocated for it
 *
 * Plain offsets on purpose: the queue itself stays here, so the reply carries
 * nothing the client would have to be a uPCIe runtime to resolve.
 */
struct serve_qalloc {
	uint64_t sq_offset;  ///< Submission queue, as a heap offset
	uint64_t cq_offset;  ///< Completion queue, as a heap offset
	uint64_t prp_offset; ///< Scratch for the client's request pool
	uint32_t qid;        ///< The identifier allocated
	uint16_t depth;      ///< Entries in the queue pair
};

int
serve_ioqpair_alloc(struct xnvme_dev *dev, uint16_t depth, struct serve_ioqpair *held,
		    struct serve_qalloc *out);

int
serve_ioqpair_free(struct xnvme_dev *dev, struct serve_ioqpair *held);

void
serve_conn_wipe(struct serve_conn *conn);

int
serve_conn_allocs_grow(struct serve_conn *conn);

void
serve_conn_release(struct serve_conn *conn, int devidx);

int
serve_nclients(void);

int
serve_nclients_of(int devidx);

int
serve_nqueues(int devidx);

void
serve_wake(void);

void
serve_owe(struct serve_conn *conn, int devidx);

#endif /* XNVME_BE_UPCIE_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_H */
