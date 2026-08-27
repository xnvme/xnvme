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
 */
#include <errno.h>

#include <libxnvme.h>
#include <libxnvme_cplane.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
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
 * Connections one controller will hold at once, and so threads
 *
 * A connection is a thread, so this is the server's own ceiling rather than a
 * statement about what is useful. The useful ceiling comes from the hardware:
 * a client is there to drive queues, and a controller has only so many, so
 * serve_clients_max() sizes this from what the controller reports and falls
 * back to this when it reports nothing.
 *
 * Queues alone would be the wrong number. A client may hold several, so the
 * queues run out first; and plenty of clients want none at all, since asking
 * for status or submitting an admin command needs no queue. So the count is
 * what the queues can support, plus room for the ones that hold none.
 */
#define SERVE_CLIENTS_MAX 32
#define SERVE_CLIENTS_QUEUELESS 8

/**
 * Controllers one server will serve
 *
 * One socket and one export per controller, since the control plane protocol has
 * nowhere to name a controller and the identifier alone cannot say which one a
 * client wants.
 */
#define SERVE_DEVS_MAX 16
#define SERVE_IOQPAIRS_PER_CLIENT 8

#define SERVE_ALLOCS_PER_CLIENT 32

/**
 * A queue this client was given, and what it takes to give it back
 *
 * Kept with the client rather than in a table of its own: a queue belongs to
 * exactly one client for exactly as long as that client does, so the client
 * record is where the lookup already is.
 */
struct serve_ioqpair {
	struct nvme_qpair qpair;
	size_t sq_offset;
	size_t cq_offset;
	size_t prp_offset;
};

struct serve_client {
	int sock;
	int dev;         ///< Index into the server's device list; what this client reached
	int initialized; ///< Whether NVME_CPLANE_OP_INIT_CONNECTION has been answered
	struct serve_ioqpair qpairs[SERVE_IOQPAIRS_PER_CLIENT];
	int nqpairs;
	uint64_t allocs[SERVE_ALLOCS_PER_CLIENT]; ///< Heap offsets handed out
	int nallocs;
};

/**
 * Everything one server holds, so that any connection can answer for all of it
 *
 * A status request may ask about a controller other than the one it connected
 * to, and the counts it reports are of connections and queues that belong to
 * other threads. Both need the whole picture, and this is it.
 */
/**
 * One client connection, served by one thread
 *
 * A thread apiece, so a request is read by blocking on the socket it came
 * from. A client that stalls mid-message holds its own thread and nothing
 * else, and a command that runs as long as a Format holds this thread and the
 * admin queue it needs, while every other connection carries on.
 */
struct serve_conn {
	struct xnvme_dev *dev;
	const struct xnvme_be_upcie_cplane_export *exported;
	struct serve_client client;
	pthread_t tid;
	int live;
};

/**
 * One controller's listeners, and the connections accepted on them
 *
 * The listeners a worker owns are its controller's, plus the runtime-wide one
 * on the first, since that answers for the runtime rather than for a
 * controller.
 */
struct serve_worker {
	struct xnvme_dev *dev;
	int devidx;
	const struct xnvme_be_upcie_cplane_export *exported;
	int listeners[2];
	int nlisteners;
	volatile sig_atomic_t *stop;
	struct serve_conn conns[SERVE_CLIENTS_MAX];
	pthread_t tid;
	int started;
};

