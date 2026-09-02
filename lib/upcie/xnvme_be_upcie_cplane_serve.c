// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/*
 * The server: one thread reads every connection and answers what is memory,
 * and one thread per controller runs what reaches the controller.
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
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_cplane_serve.h>

struct xnvme_be_upcie_cplane_serve_state g_upcie_cplane_serve;
pthread_mutex_t g_upcie_cplane_serve_lock = PTHREAD_MUTEX_INITIALIZER;

void
xnvme_be_upcie_cplane_serve_wake(void)
{
	const char one = 'x';
	ssize_t nbytes;

	do {
		nbytes = write(g_upcie_cplane_serve.wake[1], &one, 1);
	} while ((nbytes < 0) && (errno == EINTR));

	/* A full pipe is a wake-up already pending */
	(void)nbytes;
}

void
xnvme_be_upcie_cplane_serve_owe(struct xnvme_be_upcie_cplane_serve_conn *conn, int devidx)
{
	struct xnvme_be_upcie_cplane_serve_admin *admin = &g_upcie_cplane_serve.admin[devidx];

	conn->owed = 1;
	conn->owed_dev = devidx;

	pthread_mutex_lock(&admin->lock);
	admin->queue[admin->nqueued++] = conn;
	pthread_cond_signal(&admin->cond);
	pthread_mutex_unlock(&admin->lock);
}

