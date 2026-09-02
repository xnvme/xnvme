// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Serving clients of a runtime this process owns
 *
 * There is little here because the pieces are elsewhere: uPCIe defines what
 * passes between the two, the backend knows how to describe a runtime, and
 * what a client holds and how a queue is taken off a controller are their own
 * files beside this one. What is left is the sockets, the threads and what
 * happens to one message.
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
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_cplane_serve.h>

struct serve_state serve;
pthread_mutex_t serve_lock = PTHREAD_MUTEX_INITIALIZER;

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

		case NVME_CPLANE_OP_ALLOC_IOQPAIR_AT: {
			uint32_t qid = 0;

			pthread_mutex_lock(&serve_lock);
			reply.status = serve_ioqpair_alloc_at(dev, conn, admin->devidx, &conn->msg,
							      &conn->qpairs[conn->nqpairs], &qid);
			if (!reply.status) {
				conn->qpairs[conn->nqpairs++].dev = admin->devidx;
			}
			pthread_mutex_unlock(&serve_lock);
			if (reply.status) {
				break;
			}

			reply.u.queue_at.qid = qid;
			reply.u.queue_at.depth = conn->msg.u.queue_at.depth;
		} break;

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

		case NVME_CPLANE_OP_REGISTER_MEM: {
			struct serve_registration *held;

			if (conn->in_fd < 0) {
				/* The request names a region by descriptor;
				 * without one there is nothing to describe. */
				reply.status = -EINVAL;
				break;
			}

			pthread_mutex_lock(&serve_lock);
			reply.status = serve_conn_regs_grow(conn);
			pthread_mutex_unlock(&serve_lock);
			if (reply.status) {
				break;
			}

			held = &conn->regs[conn->nregs];

			pthread_mutex_lock(&serve_lock);
			reply.status = xnvme_be_upcie_cplane_register_mem(
				conn->in_fd, conn->msg.u.reg.nbytes, conn->msg.u.reg.page_size,
				serve.exports[admin->devidx].uri, &held->reg);
			if (!reply.status) {
				held->dev = admin->devidx;
				conn->nregs++;
			}
			pthread_mutex_unlock(&serve_lock);
			if (reply.status) {
				break;
			}

			reply.u.reg.desc_offset = held->reg.desc_offset;
		} break;

		case NVME_CPLANE_OP_UNREGISTER_MEM:
			reply.status = -ENOENT;
			pthread_mutex_lock(&serve_lock);
			for (int i = 0; i < conn->nregs; ++i) {
				/* Both, since a region registered for several
				 * controllers is described once and every
				 * registration on it answers to that offset. */
				if ((conn->regs[i].reg.desc_offset !=
				     conn->msg.u.reg.desc_offset) ||
				    (conn->regs[i].dev != admin->devidx)) {
					continue;
				}

				xnvme_be_upcie_cplane_unregister_mem(&conn->regs[i].reg);
				conn->regs[i] = conn->regs[--conn->nregs];
				reply.status = 0;
				break;
			}
			pthread_mutex_unlock(&serve_lock);
			break;

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

		/* The descriptor was the request's, whether or not it was used. */
		if (conn->in_fd >= 0) {
			close(conn->in_fd);
			conn->in_fd = -1;
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
	case NVME_CPLANE_OP_ALLOC_IOQPAIR_AT:
	case NVME_CPLANE_OP_FREE_IOQPAIR:
	case NVME_CPLANE_OP_REGISTER_MEM:
	case NVME_CPLANE_OP_UNREGISTER_MEM:
		/* An offset is only meaningful to a client that mapped the
		 * heap, and mapping it is what initialising does. Without the
		 * gate an uninitialised connection could take a real queue off
		 * the controller and never be able to use it. */
		if (((conn->msg.op == NVME_CPLANE_OP_ALLOC_IOQPAIR) ||
		     (conn->msg.op == NVME_CPLANE_OP_ALLOC_IOQPAIR_AT)) &&
		    !(conn->inited & (1U << (unsigned)idx))) {
			reply.status = -ENOTCONN;
			break;
		}
		if (((conn->msg.op == NVME_CPLANE_OP_ALLOC_IOQPAIR) ||
		     (conn->msg.op == NVME_CPLANE_OP_ALLOC_IOQPAIR_AT)) &&
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
	for (int i = 0; i < conn->nregs; ++i) {
		devs |= 1U << (unsigned)conn->regs[i].dev;
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
		   volatile sig_atomic_t *stop, void (*ready)(void *), void *ready_arg)
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

	/* The whole uPCIe family, not just the host one: 'upcie-cuda' and
	 * 'upcie-hip' share a controller the same way, over this socket, and
	 * differ only in where a client's buffers live. Matching the name
	 * exactly sent them down the -ENOSYS path below, which says the backend
	 * shares by its own means and leaves the caller holding controllers
	 * nobody can reach. */
	if (strncmp(devs[0]->be.attr.name, "upcie", 5)) {
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
		serve.conns[i].in_fd = -1;
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

	/* Everything a client depends on is up: the controllers are exported,
	 * the socket is accepting, and the threads that answer are running.
	 * Announcing before this point would name a server that is listening
	 * but cannot yet say what it holds. */
	if (ready) {
		ready(ready_arg);
	}

	/* One reader for every connection: requests are memory, and what is not
	 * is owed to a controller's thread. */
	while (!*stop) {
		struct pollfd pfds[SERVE_CONNS_MAX + 2];
		struct serve_conn *polled[SERVE_CONNS_MAX];
		int npolled = 0;
		int npfds = 0;

		pfds[npfds].fd = listener;
		pfds[npfds].events = POLLIN;
		npfds++;

		pfds[npfds].fd = serve.wake[0];
		pfds[npfds].events = POLLIN;
		npfds++;

		pthread_mutex_lock(&serve_lock);
		for (int i = 0; i < SERVE_CONNS_MAX; ++i) {
			if ((serve.conns[i].sock < 0) || serve.conns[i].owed) {
				continue;
			}
			pfds[npfds].fd = serve.conns[i].sock;
			pfds[npfds].events = POLLIN;
			npfds++;
			polled[npolled++] = &serve.conns[i];
		}
		pthread_mutex_unlock(&serve_lock);

		/* A timeout rather than a signal mask: the caller's flag is
		 * what ends this, and a second of latency on the way out costs
		 * nobody anything. */
		if (poll(pfds, (nfds_t)npfds, 1000) < 0) {
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

			{
				int in[NVME_CPLANE_FDS_MAX];
				uint32_t nin = 0;

				rc = nvme_cplane_msg_recv_some(conn->sock, &conn->msg,
							       &conn->nread, in, &nin);

				/* Whatever arrived is installed in this process
				 * already, so it is this process's to close. One
				 * is kept for the request that named it; a peer
				 * sending more than it needs gets them closed
				 * rather than leaked. */
				for (uint32_t f = 0; f < nin; ++f) {
					if ((f == 0) && (conn->in_fd < 0)) {
						conn->in_fd = in[f];
						continue;
					}
					close(in[f]);
				}
			}
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
		for (int r = 0; r < conn->nregs; ++r) {
			conn->closing_devs |= 1U << (unsigned)conn->regs[r].dev;
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
		   uint32_t XNVME_UNUSED(cplane_id), volatile sig_atomic_t *XNVME_UNUSED(stop),
		   void (*ready)(void *), void *XNVME_UNUSED(ready_arg))
{
	/* XNVME_UNUSED() names a parameter, which a function-pointer
	 * declarator has no room for. */
	(void)ready;

	return -ENOSYS;
}
#endif
