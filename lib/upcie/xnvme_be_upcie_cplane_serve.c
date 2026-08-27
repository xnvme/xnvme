// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Serving clients of a runtime this process owns
 *
 * There is little here because the pieces are elsewhere: uPCIe defines what
 * passes between the two, and the backend knows how to describe a runtime and
 * how to create a queue. What is left is the bookkeeping nobody else can do,
 * which is remembering what each client holds so that a disconnect can release
 * it.
 *
 * A disconnect is the only reliable signal there is. A client that exits
 * cleanly hands its queues back; one that is killed mid-command does not, and
 * the socket closing is what says so.
 *
 * One socket serves everything this process holds, and every request names the
 * controller it is about. So a client has one connection however many
 * controllers it uses, the set of controllers is something the server answers
 * for rather than something read out of file names in /tmp, and a client that
 * wants a controller it has not heard of asks instead of guessing.
 *
 * One thread reads every connection. Requests are answered where they are read,
 * because all but one kind are memory: handing over the descriptors, carving a
 * buffer from the heap, giving one back, reporting what is held. A message is
 * taken as it arrives rather than waited for, so a client that writes half of
 * one and pauses costs a partly-filled buffer and nothing else. Replies go out
 * without waiting, and a client that has stopped reading them is let go, since
 * waiting for one peer is waiting for all of them.
 *
 * What cannot be answered there is anything that reaches the controller. An
 * admin command can run as long as a Format, and creating or deleting a queue
 * is a round trip of its own, so those go to a thread per controller. That is
 * the same granularity the admin queue already has, since its completions are
 * reaped in order and carry no record of who asked, so the queue of pending
 * work is what serialises them and no lock is needed for it.
 */
#include <errno.h>

#include <libxnvme.h>
#include <libxnvme_cplane.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

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
static struct {
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
static pthread_mutex_t serve_lock = PTHREAD_MUTEX_INITIALIZER;

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

/**
 * Allocate a queue for a client and describe it in offsets
 *
 * The memory and the controller's own record of the queue both come from here:
 * the submission and completion queues are carved from the heap and the
 * controller is told to create the pair over them. What comes back is kept in
 * the connection's own record, since that is who has to give it back.
 *
 * @param dev A device this process opened
 * @param depth Entries the client asked for
 * @param held Pre-allocated slot in the connection's record
 * @param out Pre-allocated allocation to fill
 *
 * @return 0 on success, negative errno on error
 */
static int
serve_ioqpair_alloc(struct xnvme_dev *dev, uint16_t depth, struct serve_ioqpair *held,
		    struct serve_qalloc *out)
{
	struct xnvme_be_upcie_state *state;
	struct nvme_controller *ctrl;
	int err;

	if (!dev || !held || !out || !depth) {
		return -EINVAL;
	}

	state = (void *)dev->be.state;
	ctrl = state->ctrlr->ctrl;

	memset(held, 0, sizeof(*held));
	err = nvme_controller_create_io_qpair_dmamem(ctrl, &held->qpair, depth,
						     &g_upcie_rte.mem.heap, &held->sq_offset,
						     &held->cq_offset, &held->prp_offset);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_create_io_qpair_dmamem(); err(%d)", err);
		return err;
	}

	memset(out, 0, sizeof(*out));
	out->sq_offset = held->sq_offset;
	out->cq_offset = held->cq_offset;
	out->prp_offset = held->prp_offset;
	out->qid = held->qpair.qid;
	out->depth = held->qpair.depth;

	return 0;
}

/**
 * Delete a queue allocated earlier and release what went with it
 *
 * @param dev A device this process opened
 * @param held The connection's record of the queue
 *
 * @return 0 on success, negative errno on error
 */