/* The thread of one controller: runs the requests owed to it, in order */
static void *
xnvme_be_upcie_cplane_serve_admin_main(void *arg)
{
	struct xnvme_be_upcie_cplane_serve_admin *admin = arg;
	struct xnvme_dev *dev = g_upcie_cplane_serve.devs[admin->devidx];

	for (;;) {
		struct nvme_cplane_msg reply = {0};
		struct xnvme_be_upcie_cplane_serve_conn *conn;

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
			xnvme_be_upcie_cplane_serve_conn_release(conn, admin->devidx);
			xnvme_be_upcie_cplane_serve_wake();
			continue;
		}

		reply.op = conn->msg.op;
		reply.version = NVME_CPLANE_VERSION;

		switch (conn->msg.op) {
		case NVME_CPLANE_OP_ADMIN_CMD:
			reply.result = xnvme_be_upcie_cplane_serve_admin_cmd(
				dev, &conn->msg.u.admin_cmd.cmd, &reply.u.admin_cmd.cpl);
			break;

		case NVME_CPLANE_OP_ALLOC_IOQPAIR_AT: {
			uint32_t qid = 0;

			pthread_mutex_lock(&g_upcie_cplane_serve_lock);
			reply.result = xnvme_be_upcie_cplane_serve_ioqpair_alloc_at(
				dev, conn, admin->devidx, &conn->msg, &conn->qpairs[conn->nqpairs],
				&qid);
			if (!reply.result) {
				conn->qpairs[conn->nqpairs++].dev = admin->devidx;
			}
			pthread_mutex_unlock(&g_upcie_cplane_serve_lock);
			if (reply.result) {
				break;
			}

			reply.u.alloc_ioqpair_at.allocation.qid = qid;
			reply.u.alloc_ioqpair_at.allocation.depth =
				conn->msg.u.alloc_ioqpair_at.depth;
		} break;

		case NVME_CPLANE_OP_ALLOC_IOQPAIR:
			pthread_mutex_lock(&g_upcie_cplane_serve_lock);
			reply.result = xnvme_be_upcie_cplane_serve_ioqpair_alloc(
				dev, conn->msg.u.alloc_ioqpair.depth, &conn->qpairs[conn->nqpairs],
				&reply.u.alloc_ioqpair.allocation);
			if (!reply.result) {
				conn->qpairs[conn->nqpairs++].dev = admin->devidx;
			}
			pthread_mutex_unlock(&g_upcie_cplane_serve_lock);
			break;

		case NVME_CPLANE_OP_REGISTER_MEM: {
			struct xnvme_be_upcie_cplane_serve_registration *held;

			if (conn->in_fd < 0) {
				reply.result = -EINVAL;
				break;
			}

			pthread_mutex_lock(&g_upcie_cplane_serve_lock);
			reply.result = xnvme_be_upcie_cplane_serve_conn_regs_grow(conn);
			pthread_mutex_unlock(&g_upcie_cplane_serve_lock);
			if (reply.result) {
				break;
			}

			held = &conn->regs[conn->nregs];

			pthread_mutex_lock(&g_upcie_cplane_serve_lock);
			reply.result = xnvme_be_upcie_cplane_serve_register_mem(
				conn->in_fd, conn->msg.u.mem.nbytes, conn->msg.u.mem.page_size,
				g_upcie_cplane_serve.shares[admin->devidx].uri, &held->reg);
			if (!reply.result) {
				held->dev = admin->devidx;
				conn->nregs++;
			}
			pthread_mutex_unlock(&g_upcie_cplane_serve_lock);
			if (reply.result) {
				break;
			}

			reply.u.mem.desc_offset = held->reg.desc_offset;
		} break;

		case NVME_CPLANE_OP_UNREGISTER_MEM:
			reply.result = -ENOENT;
			pthread_mutex_lock(&g_upcie_cplane_serve_lock);
			for (int i = 0; i < conn->nregs; ++i) {
				/* One description serves every controller a region is registered
				 * for */
				if ((conn->regs[i].reg.desc_offset !=
				     conn->msg.u.mem.desc_offset) ||
				    (conn->regs[i].dev != admin->devidx)) {
					continue;
				}

				xnvme_be_upcie_cplane_serve_unregister_mem(&conn->regs[i].reg);
				conn->regs[i] = conn->regs[--conn->nregs];
				reply.result = 0;
				break;
			}
			pthread_mutex_unlock(&g_upcie_cplane_serve_lock);
			break;

		case NVME_CPLANE_OP_FREE_IOQPAIR:
			reply.result = -ENOENT;
			pthread_mutex_lock(&g_upcie_cplane_serve_lock);
			for (int i = 0; i < conn->nqpairs; ++i) {
				/* Two controllers hand out the same identifiers */
				if ((conn->qpairs[i].dev != admin->devidx) ||
				    (conn->qpairs[i].qpair.qid != conn->msg.u.free_ioqpair.qid)) {
					continue;
				}

				reply.result = xnvme_be_upcie_cplane_serve_ioqpair_free(
					dev, &conn->qpairs[i]);
				conn->qpairs[i] = conn->qpairs[--conn->nqpairs];
				break;
			}
			pthread_mutex_unlock(&g_upcie_cplane_serve_lock);
			break;

		default:
			reply.result = -ENOSYS;
			break;
		}

		/* The descriptor was the request's, whether or not it was used */
		if (conn->in_fd >= 0) {
			close(conn->in_fd);
			conn->in_fd = -1;
		}

		/* A peer that stopped reading is gone; the reader drops it on the shutdown */
		if (nvme_cplane_msg_trysend(conn->sock, &reply, NULL, 0)) {
			XNVME_DEBUG("INFO: reply went nowhere");
			shutdown(conn->sock, SHUT_RDWR);
		}

		pthread_mutex_lock(&g_upcie_cplane_serve_lock);
		conn->owed = 0;
		pthread_mutex_unlock(&g_upcie_cplane_serve_lock);

		xnvme_be_upcie_cplane_serve_wake();
	}

	return NULL;
}

/* What xnvme_be_upcie_cplane_serve_dispatch() leaves its caller to do */
enum xnvme_be_upcie_cplane_serve_reply {
	XNVME_BE_UPCIE_CPLANE_SERVE_REPLY_NONE = 0, /* A controller's thread answers it */
	XNVME_BE_UPCIE_CPLANE_SERVE_REPLY_SEND,     /* Send the reply */
	XNVME_BE_UPCIE_CPLANE_SERVE_REPLY_LAST,     /* Send it, then drop the connection */
};

/*
 * Answer a request that is memory into `out`, or owe it to a controller's thread.
 * Callers hold g_upcie_cplane_serve_lock, so the reply is sent by the caller after
 * letting go of it.
 */