static struct {
	struct xnvme_dev **devs;
	const struct xnvme_be_upcie_cplane_export *exports;
	struct serve_worker *workers;
	int ndevs;

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
 * How many connections one controller will hold at once
 *
 * Sized from what the controller said it has, since a client that cannot be
 * given a queue cannot do the thing a client is for. The allowance on top is
 * for the clients that never ask for one.
 */
static int
serve_clients_max(void)
{
	int from_queues;

	if (!serve.nsq_total) {
		return SERVE_CLIENTS_MAX; ///< The controller did not say
	}

	from_queues = (int)(serve.nsq_total / SERVE_IOQPAIRS_PER_CLIENT);
	from_queues += SERVE_CLIENTS_QUEUELESS;

	return (from_queues < SERVE_CLIENTS_MAX) ? from_queues : SERVE_CLIENTS_MAX;
}

/**
 * Guards what every connection shares: the heap and the table of queues
 *
 * One heap serves the whole runtime and its free list has no lock of its own,
 * and the table of queues handed out is one table. Both are short to touch.
 */
static pthread_mutex_t serve_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Guards one controller's admin queue
 *
 * Completions are reaped from the head with no record of who submitted, and
 * nvme_qpair_submit_sync() does not check that the one it takes carries the
 * identifier it sent, so two commands in flight on one queue can take each
 * other's answers. One at a time per controller, then. This is the lock a
 * Format holds for as long as it runs, which is why it is separate: everything
 * that does not need the admin queue carries on without it.
 *
 * Ordered after serve_lock wherever both are held, which is queue creation and
 * deletion, since those allocate from the heap and submit on the admin queue.
 */
static pthread_mutex_t serve_admin_lock[SERVE_DEVS_MAX];

/**
 * Release everything a client held, queues before the memory behind them
 */
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
 * the client's own record, since that is who has to give it back.
 *
 * @param dev A device this process opened
 * @param depth Entries the client asked for
 * @param held Pre-allocated slot in the client's record
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
 * @param held The client's record of the queue
 *
 * @return 0 on success, negative errno on error
 */
static int
serve_ioqpair_free(struct xnvme_dev *dev, struct serve_ioqpair *held)
{
	struct xnvme_be_upcie_state *state;
	struct nvme_controller *ctrl;

	if (!dev || !held) {
		return -EINVAL;
	}

	state = (void *)dev->be.state;
	ctrl = state->ctrlr->ctrl;

	/* The queue goes before its memory does: the controller has to stop
	 * being able to reach an address before it stops resolving, which this
	 * does in one call. */
	nvme_controller_delete_io_qpair_dmamem(ctrl, &held->qpair, &g_upcie_rte.mem.heap,
					       held->sq_offset, held->cq_offset, held->prp_offset);
	memset(held, 0, sizeof(*held));

	return 0;
}

static void
serve_client_release(struct xnvme_dev *dev, struct serve_client *client)
{
	/* Queues first: the controller has to stop being able to reach an
	 * address before that address stops meaning anything. */
	for (int i = 0; i < client->nqpairs; ++i) {
		int err;

		pthread_mutex_lock(&serve_admin_lock[client->dev]);
		err = serve_ioqpair_free(dev, &client->qpairs[i]);
		pthread_mutex_unlock(&serve_admin_lock[client->dev]);

		if (err) {
			XNVME_DEBUG("FAILED: free(slot(%d)); err(%d)", i, err);
		}
	}

	for (int i = 0; i < client->nallocs; ++i) {
		int err = xnvme_be_upcie_cplane_free_buf(client->allocs[i]);

		if (err) {
			XNVME_DEBUG("FAILED: reclaim(0x%" PRIx64 "); err(%d)", client->allocs[i],
				    err);
		}
	}

	if (client->sock >= 0) {
		close(client->sock);
	}

	memset(client, 0, sizeof(*client));
	client->sock = -1;
}

/**
 * Connections in hand, and queues held on one controller
 *
 * Walked rather than counted up as things happen: a tally maintained by hand
 * has to be right in every path that adds or drops a connection, including the
 * ones that fail halfway, and it was wrong that way once already. Callers hold
 * serve_lock.
 */
static int
serve_nclients(void)
{
	int total = 0;

	for (int d = 0; d < serve.ndevs; ++d) {
		for (int i = 0; i < SERVE_CLIENTS_MAX; ++i) {
			total += serve.workers[d].conns[i].live ? 1 : 0;
		}
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

	for (int i = 0; i < SERVE_CLIENTS_MAX; ++i) {
		const struct serve_conn *conn = &serve.workers[devidx].conns[i];

		total += conn->live ? conn->client.nqpairs : 0;
	}

	return total;
}

/**
 * Answer one message, and record what the answer handed out
 */
static int
serve_one(struct xnvme_dev *dev, struct serve_client *client,
	  const struct xnvme_be_upcie_cplane_export *exported)
{
	struct nvme_cplane_msg reply = {0};
	struct nvme_cplane_msg msg = {0};
	int fds[NVME_CPLANE_FDS_MAX];
	uint32_t nfds = 0;
	int err;

	/* Blocking, and that is the point: this thread serves one connection,
	 * so a client that writes half a message and stalls holds nothing but
	 * its own thread. */
	err = nvme_cplane_msg_recv(client->sock, &msg, NULL, NULL);
	if (err) {
		return err; ///< -ENOTCONN when the client is gone
	}

	reply.op = msg.op;
	reply.version = NVME_CPLANE_VERSION;

	if (msg.version != NVME_CPLANE_VERSION) {
		XNVME_DEBUG("FAILED: client speaks version(%u)", msg.version);
		reply.status = -EPROTO;
		return nvme_cplane_msg_send(client->sock, &reply, NULL, 0);
	}

	/* Taken outside serve_lock, and the only request that is. It can run as
	 * long as a Format does, and it touches nothing but the admin queue, so
	 * every other request on this controller carries on meanwhile. */
	if (msg.op == NVME_CPLANE_OP_ADMIN_CMD) {
		if (nvme_cplane_admin_permitted(&msg.u.admin.cmd)) {
			pthread_mutex_lock(&serve_admin_lock[client->dev]);
			reply.status = xnvme_be_upcie_cplane_admin(dev, &msg.u.admin.cmd,
								   &reply.u.admin.cpl);
			pthread_mutex_unlock(&serve_admin_lock[client->dev]);
		} else {
			reply.status = -EPERM;
		}

		return nvme_cplane_msg_send(client->sock, &reply, NULL, 0);
	}

	pthread_mutex_lock(&serve_lock);

	switch (msg.op) {
	case NVME_CPLANE_OP_INIT_CONNECTION:
		reply.u.init.record_offset = exported->record_offset;
		reply.u.init.heap_nbytes = exported->heap_nbytes;
		reply.u.init.bar0_nbytes = exported->bar0_nbytes;
		fds[nfds++] = exported->heap_fd;
		fds[nfds++] = exported->bar0_fd;
		client->initialized = 1;
		break;

	case NVME_CPLANE_OP_ALLOC_IOQPAIR: {
		struct serve_qalloc allocation = {0};

		/* An offset is only meaningful to a client that mapped the
		 * heap, and mapping it is what initialising does. Without the
		 * gate an uninitialised connection could take a real queue off
		 * the controller and never be able to use it. */
		if (!client->initialized) {
			reply.status = -ENOTCONN;
			break;
		}

		if (client->nqpairs == SERVE_IOQPAIRS_PER_CLIENT) {
			reply.status = -ENOSPC;
			break;
		}

		/* Allocates from the heap and submits Create SQ and Create CQ,
		 * so it needs the admin queue as well as what serve_lock
		 * covers. serve_lock is already held; admin comes after it. */
		pthread_mutex_lock(&serve_admin_lock[client->dev]);
		reply.status = serve_ioqpair_alloc(dev, msg.u.queue.depth,
						   &client->qpairs[client->nqpairs], &allocation);
		pthread_mutex_unlock(&serve_admin_lock[client->dev]);
		if (reply.status) {
			break;
		}

		client->nqpairs++;

		reply.u.queue.allocation.sq_offset = allocation.sq_offset;
		reply.u.queue.allocation.cq_offset = allocation.cq_offset;
		reply.u.queue.allocation.prp_offset = allocation.prp_offset;
		reply.u.queue.allocation.qid = allocation.qid;
		reply.u.queue.allocation.depth = allocation.depth;
	} break;

	case NVME_CPLANE_OP_FREE_IOQPAIR:
		reply.status = -ENOENT;
		for (int i = 0; i < client->nqpairs; ++i) {
			if (client->qpairs[i].qpair.qid != msg.u.release.qid) {
				continue;
			}

			/* Submits Delete SQ and Delete CQ, so the admin queue
			 * again, taken after serve_lock as everywhere. */
			pthread_mutex_lock(&serve_admin_lock[client->dev]);
			reply.status = serve_ioqpair_free(dev, &client->qpairs[i]);
			pthread_mutex_unlock(&serve_admin_lock[client->dev]);
			client->qpairs[i] = client->qpairs[--client->nqpairs];
			break;
		}
		break;

	case NVME_CPLANE_OP_ALLOC_BUF: {
		uint64_t offset = 0;

		if (!client->initialized) {
			reply.status = -ENOTCONN;
			break;
		}

		if (client->nallocs == SERVE_ALLOCS_PER_CLIENT) {
			reply.status = -ENOSPC;
			break;
		}

		reply.status = xnvme_be_upcie_cplane_alloc_buf(msg.u.mem.nbytes, &offset);
		if (reply.status) {
			break;
		}

		client->allocs[client->nallocs++] = offset;
		reply.u.mem.offset = offset;
	} break;

	case NVME_CPLANE_OP_FREE_BUF:
		reply.status = -ENOENT;
		for (int i = 0; i < client->nallocs; ++i) {
			if (client->allocs[i] != msg.u.mem.offset) {
				continue;
			}

			reply.status = xnvme_be_upcie_cplane_free_buf(msg.u.mem.offset);
			client->allocs[i] = client->allocs[--client->nallocs];
			break;
		}
		break;

	case NVME_CPLANE_OP_STATUS: {
		/* Any controller this server holds, not just the one this
		 * connection reached, so a client learns the set by walking
		 * the range instead of reading it out of socket names. */
		int idx = (int)msg.u.status.index;

		if ((idx < 0) || (idx >= serve.ndevs)) {
			reply.status = -ERANGE;
			break;
		}

		reply.u.status.ndevs = (uint32_t)serve.ndevs;

		/* Asking is not connecting for I/O, so whoever asks does not count
		 * itself among the clients. */
		reply.u.status.nclients = (uint32_t)(serve_nclients() - 1);
		reply.u.status.nqueues = (uint32_t)serve_nqueues(idx);
		reply.u.status.nsq_total = serve.nsq_total;
		reply.u.status.ncq_total = serve.ncq_total;
		snprintf(reply.u.status.bdf, sizeof(reply.u.status.bdf), "%s",
			 serve.exports[idx].uri);
	} break;

	default:
		reply.status = -ENOSYS;
		break;
	}

	pthread_mutex_unlock(&serve_lock);

	reply.nfds = nfds;

	return nvme_cplane_msg_send(client->sock, &reply, nfds ? fds : NULL, nfds);
}

static void *
serve_conn_main(void *arg)
{
	struct serve_conn *conn = arg;

	while (!serve_one(conn->dev, &conn->client, conn->exported)) {
		;
	}

	/* The count comes down in serve_client_release(), along with the socket
	 * it belongs to. */
	pthread_mutex_lock(&serve_lock);
	serve_client_release(conn->dev, &conn->client);
	pthread_mutex_unlock(&serve_lock);

	return NULL;
}

static void *
serve_worker_main(void *arg)
{
	struct serve_worker *worker = arg;
	struct pollfd pfds[2];

	while (!*worker->stop) {
		int nfds = 0;

		for (int i = 0; i < worker->nlisteners; ++i) {
			pfds[nfds].fd = worker->listeners[i];
			pfds[nfds].events = POLLIN;
			nfds++;
		}

		/* A timeout rather than a signal mask: the caller's flag is
		 * what ends this, and a second of latency on the way out costs
		 * nobody anything. */
		if (poll(pfds, nfds, 1000) < 0) {
			if (errno == EINTR) {
				continue;
			}
			break;
		}

		for (int l = 0; l < worker->nlisteners; ++l) {
			while (pfds[l].revents & POLLIN) {
				struct serve_conn *conn = NULL;
				int sock = accept(worker->listeners[l], NULL, NULL);

				if (sock < 0) {
					break; ///< Drained, or nothing was waiting
				}

				/* Reaped here rather than by joining on exit,
				 * so a client that comes and goes does not
				 * accumulate a slot for the life of the
				 * server. */
				pthread_mutex_lock(&serve_lock);
				for (int i = 0; i < serve_clients_max(); ++i) {
					if (worker->conns[i].live &&
					    (worker->conns[i].client.sock < 0)) {
						pthread_join(worker->conns[i].tid, NULL);
						worker->conns[i].live = 0;
					}
					if (!worker->conns[i].live && !conn) {
						conn = &worker->conns[i];
					}
				}
				pthread_mutex_unlock(&serve_lock);
				if (!conn) {
					close(sock);
					continue;
				}

				memset(conn, 0, sizeof(*conn));
				conn->dev = worker->dev;
				conn->exported = worker->exported;
				conn->client.sock = sock;
				conn->client.dev = worker->devidx;

				/* Marked live before the thread starts, since
				 * that is what the counts walk; cleared again
				 * if the thread never runs. */
				pthread_mutex_lock(&serve_lock);
				conn->live = 1;
				pthread_mutex_unlock(&serve_lock);

				if (pthread_create(&conn->tid, NULL, serve_conn_main, conn)) {
					XNVME_DEBUG("FAILED: pthread_create(conn)");
					pthread_mutex_lock(&serve_lock);
					conn->live = 0;
					pthread_mutex_unlock(&serve_lock);
					close(sock);
					continue;
				}
			}
		}
	}

	/* Every connection thread is blocked reading its socket, so shutting
	 * the socket down is what returns it. */
	for (int i = 0; i < SERVE_CLIENTS_MAX; ++i) {
		if (!worker->conns[i].live) {
			continue;
		}
		shutdown(worker->conns[i].client.sock, SHUT_RDWR);
	}
	for (int i = 0; i < SERVE_CLIENTS_MAX; ++i) {
		if (!worker->conns[i].live) {
			continue;
		}
		pthread_join(worker->conns[i].tid, NULL);
		worker->conns[i].live = 0;
	}

	return NULL;
}

int
xnvme_cplane_serve(struct xnvme_dev **devs, int ndevs, uint32_t cplane_id,
		   volatile sig_atomic_t *stop)
{
	struct xnvme_be_upcie_cplane_export exported[SERVE_DEVS_MAX] = {0};
	struct sockaddr_un addr = {.sun_family = AF_UNIX};
	char paths[SERVE_DEVS_MAX + 1][256] = {{0}};
	int listeners[SERVE_DEVS_MAX + 1];
	struct serve_worker *workers;
	int nlisteners = 0;
	int nexported = 0;
	int err;

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

	/* On the heap rather than the stack: a worker carries a connection
	 * apiece and a connection carries the queues it was given, so the
	 * array is half a megabyte at the caps above. */
	workers = calloc(SERVE_DEVS_MAX, sizeof(*workers));
	if (!workers) {
		return -ENOMEM;
	}

	serve.running = 1;
	serve.devs = devs;
	serve.exports = exported;
	serve.workers = workers;
	serve.ndevs = ndevs;

	for (int i = 0; i < ndevs; ++i) {
		pthread_mutex_init(&serve_admin_lock[i], NULL);
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

	xnvme_be_upcie_cplane_socket_path(cplane_id, NULL, paths[0], sizeof(paths[0]));
	for (int i = 0; i < ndevs; ++i) {
		xnvme_be_upcie_cplane_socket_path(cplane_id, devs[i]->ident.uri, paths[i + 1],
						  sizeof(paths[i + 1]));
	}

	for (nlisteners = 0; nlisteners < (ndevs + 1); ++nlisteners) {
		unlink(paths[nlisteners]);

		/* Non-blocking, because accept() is drained in a loop: on a
		 * blocking listener the call after the last waiting connection
		 * waits for one that is not coming, and the loop never gets
		 * back to poll(). */
		listeners[nlisteners] = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
		if (listeners[nlisteners] < 0) {
			err = -errno;
			goto exit;
		}

		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", paths[nlisteners]);

		/* Deep enough to absorb a burst of probes. Something asking
		 * whether a runtime is alive gets its answer from connecting,
		 * so a refused connection reads as a runtime that is not
		 * there. */
		if (bind(listeners[nlisteners], (struct sockaddr *)&addr, sizeof(addr)) ||
		    listen(listeners[nlisteners], SERVE_CLIENTS_MAX * 8)) {
			err = -errno;
			close(listeners[nlisteners]);
			goto exit;
		}
	}

	/* One worker per controller. The runtime-wide listener goes to the
	 * first, since it answers for the runtime rather than for a controller
	 * and its clients are treated as that controller's. */
	for (int i = 0; i < ndevs; ++i) {
		workers[i].dev = devs[i];
		workers[i].devidx = i;
		workers[i].exported = &exported[i];
		workers[i].stop = stop;
		workers[i].nlisteners = 0;
		if (!i) {
			workers[i].listeners[workers[i].nlisteners++] = listeners[0];
		}
		workers[i].listeners[workers[i].nlisteners++] = listeners[i + 1];
	}

	for (int i = 0; i < ndevs; ++i) {
		if (pthread_create(&workers[i].tid, NULL, serve_worker_main, &workers[i])) {
			err = -errno;
			XNVME_DEBUG("FAILED: pthread_create(worker %d); err(%d)", i, err);
			*stop = 1;
			break;
		}
		workers[i].started = 1;
	}

	for (int i = 0; i < ndevs; ++i) {
		if (workers[i].started) {
			pthread_join(workers[i].tid, NULL);
		}
	}

exit:
	for (int i = 0; i < ndevs; ++i) {
		pthread_mutex_destroy(&serve_admin_lock[i]);
	}

	serve.running = 0;
	for (int i = 0; i < nlisteners; ++i) {
		close(listeners[i]);
		unlink(paths[i]);
	}
	for (int i = 0; i < nexported; ++i) {
		xnvme_be_upcie_cplane_unexport(&exported[i]);
	}

	free(workers);

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