static int
serve_ioqpair_free(struct xnvme_dev *dev, struct serve_ioqpair *held)
{
	struct xnvme_be_upcie_state *state;
	struct nvme_controller *ctrl;
	int err;

	if (!dev || !held) {
		return -EINVAL;
	}

	state = (void *)dev->be.state;
	ctrl = state->ctrlr->ctrl;

	/* The queue goes before its memory does: the controller has to stop
	 * being able to reach an address before it stops resolving, which this
	 * does in one call.
	 *
	 * A failure there keeps both rather than returning them, so what is
	 * lost is one queue's memory for as long as this server runs. The
	 * record of it goes either way: the server has nothing further it can
	 * do with a queue the controller will not give up, and holding the slot
	 * would only stop the client's other queues from coming back. */
	err = nvme_controller_delete_io_qpair_dmamem(ctrl, &held->qpair, &g_upcie_rte.mem.heap,
						     held->sq_offset, held->cq_offset,
						     held->prp_offset);
	memset(held, 0, sizeof(*held));

	return err;
}

/**
 * Give a connection slot back to the table
 *
 * The one place a slot is reset, so the allocation list is released exactly
 * where the slot is, rather than at each of the three sites that free one.
 */
static void
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
static int
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
static void
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
static int
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
static int
serve_nclients_of(int devidx)
{
	int total = 0;

	for (int i = 0; i < SERVE_CONNS_MAX; ++i) {
		const struct serve_conn *conn = &serve.conns[i];

		total += ((conn->sock >= 0) && (conn->inited & (1U << (unsigned)devidx))) ? 1 : 0;
	}

	return total;
}

static int
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
static void
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
static void
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

/**
 * Answer the requests that reach the controller
 *
 * One per controller. An admin command runs with no lock held, so a Format
 * holds up this controller's queue and nothing else in the server. Creating
 * and deleting a queue is the exception, since the heap allocation and the
 * controller's own record of the queue are made in one call.
 */
static void *
serve_admin_main(void *arg)
{
	struct serve_admin *admin = arg;
	struct xnvme_dev *dev = serve.devs[admin->devidx];

	for (;;) {
		struct nvme_cplane_msg reply = {0};
		struct serve_conn *conn;

		pthread_mutex_lock(&admin->lock);
		while (!admin->nqueued && !admin->stop) {
			pthread_cond_wait(&admin->cond, &admin->lock);
		}
		if (!admin->nqueued && admin->stop) {
			pthread_mutex_unlock(&admin->lock);
			break;
		}
		conn = admin->queue[0];
		memmove(&admin->queue[0], &admin->queue[1],
			sizeof(admin->queue[0]) * (size_t)(--admin->nqueued));
		pthread_mutex_unlock(&admin->lock);

		if (conn->closing) {
			serve_conn_release(conn, admin->devidx);
			serve_wake();
			continue;
		}

		reply.op = conn->msg.op;
		reply.version = NVME_CPLANE_VERSION;

		switch (conn->msg.op) {
		case NVME_CPLANE_OP_ADMIN_CMD:
			if (nvme_cplane_admin_permitted(&conn->msg.u.admin.cmd)) {
				reply.status = xnvme_be_upcie_cplane_admin(
					dev, &conn->msg.u.admin.cmd, &reply.u.admin.cpl);
			} else {
				reply.status = -EPERM;
			}
			break;

		case NVME_CPLANE_OP_ALLOC_IOQPAIR: {
			struct serve_qalloc allocation = {0};

			pthread_mutex_lock(&serve_lock);
			reply.status =
				serve_ioqpair_alloc(dev, conn->msg.u.queue.depth,
						    &conn->qpairs[conn->nqpairs], &allocation);
			if (!reply.status) {
				conn->qpairs[conn->nqpairs++].dev = admin->devidx;
			}
			pthread_mutex_unlock(&serve_lock);
			if (reply.status) {
				break;
			}

			reply.u.queue.allocation.sq_offset = allocation.sq_offset;
			reply.u.queue.allocation.cq_offset = allocation.cq_offset;
			reply.u.queue.allocation.prp_offset = allocation.prp_offset;
			reply.u.queue.allocation.qid = allocation.qid;
			reply.u.queue.allocation.depth = allocation.depth;
		} break;

		case NVME_CPLANE_OP_FREE_IOQPAIR:
			reply.status = -ENOENT;
			pthread_mutex_lock(&serve_lock);
			for (int i = 0; i < conn->nqpairs; ++i) {
				/* On the controller it was asked of, since the
				 * identifier is that controller's own and two
				 * of them hand out the same numbers. */
				if ((conn->qpairs[i].dev != admin->devidx) ||
				    (conn->qpairs[i].qpair.qid != conn->msg.u.release.qid)) {
					continue;
				}

				reply.status = serve_ioqpair_free(dev, &conn->qpairs[i]);
				conn->qpairs[i] = conn->qpairs[--conn->nqpairs];
				break;
			}
			pthread_mutex_unlock(&serve_lock);
			break;

		default:
			reply.status = -ENOSYS;
			break;
		}

		/* Never waiting, and never taking the connection down here: the
		 * reader owns that, and ending the socket is how it is told. A
		 * peer that has stopped reading is treated as gone, since the
		 * buffer holds thousands of replies and nothing else fills
		 * it. */
		if (nvme_cplane_msg_trysend(conn->sock, &reply, NULL, 0)) {
			XNVME_DEBUG("INFO: reply went nowhere");
			shutdown(conn->sock, SHUT_RDWR);
		}

		pthread_mutex_lock(&serve_lock);
		conn->nread = 0;
		conn->owed = 0;
		pthread_mutex_unlock(&serve_lock);

		serve_wake();
	}

	return NULL;
}