static enum xnvme_be_upcie_cplane_serve_reply
xnvme_be_upcie_cplane_serve_dispatch(struct xnvme_be_upcie_cplane_serve_conn *conn,
				     struct nvme_cplane_msg *out, int *fds, uint32_t *out_nfds)
{
	const struct xnvme_be_upcie_cplane_share *share;
	struct nvme_cplane_msg reply = {0};
	const int idx = (int)conn->msg.index;
	uint32_t nfds = 0;

	reply.op = conn->msg.op;
	reply.version = NVME_CPLANE_VERSION;
	reply.index = conn->msg.index;

	/* Refused and dropped, since its messages may not be the size of these */
	if (conn->msg.version != NVME_CPLANE_VERSION) {
		XNVME_DEBUG("FAILED: client speaks version(%u)", conn->msg.version);
		reply.result = -EPROTO;
		*out = reply;
		*out_nfds = 0;

		return XNVME_BE_UPCIE_CPLANE_SERVE_REPLY_LAST;
	}

	/* Everything but the heap operations names a controller; checked once here */
	switch (conn->msg.op) {
	case NVME_CPLANE_OP_ALLOC_BUF:
	case NVME_CPLANE_OP_FREE_BUF:
		share = NULL;
		break;

	default:
		if ((idx < 0) || (idx >= g_upcie_cplane_serve.ndevs)) {
			reply.result = -ERANGE;
			*out = reply;
			*out_nfds = 0;

			return XNVME_BE_UPCIE_CPLANE_SERVE_REPLY_SEND;
		}
		share = &g_upcie_cplane_serve.shares[idx];
		break;
	}

	switch (conn->msg.op) {
	case NVME_CPLANE_OP_ADMIN_CMD:
	case NVME_CPLANE_OP_ALLOC_IOQPAIR:
	case NVME_CPLANE_OP_ALLOC_IOQPAIR_AT:
	case NVME_CPLANE_OP_FREE_IOQPAIR:
	case NVME_CPLANE_OP_REGISTER_MEM:
	case NVME_CPLANE_OP_UNREGISTER_MEM:
		/* Offsets mean nothing to a client that has not mapped the heap */
		if (((conn->msg.op == NVME_CPLANE_OP_ALLOC_IOQPAIR) ||
		     (conn->msg.op == NVME_CPLANE_OP_ALLOC_IOQPAIR_AT) ||
		     (conn->msg.op == NVME_CPLANE_OP_REGISTER_MEM)) &&
		    !(conn->inited & (UINT64_C(1) << (unsigned)idx))) {
			reply.result = -ENOTCONN;
			break;
		}
		if (((conn->msg.op == NVME_CPLANE_OP_ALLOC_IOQPAIR) ||
		     (conn->msg.op == NVME_CPLANE_OP_ALLOC_IOQPAIR_AT)) &&
		    (conn->nqpairs == XNVME_BE_UPCIE_CPLANE_SERVE_IOQPAIRS_PER_CLIENT)) {
			reply.result = -ENOSPC;
			break;
		}

		xnvme_be_upcie_cplane_serve_owe(conn, idx);

		return XNVME_BE_UPCIE_CPLANE_SERVE_REPLY_NONE;

	case NVME_CPLANE_OP_INIT_CONNECTION:
		reply.u.init_connection.ctrlr = share->base;
		/* The identifier clients name it by; the controller's own bdf can be empty */
		snprintf(reply.u.init_connection.bdf, sizeof(reply.u.init_connection.bdf), "%s",
			 share->uri);
		reply.u.init_connection.desc_offset = share->desc_offset;
		reply.u.init_connection.heap_nbytes = share->heap_nbytes;
		reply.u.init_connection.bar0_nbytes = share->bar0_nbytes;
		fds[nfds++] = share->heap_fd;
		fds[nfds++] = share->bar0_fd;
		conn->inited |= UINT64_C(1) << (unsigned)idx;
		break;

	case NVME_CPLANE_OP_ALLOC_BUF: {
		uint64_t offset = 0;

		/* Any controller initialised means the heap is mapped */
		if (!conn->inited) {
			reply.result = -ENOTCONN;
			break;
		}

		reply.result = xnvme_be_upcie_cplane_serve_conn_allocs_grow(conn);
		if (reply.result) {
			break;
		}

		reply.result =
			xnvme_be_upcie_cplane_serve_buf_alloc(conn->msg.u.buf.nbytes, &offset);
		if (reply.result) {
			break;
		}

		conn->allocs[conn->nallocs++] = offset;
		reply.u.buf.offset = offset;
	} break;

	case NVME_CPLANE_OP_FREE_BUF:
		reply.result = -ENOENT;
		for (int i = 0; i < conn->nallocs; ++i) {
			if (conn->allocs[i] != conn->msg.u.buf.offset) {
				continue;
			}

			reply.result =
				xnvme_be_upcie_cplane_serve_buf_free(conn->msg.u.buf.offset);
			conn->allocs[i] = conn->allocs[--conn->nallocs];
			break;
		}
		break;

	case NVME_CPLANE_OP_STATUS:
		reply.u.status.ndevs = (uint32_t)g_upcie_cplane_serve.ndevs;

		/* The connection asking is not counted */
		reply.u.status.nclients = (uint32_t)(xnvme_be_upcie_cplane_serve_conn_count() - 1);
		reply.u.status.nclients_ctrlr =
			(uint32_t)xnvme_be_upcie_cplane_serve_conn_count_of(idx);
		reply.u.status.nqueues = (uint32_t)xnvme_be_upcie_cplane_serve_conn_nqueues(idx);
		reply.u.status.nsq_total = g_upcie_cplane_serve.nsq_total;
		reply.u.status.ncq_total = g_upcie_cplane_serve.ncq_total;
		snprintf(reply.u.status.bdf, sizeof(reply.u.status.bdf), "%s", share->uri);
		break;

	default:
		reply.result = -ENOSYS;
		break;
	}