/**
 * What serve_dispatch() leaves its caller to do
 */
enum serve_reply {
	SERVE_REPLY_NONE = 0, ///< Owed to a controller's thread; that answers it
	SERVE_REPLY_SEND,     ///< Send the reply and carry on
	SERVE_REPLY_LAST,     ///< Send it, then take the connection down
};

/**
 * Answer one request, or hand it on
 *
 * Callers hold serve_lock. Everything answered here is memory, so it is done
 * where it is read; anything that reaches the controller is owed to that
 * controller's thread instead.
 *
 * The reply is filled in rather than sent, because sending is a write to a
 * socket somebody else owns: a client that stops reading would otherwise hold
 * the lock that every other connection needs.
 *
 * @return One of enum serve_reply
 */
static enum serve_reply
serve_dispatch(struct serve_conn *conn, struct nvme_cplane_msg *out, int *fds, uint32_t *out_nfds)
{
	const struct xnvme_be_upcie_cplane_export *exported;
	struct nvme_cplane_msg reply = {0};
	const int idx = (int)conn->msg.index;
	uint32_t nfds = 0;

	reply.op = conn->msg.op;
	reply.version = NVME_CPLANE_VERSION;
	reply.index = conn->msg.index;

	/* A peer speaking another version is refused and then let go. Versions
	 * differ because the message does, so it is waiting for a reply of a
	 * size this does not send; leaving the connection open leaves it
	 * blocked on bytes that are not coming, where the socket ending tells
	 * it plainly. */
	if (conn->msg.version != NVME_CPLANE_VERSION) {
		XNVME_DEBUG("FAILED: client speaks version(%u)", conn->msg.version);
		reply.status = -EPROTO;
		conn->nread = 0;
		*out = reply;
		*out_nfds = 0;

		return SERVE_REPLY_LAST;
	}

	/* Everything but the heap operations is about one controller, and a
	 * client names it. Resolved once here, so that neither this nor a
	 * controller's thread indexes the device list on trust. */
	switch (conn->msg.op) {
	case NVME_CPLANE_OP_ALLOC_BUF:
	case NVME_CPLANE_OP_FREE_BUF:
		exported = NULL; ///< The heap is the runtime's; no controller named
		break;

	default:
		if ((idx < 0) || (idx >= serve.ndevs)) {
			reply.status = -ERANGE;
			conn->nread = 0;
			*out = reply;
			*out_nfds = 0;

			return SERVE_REPLY_SEND;
		}
		exported = &serve.exports[idx];
		break;
	}

	switch (conn->msg.op) {
	case NVME_CPLANE_OP_ADMIN_CMD:
	case NVME_CPLANE_OP_ALLOC_IOQPAIR:
	case NVME_CPLANE_OP_FREE_IOQPAIR:
		/* An offset is only meaningful to a client that mapped the
		 * heap, and mapping it is what initialising does. Without the
		 * gate an uninitialised connection could take a real queue off
		 * the controller and never be able to use it. */
		if ((conn->msg.op == NVME_CPLANE_OP_ALLOC_IOQPAIR) &&
		    !(conn->inited & (1U << (unsigned)idx))) {
			reply.status = -ENOTCONN;
			break;
		}
		if ((conn->msg.op == NVME_CPLANE_OP_ALLOC_IOQPAIR) &&
		    (conn->nqpairs == SERVE_IOQPAIRS_PER_CLIENT)) {
			reply.status = -ENOSPC;
			break;
		}

		serve_owe(conn, idx);

		return SERVE_REPLY_NONE; ///< The controller's thread answers this one

	case NVME_CPLANE_OP_INIT_CONNECTION:
		reply.u.init.record_offset = exported->record_offset;
		reply.u.init.heap_nbytes = exported->heap_nbytes;
		reply.u.init.bar0_nbytes = exported->bar0_nbytes;
		fds[nfds++] = exported->heap_fd;
		fds[nfds++] = exported->bar0_fd;
		conn->inited |= 1U << (unsigned)idx;
		break;

	case NVME_CPLANE_OP_ALLOC_BUF: {
		uint64_t offset = 0;

		/* The heap is the runtime's, so any controller having been
		 * initialised means this client has it mapped. */
		if (!conn->inited) {
			reply.status = -ENOTCONN;
			break;
		}

		reply.status = serve_conn_allocs_grow(conn);
		if (reply.status) {
			break;
		}

		reply.status = xnvme_be_upcie_cplane_alloc_buf(conn->msg.u.mem.nbytes, &offset);
		if (reply.status) {
			break;
		}

		conn->allocs[conn->nallocs++] = offset;
		reply.u.mem.offset = offset;
	} break;

	case NVME_CPLANE_OP_FREE_BUF:
		reply.status = -ENOENT;
		for (int i = 0; i < conn->nallocs; ++i) {
			if (conn->allocs[i] != conn->msg.u.mem.offset) {
				continue;
			}

			reply.status = xnvme_be_upcie_cplane_free_buf(conn->msg.u.mem.offset);
			conn->allocs[i] = conn->allocs[--conn->nallocs];
			break;
		}
		break;

	case NVME_CPLANE_OP_STATUS:
		reply.u.status.ndevs = (uint32_t)serve.ndevs;

		/* Asking is not connecting for I/O, so whoever asks does not
		 * count itself among the clients. It is not counted against the
		 * controller either, since asking initialises nothing. */
		reply.u.status.nclients = (uint32_t)(serve_nclients() - 1);
		reply.u.status.nclients_ctrlr = (uint32_t)serve_nclients_of(idx);
		reply.u.status.nqueues = (uint32_t)serve_nqueues(idx);
		reply.u.status.nsq_total = serve.nsq_total;
		reply.u.status.ncq_total = serve.ncq_total;
		snprintf(reply.u.status.bdf, sizeof(reply.u.status.bdf), "%s", exported->uri);
		break;

	default:
		reply.status = -ENOSYS;
		break;
	}

	reply.nfds = nfds;
	conn->nread = 0;
	*out = reply;
	*out_nfds = nfds;

	return SERVE_REPLY_SEND;
}

/**
 * Take a connection down
 *
 * A queue has to be given back to the controller that lent it, so the
 * connection is owed to each of those in turn and the last one to finish frees
 * the slot. Where it lent none there is nothing to tell any controller, and
 * buffers are heap, which this already holds the lock over.
 *
 * Callers hold serve_lock.
 */
static void
serve_drop(struct serve_conn *conn)
{
	uint32_t devs = 0;

	for (int i = 0; i < conn->nqpairs; ++i) {
		devs |= 1U << (unsigned)conn->qpairs[i].dev;
	}

	if (devs) {
		conn->closing = 1;
		conn->closing_devs = devs;

		for (int i = 0; i < serve.ndevs; ++i) {
			if (devs & (1U << (unsigned)i)) {
				serve_owe(conn, i);
			}
		}

		return;
	}

	for (int i = 0; i < conn->nallocs; ++i) {
		if (xnvme_be_upcie_cplane_free_buf(conn->allocs[i])) {
			XNVME_DEBUG("FAILED: reclaim(0x%" PRIx64 ")", conn->allocs[i]);
		}
	}

	close(conn->sock);
	serve_conn_wipe(conn);
}

int
xnvme_cplane_serve(struct xnvme_dev **devs, int ndevs, uint32_t cplane_id,
		   volatile sig_atomic_t *stop)
{
	struct xnvme_be_upcie_cplane_export exported[SERVE_DEVS_MAX] = {0};
	struct sockaddr_un addr = {.sun_family = AF_UNIX};
	char path[256] = {0};
	int listener = -1;
	int nexported = 0;
	int nadmin = 0;
	int err = 0;

	if (!devs || (ndevs < 1) || !stop) {
		return -EINVAL;
	}
	if (serve.running) {
		XNVME_DEBUG("FAILED: this process is already serving");
		return -EBUSY;
	}
	if (ndevs > SERVE_DEVS_MAX) {
		/* Distinct from the -ENOSYS below: that one says the backend
		 * shares its own way and the caller should just hold the
		 * devices. Holding them here would leave clients to open the
		 * controllers themselves, which closes them out from under the
		 * server. */
		XNVME_DEBUG("FAILED: serving %d devices; the cap is %d", ndevs, SERVE_DEVS_MAX);
		return -EOPNOTSUPP;
	}

	if (strcmp(devs[0]->be.attr.name, "upcie")) {
		XNVME_DEBUG("FAILED: serving is uPCIe's; be(%s) has its own arrangement",
			    devs[0]->be.attr.name);
		return -ENOSYS;
	}

	memset(&serve, 0, sizeof(serve));
	serve.running = 1;
	serve.devs = devs;
	serve.exports = exported;
	serve.ndevs = ndevs;
	serve.wake[0] = serve.wake[1] = -1;

	for (int i = 0; i < SERVE_CONNS_MAX; ++i) {
		serve.conns[i].sock = -1;
	}

	/* Non-blocking at both ends: the reader drains until empty, which on a
	 * blocking pipe is a read that never returns, and a controller's thread
	 * must not wait on a pipe that is already full of wake-ups it does not
	 * need to add to. */
	if (pipe2(serve.wake, O_NONBLOCK)) {
		serve.running = 0;
		return -errno;
	}

	for (nexported = 0; nexported < ndevs; ++nexported) {
		err = xnvme_be_upcie_cplane_export(devs[nexported], &exported[nexported]);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_be_upcie_cplane_export(%d); err(%d)", nexported,
				    err);
			goto exit;
		}
	}

	/* Asking is the server's to do, since a client has no controller to
	 * submit on. A refusal is not fatal: the totals stay zero, which the
	 * reply defines as "did not answer". */
	{
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(devs[0]);

		if (xnvme_adm_gfeat_nqueues(&ctx, &serve.nsq_total, &serve.ncq_total)) {
			XNVME_DEBUG("INFO: controller did not report its queue counts");
			serve.nsq_total = 0;
			serve.ncq_total = 0;
		}
	}

	/* One socket for everything this process holds. A request names the
	 * controller it is about, so nothing has to be encoded in the name and
	 * a client asks what is here rather than deducing it from /tmp. */
	xnvme_be_upcie_cplane_socket_path(cplane_id, path, sizeof(path));
	unlink(path);

	/* Non-blocking, because accept() is drained in a loop: on a blocking
	 * listener the call after the last waiting connection waits for one
	 * that is not coming, and the loop never gets back to poll(). */
	listener = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (listener < 0) {
		err = -errno;
		goto exit;
	}

	addr.sun_family = AF_UNIX;
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);

	/* Deep enough to absorb a burst of probes. Something asking whether a
	 * runtime is alive gets its answer from connecting, so a refused
	 * connection reads as a runtime that is not there. */
	if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) ||
	    listen(listener, SERVE_CONNS_MAX)) {
		err = -errno;
		goto exit;
	}

	for (nadmin = 0; nadmin < ndevs; ++nadmin) {
		struct serve_admin *admin = &serve.admin[nadmin];

		admin->devidx = nadmin;
		pthread_mutex_init(&admin->lock, NULL);
		pthread_cond_init(&admin->cond, NULL);
		if (pthread_create(&admin->tid, NULL, serve_admin_main, admin)) {
			err = -errno;
			XNVME_DEBUG("FAILED: pthread_create(admin %d); err(%d)", nadmin, err);
			goto exit;
		}
		admin->started = 1;
	}

	/* One reader for every connection: requests are memory, and what is not
	 * is owed to a controller's thread. */
	while (!*stop) {
		struct pollfd pfds[SERVE_CONNS_MAX + 2];
		struct serve_conn *polled[SERVE_CONNS_MAX];
		int npolled = 0;
		int nfds = 0;

		pfds[nfds].fd = listener;
		pfds[nfds].events = POLLIN;
		nfds++;

		pfds[nfds].fd = serve.wake[0];
		pfds[nfds].events = POLLIN;
		nfds++;

		pthread_mutex_lock(&serve_lock);
		for (int i = 0; i < SERVE_CONNS_MAX; ++i) {
			if ((serve.conns[i].sock < 0) || serve.conns[i].owed) {
				continue;
			}
			pfds[nfds].fd = serve.conns[i].sock;
			pfds[nfds].events = POLLIN;
			nfds++;
			polled[npolled++] = &serve.conns[i];
		}
		pthread_mutex_unlock(&serve_lock);

		/* A timeout rather than a signal mask: the caller's flag is
		 * what ends this, and a second of latency on the way out costs
		 * nobody anything. */
		if (poll(pfds, (nfds_t)nfds, 1000) < 0) {
			if (errno == EINTR) {
				continue;
			}
			err = -errno;
			break;
		}

		while (pfds[0].revents & POLLIN) {
			struct serve_conn *conn = NULL;
			int sock = accept(listener, NULL, NULL);

			if (sock < 0) {
				break; ///< Drained, or nothing was waiting
			}

			pthread_mutex_lock(&serve_lock);
			for (int s = 0; s < SERVE_CONNS_MAX; ++s) {
				if (serve.conns[s].sock < 0) {
					conn = &serve.conns[s];
					break;
				}
			}
			if (conn) {
				serve_conn_wipe(conn);
				conn->sock = sock;
			}
			pthread_mutex_unlock(&serve_lock);

			if (!conn) {
				close(sock);
			}
		}

		if (pfds[1].revents & POLLIN) {
			char drain[64];

			while (read(serve.wake[0], drain, sizeof(drain)) > 0) {
				;
			}
		}

		for (int i = 0; i < npolled; ++i) {
			struct serve_conn *conn = polled[i];
			short revents = pfds[2 + i].revents;
			struct nvme_cplane_msg reply = {0};
			int fds[NVME_CPLANE_FDS_MAX];
			enum serve_reply send;
			uint32_t nfds = 0;
			int sock, rc;

			if (!revents) {
				continue;
			}

			pthread_mutex_lock(&serve_lock);
			if ((conn->sock < 0) || conn->owed) {
				pthread_mutex_unlock(&serve_lock);
				continue;
			}

			rc = nvme_cplane_msg_recv_some(conn->sock, &conn->msg, &conn->nread, NULL,
						       NULL);
			if (rc == -EAGAIN) {
				pthread_mutex_unlock(&serve_lock);
				continue;
			}
			if (rc) {
				serve_drop(conn); ///< Gone, or spoke nonsense
				pthread_mutex_unlock(&serve_lock);
				continue;
			}

			send = serve_dispatch(conn, &reply, fds, &nfds);
			sock = conn->sock;
			pthread_mutex_unlock(&serve_lock);

			if (send == SERVE_REPLY_NONE) {
				continue; ///< A controller's thread has it
			}

			/* Sent with nothing held, and never waited on: a peer
			 * that has stopped reading its replies would otherwise
			 * stall the one thread every other client needs. A
			 * send that fails and a refusal that ends the
			 * conversation come to the same thing here. */
			if (nvme_cplane_msg_trysend(sock, &reply, nfds ? fds : NULL, nfds) ||
			    (send == SERVE_REPLY_LAST)) {
				pthread_mutex_lock(&serve_lock);
				if (conn->sock == sock) {
					serve_drop(conn);
				}
				pthread_mutex_unlock(&serve_lock);
			}
		}
	}