	reply.nfds = nfds;
	*out = reply;
	*out_nfds = nfds;

	return XNVME_BE_UPCIE_CPLANE_SERVE_REPLY_SEND;
}

/*
 * Take a connection down. Queues go back on the threads of the controllers that
 * lent them, see xnvme_be_upcie_cplane_serve_conn_release(); without queues the
 * buffers go back here. Callers hold g_upcie_cplane_serve_lock.
 */
static void
xnvme_be_upcie_cplane_serve_drop(struct xnvme_be_upcie_cplane_serve_conn *conn)
{
	uint64_t devs = 0;

	for (int i = 0; i < conn->nqpairs; ++i) {
		devs |= UINT64_C(1) << (unsigned)conn->qpairs[i].dev;
	}
	for (int i = 0; i < conn->nregs; ++i) {
		devs |= UINT64_C(1) << (unsigned)conn->regs[i].dev;
	}

	if (devs) {
		conn->closing = 1;
		conn->closing_devs = devs;

		for (int i = 0; i < g_upcie_cplane_serve.ndevs; ++i) {
			if (devs & (UINT64_C(1) << (unsigned)i)) {
				xnvme_be_upcie_cplane_serve_owe(conn, i);
			}
		}

		return;
	}

	for (int i = 0; i < conn->nallocs; ++i) {
		if (xnvme_be_upcie_cplane_serve_buf_free(conn->allocs[i])) {
			XNVME_DEBUG("FAILED: reclaim(0x%" PRIx64 ")", conn->allocs[i]);
		}
	}

	close(conn->sock);
	xnvme_be_upcie_cplane_serve_conn_wipe(conn);
}