exit:
	/* Tell every client the server is going before taking its queues away.
	 * A client learns of a shutdown by its socket ending; delete its queues
	 * first and it is left polling a completion queue that no longer exists
	 * for a completion that cannot come. */
	pthread_mutex_lock(&serve_lock);
	for (int i = 0; i < SERVE_CONNS_MAX; ++i) {
		if (serve.conns[i].sock >= 0) {
			shutdown(serve.conns[i].sock, SHUT_RDWR);
		}
	}
	pthread_mutex_unlock(&serve_lock);

	for (int i = 0; i < nadmin; ++i) {
		struct serve_admin *admin = &serve.admin[i];

		if (!admin->started) {
			continue;
		}
		pthread_mutex_lock(&admin->lock);
		admin->stop = 1;
		pthread_cond_signal(&admin->cond);
		pthread_mutex_unlock(&admin->lock);
		pthread_join(admin->tid, NULL);
		pthread_mutex_destroy(&admin->lock);
		pthread_cond_destroy(&admin->cond);
	}

	/* Whatever is still connected is let go here, since the threads that
	 * would have done it are gone. Every controller in turn, because a
	 * connection may hold queues on several and the last one frees the
	 * slot. */
	for (int i = 0; i < SERVE_CONNS_MAX; ++i) {
		struct serve_conn *conn = &serve.conns[i];

		if (conn->sock < 0) {
			continue;
		}

		conn->closing_devs = 0;
		for (int q = 0; q < conn->nqpairs; ++q) {
			conn->closing_devs |= 1U << (unsigned)conn->qpairs[q].dev;
		}
		conn->closing_devs |= 1U; ///< So the last release is device zero's

		for (int d = 0; d < ndevs; ++d) {
			if (conn->closing_devs & (1U << (unsigned)d)) {
				serve_conn_release(conn, d);
			}
		}
	}

	if (listener >= 0) {
		close(listener);
		unlink(path);
	}
	for (int i = 0; i < nexported; ++i) {
		xnvme_be_upcie_cplane_unexport(&exported[i]);
	}
	for (int i = 0; i < 2; ++i) {
		if (serve.wake[i] >= 0) {
			close(serve.wake[i]);
		}
	}

	serve.running = 0;

	return err;
}
#else
int
xnvme_cplane_serve(struct xnvme_dev **XNVME_UNUSED(devs), int XNVME_UNUSED(ndevs),
		   uint32_t XNVME_UNUSED(cplane_id), volatile sig_atomic_t *XNVME_UNUSED(stop))
{
	return -ENOSYS;
}
#endif