int
xnvme_cplane_serve(struct xnvme_dev **devs, int ndevs, uint32_t cplane_id,
		   volatile sig_atomic_t *stop, void (*ready)(void *), void *ready_arg)
{
	struct xnvme_be_upcie_cplane_share shares[XNVME_BE_UPCIE_CPLANE_SERVE_DEVS_MAX] = {0};
	struct sockaddr_un addr = {.sun_family = AF_UNIX};
	char path[256] = {0};
	int listener = -1;
	mode_t mask;
	int nshared = 0;
	int nadmin = 0;
	int err = 0;

	if (!devs || (ndevs < 1) || !stop) {
		return -EINVAL;
	}
	if (g_upcie_cplane_serve.running) {
		XNVME_DEBUG("FAILED: this process is already serving");
		return -EBUSY;
	}
	if (ndevs > XNVME_BE_UPCIE_CPLANE_SERVE_DEVS_MAX) {
		/* Not -ENOSYS, which tells the caller to hold the devices without serving */
		XNVME_DEBUG("FAILED: serving %d devices; the cap is %d", ndevs,
			    XNVME_BE_UPCIE_CPLANE_SERVE_DEVS_MAX);
		return -EOPNOTSUPP;
	}

	/* upcie-cuda and upcie-hip share a controller the way upcie does */
	if (strncmp(devs[0]->be.attr.name, "upcie", 5)) {
		XNVME_DEBUG("FAILED: serving is uPCIe's; be(%s) has its own arrangement",
			    devs[0]->be.attr.name);
		return -ENOSYS;
	}

	memset(&g_upcie_cplane_serve, 0, sizeof(g_upcie_cplane_serve));
	g_upcie_cplane_serve.running = 1;
	g_upcie_cplane_serve.devs = devs;
	g_upcie_cplane_serve.shares = shares;
	g_upcie_cplane_serve.ndevs = ndevs;
	g_upcie_cplane_serve.wake[0] = g_upcie_cplane_serve.wake[1] = -1;

	for (int i = 0; i < XNVME_BE_UPCIE_CPLANE_SERVE_CONNS_MAX; ++i) {
		g_upcie_cplane_serve.conns[i].sock = -1;
		g_upcie_cplane_serve.conns[i].in_fd = -1;
	}

	/* The reader drains it until empty, and a writer must not wait on a full one */
	if (pipe2(g_upcie_cplane_serve.wake, O_NONBLOCK)) {
		g_upcie_cplane_serve.running = 0;
		return -errno;
	}

	for (nshared = 0; nshared < ndevs; ++nshared) {
		err = xnvme_be_upcie_cplane_share_get(devs[nshared], &shares[nshared]);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_be_upcie_cplane_share_get(%d); err(%d)",
				    nshared, err);
			goto exit;
		}
	}

	/* Zero totals mean the controller did not answer, as the status reply defines */
	{
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(devs[0]);

		if (xnvme_adm_gfeat_nqueues(&ctx, &g_upcie_cplane_serve.nsq_total,
					    &g_upcie_cplane_serve.ncq_total)) {
			XNVME_DEBUG("INFO: controller did not report its queue counts");
			g_upcie_cplane_serve.nsq_total = 0;
			g_upcie_cplane_serve.ncq_total = 0;
		}
	}

	xnvme_be_upcie_cplane_socket_path(cplane_id, path, sizeof(path));
	unlink(path);

	/* accept() is drained in a loop, which must end when nobody waits */
	listener = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);
	if (listener < 0) {
		err = -errno;
		goto exit;
	}

	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);

	/* Connecting takes write permission, so the socket is created for this
	 * user alone; set by umask, as a chmod() after bind() leaves a window */
	mask = umask(0177);
	err = bind(listener, (struct sockaddr *)&addr, sizeof(addr)) ? -errno : 0;
	umask(mask);
	/* Deep enough for a burst of probes, which read a refusal as no server */
	if (err || listen(listener, XNVME_BE_UPCIE_CPLANE_SERVE_CONNS_MAX)) {
		err = err ? err : -errno;
		goto exit;
	}

	for (nadmin = 0; nadmin < ndevs; ++nadmin) {
		struct xnvme_be_upcie_cplane_serve_admin *admin =
			&g_upcie_cplane_serve.admin[nadmin];

		admin->devidx = nadmin;
		pthread_mutex_init(&admin->lock, NULL);
		pthread_cond_init(&admin->cond, NULL);
		err = -pthread_create(&admin->tid, NULL, xnvme_be_upcie_cplane_serve_admin_main,
				      admin);
		if (err) {
			XNVME_DEBUG("FAILED: pthread_create(admin %d); err(%d)", nadmin, err);
			goto exit;
		}
		admin->started = 1;
	}

	if (ready) {
		ready(ready_arg);
	}

	while (!*stop) {
		struct pollfd pfds[XNVME_BE_UPCIE_CPLANE_SERVE_CONNS_MAX + 2];
		struct xnvme_be_upcie_cplane_serve_conn
			*polled[XNVME_BE_UPCIE_CPLANE_SERVE_CONNS_MAX];
		int npolled = 0;
		int npfds = 0;

		pfds[npfds].fd = listener;
		pfds[npfds].events = POLLIN;
		npfds++;

		pfds[npfds].fd = g_upcie_cplane_serve.wake[0];
		pfds[npfds].events = POLLIN;
		npfds++;

		pthread_mutex_lock(&g_upcie_cplane_serve_lock);
		for (int i = 0; i < XNVME_BE_UPCIE_CPLANE_SERVE_CONNS_MAX; ++i) {
			if ((g_upcie_cplane_serve.conns[i].sock < 0) ||
			    g_upcie_cplane_serve.conns[i].owed) {
				continue;
			}
			pfds[npfds].fd = g_upcie_cplane_serve.conns[i].sock;
			pfds[npfds].events = POLLIN;
			npfds++;
			polled[npolled++] = &g_upcie_cplane_serve.conns[i];
		}
		pthread_mutex_unlock(&g_upcie_cplane_serve_lock);

		/* The timeout is what notices `*stop` */
		if (poll(pfds, (nfds_t)npfds, 1000) < 0) {
			if (errno == EINTR) {
				continue;
			}
			err = -errno;
			break;
		}

		while (pfds[0].revents & POLLIN) {
			struct xnvme_be_upcie_cplane_serve_conn *conn = NULL;
			int sock = accept(listener, NULL, NULL);

			if (sock < 0) {
				break; /* Drained */
			}
			if (xnvme_be_upcie_cplane_peer_check(sock, 0)) {
				close(sock);
				continue;
			}

			pthread_mutex_lock(&g_upcie_cplane_serve_lock);
			for (int s = 0; s < XNVME_BE_UPCIE_CPLANE_SERVE_CONNS_MAX; ++s) {
				if (g_upcie_cplane_serve.conns[s].sock < 0) {
					conn = &g_upcie_cplane_serve.conns[s];
					break;
				}
			}
			if (conn) {
				xnvme_be_upcie_cplane_serve_conn_wipe(conn);
				conn->sock = sock;
			}
			pthread_mutex_unlock(&g_upcie_cplane_serve_lock);

			if (!conn) {
				close(sock);
			}
		}

		if (pfds[1].revents & POLLIN) {
			char drain[64];

			while (read(g_upcie_cplane_serve.wake[0], drain, sizeof(drain)) > 0) {
				;
			}
		}

		for (int i = 0; i < npolled; ++i) {
			struct xnvme_be_upcie_cplane_serve_conn *conn = polled[i];
			short revents = pfds[2 + i].revents;
			struct nvme_cplane_msg reply = {0};
			int fds[NVME_CPLANE_FDS_MAX];
			enum xnvme_be_upcie_cplane_serve_reply send;
			uint32_t nfds = 0;
			int sock, rc;

			if (!revents) {
				continue;
			}

			pthread_mutex_lock(&g_upcie_cplane_serve_lock);
			if ((conn->sock < 0) || conn->owed) {
				pthread_mutex_unlock(&g_upcie_cplane_serve_lock);
				continue;
			}

			{
				int in[NVME_CPLANE_FDS_MAX];
				uint32_t nin = 0;

				rc = nvme_cplane_msg_tryrecv(conn->sock, &conn->msg, in, &nin);

				/* The first descriptor is the request's; any more are closed */
				for (uint32_t f = 0; f < nin; ++f) {
					if ((f == 0) && (conn->in_fd < 0)) {
						conn->in_fd = in[f];
						continue;
					}
					close(in[f]);
				}
			}
			if (rc == -EAGAIN) {
				pthread_mutex_unlock(&g_upcie_cplane_serve_lock);
				continue;
			}
			if (rc) {
				xnvme_be_upcie_cplane_serve_drop(conn);
				pthread_mutex_unlock(&g_upcie_cplane_serve_lock);
				continue;
			}

			send = xnvme_be_upcie_cplane_serve_dispatch(conn, &reply, fds, &nfds);
			sock = conn->sock;
			/* A request answered here is done with its descriptor */
			if ((send != XNVME_BE_UPCIE_CPLANE_SERVE_REPLY_NONE) &&
			    (conn->in_fd >= 0)) {
				close(conn->in_fd);
				conn->in_fd = -1;
			}
			pthread_mutex_unlock(&g_upcie_cplane_serve_lock);

			if (send == XNVME_BE_UPCIE_CPLANE_SERVE_REPLY_NONE) {
				continue;
			}

			/* Never waited on: a peer that stopped reading is dropped */
			if (nvme_cplane_msg_trysend(sock, &reply, nfds ? fds : NULL, nfds) ||
			    (send == XNVME_BE_UPCIE_CPLANE_SERVE_REPLY_LAST)) {
				pthread_mutex_lock(&g_upcie_cplane_serve_lock);
				if (conn->sock == sock) {
					xnvme_be_upcie_cplane_serve_drop(conn);
				}
				pthread_mutex_unlock(&g_upcie_cplane_serve_lock);
			}
		}
	}

exit:
	/* Clients learn of the shutdown before their queues are deleted, see
	 * xnvme_be_upcie_cplane_queue_poke_idle() */
	pthread_mutex_lock(&g_upcie_cplane_serve_lock);
	for (int i = 0; i < XNVME_BE_UPCIE_CPLANE_SERVE_CONNS_MAX; ++i) {
		if (g_upcie_cplane_serve.conns[i].sock >= 0) {
			shutdown(g_upcie_cplane_serve.conns[i].sock, SHUT_RDWR);
		}
	}
	pthread_mutex_unlock(&g_upcie_cplane_serve_lock);

	for (int i = 0; i < nadmin; ++i) {
		struct xnvme_be_upcie_cplane_serve_admin *admin = &g_upcie_cplane_serve.admin[i];

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

	/* The controllers' threads are gone, so their releases run here */
	for (int i = 0; i < XNVME_BE_UPCIE_CPLANE_SERVE_CONNS_MAX; ++i) {
		struct xnvme_be_upcie_cplane_serve_conn *conn = &g_upcie_cplane_serve.conns[i];

		if (conn->sock < 0) {
			continue;
		}

		conn->closing_devs = 0;
		for (int q = 0; q < conn->nqpairs; ++q) {
			conn->closing_devs |= UINT64_C(1) << (unsigned)conn->qpairs[q].dev;
		}
		for (int r = 0; r < conn->nregs; ++r) {
			conn->closing_devs |= UINT64_C(1) << (unsigned)conn->regs[r].dev;
		}
		conn->closing_devs |= UINT64_C(1); /* Releases the slot even without queues */

		for (int d = 0; d < ndevs; ++d) {
			if (conn->closing_devs & (UINT64_C(1) << (unsigned)d)) {
				xnvme_be_upcie_cplane_serve_conn_release(conn, d);
			}
		}
	}

	if (listener >= 0) {
		close(listener);
		unlink(path);
	}
	for (int i = 0; i < nshared; ++i) {
		xnvme_be_upcie_cplane_share_put(&shares[i]);
	}
	for (int i = 0; i < 2; ++i) {
		if (g_upcie_cplane_serve.wake[i] >= 0) {
			close(g_upcie_cplane_serve.wake[i]);
		}
	}

	g_upcie_cplane_serve.running = 0;

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
