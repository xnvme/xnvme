// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/*
 * The client side of the control plane: maps the heap and BAR0 a server hands
 * over, and asks the server for queues, buffers and admin commands.
 */
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_queue.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include <xnvme_be_upcie.h>

/* Keeps a queue's request pool off the cache lines of its neighbours */
#define XNVME_BE_UPCIE_CACHE_LINE 64

/* The stride nvme_request_pool_init_prps_dmamem() carves the server's PRP scratch with */
#define XNVME_BE_UPCIE_PRP_NBYTES 4096

/* Idle pokes between looks at the server's socket; keeps recv() off the fast path */
#define XNVME_BE_UPCIE_POKES_IDLE_MAX 1024

/*
 * Serialises a request against its reply on the runtime's socket. Not a member
 * of the connection, so that letting go of a runtime can zero the connection
 * without destroying a lock another thread is about to take.
 */
static pthread_mutex_t cplane_sock_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Held from writing a controller's admin PRP scratch until the reply, since
 * there is one scratch per controller. Its own lock rather than xnvme_lock(),
 * which would stop the process's other threads for as long as a Format runs.
 */
static pthread_mutex_t cplane_admin_lock = PTHREAD_MUTEX_INITIALIZER;

void
xnvme_be_upcie_cplane_socket_path(uint32_t cplane_id, char *path, size_t nbytes)
{
	snprintf(path, nbytes, "/tmp/xnvme-homi-%u.sock", cplane_id);
}

int
xnvme_be_upcie_cplane_connect(const char *path)
{
	struct sockaddr_un addr = {.sun_family = AF_UNIX};
	int sock, err;

	if (!path) {
		return -EINVAL;
	}

	sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (sock < 0) {
		return -errno;
	}
	/* Zero is what an unconnected runtime holds, so the socket must not be it */
	if (!sock) {
		int moved = fcntl(sock, F_DUPFD, 1);

		close(sock);
		if (moved < 0) {
			return -errno;
		}
		sock = moved;
	}
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr))) {
		err = -errno;
		close(sock);

		return ((err == -ENOENT) || (err == -ECONNREFUSED)) ? -ENOENT : err;
	}

	/* The path is in a directory anyone may write, so another user could be
	 * listening on it; what a server hands over is trusted, so refuse one
	 * that is neither root nor this user. */
	err = xnvme_be_upcie_cplane_peer_check(sock, 1);
	if (err) {
		close(sock);
		return err;
	}

	return sock;
}

int
xnvme_be_upcie_cplane_peer_check(int sock, int root_ok)
{
	struct ucred cred = {0};
	int err;

	err = nvme_cplane_peer_cred(sock, &cred);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_cplane_peer_cred(); err(%d)", err);
		return err;
	}
	if ((cred.uid != geteuid()) && !(root_ok && (cred.uid == 0))) {
		XNVME_DEBUG("FAILED: peer pid(%d) runs as uid(%u)", cred.pid, cred.uid);
		return -EPERM;
	}

	return 0;
}

/* Whether `nbytes` from `offset` lies within the server's heap */
static bool
cplane_in_heap(uint64_t offset, uint64_t nbytes)
{
	uint64_t heap_nbytes = g_upcie_rte.connection.heap_nbytes;

	return (offset <= heap_nbytes) && (nbytes <= heap_nbytes - offset);
}

/* Whether the description at `offset`, table included, lies within a heap of `heap_nbytes` */
static bool
cplane_desc_fits(const char *heap_base, uint64_t heap_nbytes, uint64_t offset)
{
	const struct hostmem_shared_desc *desc;

	if ((offset > heap_nbytes) || (heap_nbytes - offset < sizeof(*desc))) {
		return false;
	}
	desc = (const struct hostmem_shared_desc *)(heap_base + offset);

	return hostmem_shared_desc_nbytes(desc->nphys) <= heap_nbytes - offset;
}

int
xnvme_be_upcie_cplane_ask(int sock, struct nvme_cplane_msg *msg, int *fds, uint32_t *nfds)
{
	if (sock < 0) {
		return -ENOTCONN;
	}

	return nvme_cplane_request(sock, msg, fds, nfds);
}

int
xnvme_be_upcie_cplane_ask_ctrlr(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_cplane_msg *msg,
				int *fds, uint32_t *nfds)
{
	int err;

	if (!ctrlr || !msg) {
		return -ENOTCONN;
	}

	msg->index = (uint32_t)ctrlr->index;

	pthread_mutex_lock(&cplane_sock_lock);
	err = xnvme_be_upcie_cplane_ask(g_upcie_rte.connection.sock, msg, fds, nfds);
	pthread_mutex_unlock(&cplane_sock_lock);

	return err;
}

int
xnvme_be_upcie_cplane_sync_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes)
{
	struct xnvme_be_upcie_state *state = (void *)ctx->dev->be.state;
	struct nvme_command *cmd = (struct nvme_command *)&ctx->cmd;
	struct nvme_cplane_msg msg = {0};
	int err;

	pthread_mutex_lock(&cplane_admin_lock);
	if (dbuf) {
		struct nvme_request *prp = xnvme_be_upcie_ctrlr_admin_prp(state->ctrlr);

		if (!prp) {
			err = -errno;
			XNVME_DEBUG("FAILED: admin PRP scratch; err(%d)", err);
			pthread_mutex_unlock(&cplane_admin_lock);
			return err;
		}

		err = nvme_request_prep_command_prps_contig_dmamem(prp, state->dmem, dbuf,
								   dbuf_nbytes, cmd);
		if (err) {
			XNVME_DEBUG("FAILED: prps_contig_dmamem(); err(%d)", err);
			pthread_mutex_unlock(&cplane_admin_lock);
			return err;
		}
	}

	msg.op = NVME_CPLANE_OP_ADMIN_CMD;
	memcpy(&msg.u.admin_cmd.cmd, cmd, sizeof(msg.u.admin_cmd.cmd));

	err = xnvme_be_upcie_cplane_ask_ctrlr(state->ctrlr, &msg, NULL, NULL);
	pthread_mutex_unlock(&cplane_admin_lock);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_cplane_ask_ctrlr(); err(%d)", err);
		return err;
	}

	memcpy(&ctx->cpl, &msg.u.admin_cmd.cpl, sizeof(ctx->cpl));

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		XNVME_DEBUG("FAILED: command; sc(%d); sct(%d)", ctx->cpl.status.sc,
			    ctx->cpl.status.sct);
		return -EIO;
	}

	return 0;
}

void *
xnvme_be_upcie_cplane_buf_alloc(struct xnvme_be_upcie_ctrlr *ctrlr, size_t nbytes, uint64_t *phys)
{
	struct nvme_cplane_msg msg = {0};
	void *buf;
	int err;

	if (!ctrlr) {
		errno = EINVAL;
		return NULL;
	}

	msg.op = NVME_CPLANE_OP_ALLOC_BUF;
	msg.u.buf.nbytes = nbytes;

	err = xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL);
	if (err) {
		errno = -err;
		return NULL;
	}
	if (!cplane_in_heap(msg.u.buf.offset, nbytes)) {
		XNVME_DEBUG("FAILED: offset(0x%" PRIx64 ") is outside the heap", msg.u.buf.offset);
		errno = EPROTO;
		return NULL;
	}

	buf = (char *)g_upcie_rte.connection.heap_base + msg.u.buf.offset;
	if (phys) {
		*phys = dmamem_va_to_iova(&g_upcie_rte.mem.dmem, buf);
	}

	return buf;
}

void
xnvme_be_upcie_cplane_buf_free(struct xnvme_be_upcie_ctrlr *ctrlr, void *buf)
{
	struct nvme_cplane_msg msg = {0};

	msg.op = NVME_CPLANE_OP_FREE_BUF;
	msg.u.buf.offset = (uint64_t)((char *)buf - (char *)g_upcie_rte.connection.heap_base);

	if (!ctrlr || xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL)) {
		XNVME_DEBUG("FAILED: giving back offset(0x%" PRIx64 ")", msg.u.buf.offset);
	}
}

/* The server's index for `bdf`, matched against the identifiers it publishes */
static int
cplane_index_of(int sock, const char *bdf)
{
	struct nvme_cplane_msg msg = {0};
	uint32_t ndevs;
	char wanted[16] = {0};

	/* Truncated as the server truncates what it publishes */
	snprintf(wanted, sizeof(wanted), "%s", bdf);

	if (xnvme_be_upcie_cplane_ask_status(sock, 0, &msg)) {
		return -ENOTCONN;
	}

	ndevs = msg.u.status.ndevs;
	for (uint32_t i = 0; i < ndevs; ++i) {
		if (i && xnvme_be_upcie_cplane_ask_status(sock, i, &msg)) {
			return -ENOTCONN;
		}
		if (!strcmp(msg.u.status.bdf, wanted)) {
			return (int)i;
		}
	}

	return -ENODEV;
}

/* The runtime's socket, connected on first use; callers hold cplane_sock_lock */
static int
cplane_conn_get(uint32_t cplane_id)
{
	char path[256] = {0};
	int sock;

	if (g_upcie_rte.connection.sock > 0) {
		return g_upcie_rte.connection.sock;
	}

	xnvme_be_upcie_cplane_socket_path(cplane_id, path, sizeof(path));

	sock = xnvme_be_upcie_cplane_connect(path);
	if (sock < 0) {
		return sock;
	}

	g_upcie_rte.connection.sock = sock;

	return sock;
}

int
xnvme_be_upcie_cplane_ask_ctrlr_fd(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_cplane_msg *msg,
				   int fd)
{
	int err;

	if (!ctrlr || !msg) {
		return -ENOTCONN;
	}

	msg->index = (uint32_t)ctrlr->index;

	pthread_mutex_lock(&cplane_sock_lock);
	err = nvme_cplane_request_with(g_upcie_rte.connection.sock, msg, &fd, 1, NULL, NULL);
	pthread_mutex_unlock(&cplane_sock_lock);

	return err;
}

int
xnvme_be_upcie_cplane_register_client_mem(struct xnvme_be_upcie_ctrlr *ctrlr, int dmabuf_fd,
					  uint64_t nbytes, uint32_t page_size,
					  const struct hostmem_shared_desc **desc_out,
					  uint64_t *offset_out)
{
	struct nvme_cplane_msg msg = {0};
	int err;

	if (!ctrlr || (dmabuf_fd < 0) || !nbytes || !page_size || !desc_out) {
		return -EINVAL;
	}
	if (!g_upcie_rte.connection.alive) {
		return -ENOTCONN;
	}

	msg.op = NVME_CPLANE_OP_REGISTER_MEM;
	msg.u.mem.nbytes = nbytes;
	msg.u.mem.page_size = page_size;

	err = xnvme_be_upcie_cplane_ask_ctrlr_fd(ctrlr, &msg, dmabuf_fd);
	if (err) {
		XNVME_DEBUG("FAILED: registering %" PRIu64 " bytes; err(%d)", nbytes, err);
		return err;
	}
	if (!cplane_desc_fits(g_upcie_rte.connection.heap_base, g_upcie_rte.connection.heap_nbytes,
			      msg.u.mem.desc_offset)) {
		XNVME_DEBUG("FAILED: the description lies outside the heap");
		return -EPROTO;
	}

	*desc_out = (const struct hostmem_shared_desc *)((char *)g_upcie_rte.connection.heap_base +
							 msg.u.mem.desc_offset);
	if (offset_out) {
		*offset_out = msg.u.mem.desc_offset;
	}

	return 0;
}

int
xnvme_be_upcie_cplane_unregister_client_mem(struct xnvme_be_upcie_ctrlr *ctrlr, uint64_t offset)
{
	struct nvme_cplane_msg msg = {0};

	if (!ctrlr || !offset) {
		return -EINVAL;
	}

	msg.op = NVME_CPLANE_OP_UNREGISTER_MEM;
	msg.u.mem.desc_offset = offset;

	return xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL);
}

int
xnvme_be_upcie_cplane_init_connection(uint32_t cplane_id, const char *bdf,
				      struct xnvme_be_upcie_ctrlr *ctrlr)
{
	struct nvme_cplane_msg msg = {0};
	const struct hostmem_shared_desc *desc;
	int fds[NVME_CPLANE_FDS_MAX];
	uint32_t nfds = 0;
	void *heap_base = MAP_FAILED;
	void *bar0 = MAP_FAILED;
	int index = 0;
	int sock, err;

	pthread_mutex_lock(&cplane_sock_lock);

	sock = cplane_conn_get(cplane_id);
	if (sock < 0) {
		pthread_mutex_unlock(&cplane_sock_lock);

		return sock;
	}

	if (bdf) {
		index = cplane_index_of(sock, bdf);
		if (index < 0) {
			XNVME_DEBUG("FAILED: nobody serves bdf(%s); err(%d)", bdf, index);
			err = index;
			goto failed;
		}
	}

	msg.op = NVME_CPLANE_OP_INIT_CONNECTION;
	msg.index = (uint32_t)index;
	err = xnvme_be_upcie_cplane_ask(sock, &msg, fds, &nfds);
	if (err || (nfds != 2)) {
		XNVME_DEBUG("FAILED: init-connection; err(%d) nfds(%u)", err, nfds);
		/* Descriptors that arrived are this process's to close */
		for (uint32_t i = 0; i < nfds; ++i) {
			close(fds[i]);
		}
		err = err ? err : -EPROTO;
		goto failed;
	}

	/* Every reply carries the heap; only the first controller maps it */
	if (g_upcie_rte.connection.alive) {
		heap_base = g_upcie_rte.connection.heap_base;
		close(fds[0]);
		if (msg.u.init_connection.heap_nbytes != g_upcie_rte.connection.heap_nbytes) {
			XNVME_DEBUG("FAILED: the server names a heap of another size");
			close(fds[1]);
			err = -EPROTO;
			goto failed;
		}
	} else {
		heap_base = mmap(NULL, msg.u.init_connection.heap_nbytes, PROT_READ | PROT_WRITE,
				 MAP_SHARED, fds[0], 0);
		close(fds[0]);
		if (heap_base == MAP_FAILED) {
			XNVME_DEBUG("FAILED: mmap(heap); errno(%d)", errno);
			err = -errno;
			close(fds[1]);
			goto failed;
		}
	}

	bar0 = mmap(NULL, msg.u.init_connection.bar0_nbytes, PROT_READ | PROT_WRITE, MAP_SHARED,
		    fds[1], 0);
	close(fds[1]);
	if (bar0 == MAP_FAILED) {
		XNVME_DEBUG("FAILED: mmap(BAR0); errno(%d)", errno);
		err = -errno;
		goto failed;
	}

	/* The base the server opened the controller with must match this BAR0 */
	if (msg.u.init_connection.ctrlr.cap != nvme_mmio_cap_read(bar0)) {
		XNVME_DEBUG("FAILED: the server's CAP is not what this BAR0 reads");
		err = -EPROTO;
		goto failed;
	}
	if (!cplane_desc_fits(heap_base, msg.u.init_connection.heap_nbytes,
			      msg.u.init_connection.desc_offset)) {
		XNVME_DEBUG("FAILED: the heap description lies outside the heap");
		err = -EPROTO;
		goto failed;
	}

	/* The server describes the heap's device addresses; this process cannot read them */
	desc = (const struct hostmem_shared_desc *)((char *)heap_base +
						    msg.u.init_connection.desc_offset);

	if (!g_upcie_rte.connection.alive) {
		err = dmamem_from_shared_hostmem(&g_upcie_rte.mem.dmem, heap_base, desc,
						 xnvme_be_upcie_va_bits());
		if (err) {
			XNVME_DEBUG("FAILED: dmamem_from_shared_hostmem(); err(%d)", err);
			goto failed;
		}
		g_upcie_rte.mem.dmem_alive = 1;

		g_upcie_rte.connection.heap_base = heap_base;
		g_upcie_rte.connection.heap_nbytes = msg.u.init_connection.heap_nbytes;
		g_upcie_rte.connection.alive = 1;
	}

	/* Without a controller the caller wanted the heap alone; the socket stays */
	if (!ctrlr) {
		munmap(bar0, msg.u.init_connection.bar0_nbytes);
		pthread_mutex_unlock(&cplane_sock_lock);

		return 0;
	}

	ctrlr->index = index;
	ctrlr->bar0 = bar0;
	ctrlr->bar0_nbytes = msg.u.init_connection.bar0_nbytes;
	ctrlr->base = msg.u.init_connection.ctrlr;
	snprintf(ctrlr->bdf, sizeof(ctrlr->bdf), "%s", msg.u.init_connection.bdf);
	ctrlr->attached = 1;

	pthread_mutex_unlock(&cplane_sock_lock);

	return 0;

failed:
	if (bar0 != MAP_FAILED) {
		munmap(bar0, msg.u.init_connection.bar0_nbytes);
	}
	if ((heap_base != MAP_FAILED) && !g_upcie_rte.connection.alive) {
		munmap(heap_base, msg.u.init_connection.heap_nbytes);
	}

	/* Once the runtime is up the socket is its to close, in _rte_term() */
	if (!g_upcie_rte.connection.alive) {
		close(g_upcie_rte.connection.sock);
		g_upcie_rte.connection.sock = 0;
	}

	pthread_mutex_unlock(&cplane_sock_lock);

	return err;
}

int
xnvme_be_upcie_cplane_ask_status(int sock, uint32_t index, struct nvme_cplane_msg *msg)
{
	if ((sock < 0) || !msg) {
		return -EINVAL;
	}

	memset(msg, 0, sizeof(*msg));
	msg->op = NVME_CPLANE_OP_STATUS;
	msg->index = index;

	return nvme_cplane_request(sock, msg, NULL, NULL);
}

int
xnvme_be_upcie_cplane_ctrlr_from_base(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	struct nvme_controller *ctrl;

	if (!ctrlr || !ctrlr->ctrl || !ctrlr->attached) {
		return -ENOTCONN;
	}

	ctrl = ctrlr->ctrl;

	memset(ctrl, 0, sizeof(*ctrl));
	ctrl->base = ctrlr->base;
	ctrl->func.bars[0].region = ctrlr->bar0;
	ctrl->func.bars[0].size = ctrlr->bar0_nbytes;
	ctrl->func.bars[0].fd = -1; /* The server holds it */
	snprintf(ctrl->func.bdf, sizeof(ctrl->func.bdf), "%s", ctrlr->bdf);

	/* The admin queue is the server's, so it stays empty here */

	return 0;
}

int
xnvme_be_upcie_cplane_alloc_qpair_at(struct xnvme_be_upcie_ctrlr *ctrlr, uint64_t desc_offset,
				     uint64_t sq_offset, uint64_t cq_offset, uint16_t depth,
				     uint32_t *qid)
{
	struct nvme_cplane_msg msg = {0};
	int err;

	if (!ctrlr || !qid) {
		return -EINVAL;
	}

	msg.op = NVME_CPLANE_OP_ALLOC_IOQPAIR_AT;
	msg.u.alloc_ioqpair_at.desc_offset = desc_offset;
	msg.u.alloc_ioqpair_at.sq_offset = sq_offset;
	msg.u.alloc_ioqpair_at.cq_offset = cq_offset;
	msg.u.alloc_ioqpair_at.depth = depth;

	err = xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: asking for a queue on registered memory; err(%d)", err);
		return err;
	}

	*qid = msg.u.alloc_ioqpair_at.allocation.qid;

	return 0;
}

int
xnvme_be_upcie_cplane_free_qpair_at(struct xnvme_be_upcie_ctrlr *ctrlr, uint32_t qid)
{
	struct nvme_cplane_msg msg = {0};

	if (!ctrlr) {
		return -EINVAL;
	}

	msg.op = NVME_CPLANE_OP_FREE_IOQPAIR;
	msg.u.free_ioqpair.qid = qid;

	return xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL);
}

/* Map the queue pair the server allocated, with doorbells in this process's BAR0 mapping */
static int
_qpair_from_allocation(struct xnvme_be_upcie_ctrlr *ctrlr, const struct nvme_ioqpair *allocation,
		       struct nvme_qpair *qpair)
{
	char *base = g_upcie_rte.connection.heap_base;
	int dstrd = nvme_reg_cap_get_dstrd(nvme_mmio_cap_read(ctrlr->bar0));
	void *rpool = NULL;
	size_t nbytes;
	int err;

	memset(qpair, 0, sizeof(*qpair));
	if (!cplane_in_heap(allocation->sq_offset,
			    (uint64_t)allocation->depth * sizeof(struct nvme_command)) ||
	    !cplane_in_heap(allocation->cq_offset,
			    (uint64_t)allocation->depth * sizeof(struct nvme_completion)) ||
	    !cplane_in_heap(allocation->prp_offset,
			    (uint64_t)NVME_REQUEST_POOL_LEN * XNVME_BE_UPCIE_PRP_NBYTES)) {
		XNVME_DEBUG("FAILED: the server placed queue(%u) outside the heap",
			    allocation->qid);
		return -EPROTO;
	}
	qpair->qid = allocation->qid;
	qpair->depth = allocation->depth;
	qpair->sq = base + allocation->sq_offset;
	qpair->cq = base + allocation->cq_offset;
	if ((size_t)XNVME_BE_UPCIE_DOORBELL_OFFSET + ((2 * qpair->qid + 1) << (2 + dstrd)) + 4 >
	    ctrlr->bar0_nbytes) {
		XNVME_DEBUG("FAILED: the doorbells of queue(%u) lie outside BAR0", qpair->qid);
		return -EPROTO;
	}
	qpair->sqdb = (char *)ctrlr->bar0 + XNVME_BE_UPCIE_DOORBELL_OFFSET +
		      ((2 * qpair->qid) << (2 + dstrd));
	qpair->cqdb = (char *)ctrlr->bar0 + XNVME_BE_UPCIE_DOORBELL_OFFSET +
		      ((2 * qpair->qid + 1) << (2 + dstrd));
	qpair->tail_last_written = UINT16_MAX;
	qpair->phase = 1;

	/* The pool's top is written on every I/O; sharing its cache line with the
	 * next queue's header, as calloc() can, cost each queue a quarter of its
	 * throughput when two threads drive neighbouring queues. */
	nbytes = (sizeof(*qpair->rpool) + XNVME_BE_UPCIE_CACHE_LINE - 1) &
		 ~(size_t)(XNVME_BE_UPCIE_CACHE_LINE - 1);
	err = posix_memalign(&rpool, XNVME_BE_UPCIE_CACHE_LINE, nbytes);
	if (err) {
		return -err;
	}
	memset(rpool, 0, nbytes);
	qpair->rpool = rpool;
	nvme_request_pool_init(qpair->rpool);

	/* Strided as the server allocated; a client's runtime config is never filled in */
	for (uint16_t i = 0; i < NVME_REQUEST_POOL_LEN; ++i) {
		void *prp =
			base + allocation->prp_offset + ((size_t)i * XNVME_BE_UPCIE_PRP_NBYTES);

		qpair->rpool->reqs[i].prp = prp;
		qpair->rpool->reqs[i].prp_addr = dmamem_va_to_iova(&g_upcie_rte.mem.dmem, prp);
	}
	qpair->rpool->prps = base + allocation->prp_offset;

	return 0;
}

int
xnvme_be_upcie_cplane_alloc_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_qpair *qpair,
				  uint16_t depth)
{
	struct nvme_cplane_msg msg = {0};
	int err;

	if (!qpair || !ctrlr || !ctrlr->bar0) {
		return -ENOTCONN;
	}

	msg.op = NVME_CPLANE_OP_ALLOC_IOQPAIR;
	msg.u.alloc_ioqpair.depth = depth;

	err = xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: asking for a queue; err(%d)", err);
		return err;
	}

	return _qpair_from_allocation(ctrlr, &msg.u.alloc_ioqpair.allocation, qpair);
}

int
xnvme_be_upcie_cplane_alloc_qpair_cq_at(struct xnvme_be_upcie_ctrlr *ctrlr,
					struct nvme_qpair *qpair, uint16_t depth,
					uint64_t desc_offset, uint64_t cq_offset)
{
	struct nvme_cplane_msg msg = {0};
	int err;

	if (!qpair || !ctrlr || !ctrlr->bar0) {
		return -ENOTCONN;
	}

	msg.op = NVME_CPLANE_OP_ALLOC_IOQPAIR_AT;
	msg.u.alloc_ioqpair_at.desc_offset = desc_offset;
	msg.u.alloc_ioqpair_at.sq_offset = NVME_CPLANE_QUEUE_IN_HEAP;
	msg.u.alloc_ioqpair_at.cq_offset = cq_offset;
	msg.u.alloc_ioqpair_at.depth = depth;

	err = xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: asking for a queue with its CQ in registered memory; err(%d)",
			    err);
		return err;
	}

	return _qpair_from_allocation(ctrlr, &msg.u.alloc_ioqpair_at.allocation, qpair);
}

struct nvme_qpair *
xnvme_be_upcie_ctrlr_ioq(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	int err;

	if (!ctrlr) {
		errno = EINVAL;
		return NULL;
	}
	if (ctrlr->sync.rpool) {
		return &ctrlr->sync;
	}
	if (!g_upcie_rte.connection.alive) {
		/* The owner creates it at open, so missing means the controller never came up */
		errno = ENOTCONN;
		return NULL;
	}

	err = xnvme_be_upcie_cplane_alloc_qpair(ctrlr, &ctrlr->sync, 16);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_cplane_alloc_qpair(); err(%d)", err);
		errno = -err;
		return NULL;
	}

	return &ctrlr->sync;
}

struct nvme_request *
xnvme_be_upcie_ctrlr_admin_prp(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	if (!ctrlr) {
		errno = EINVAL;
		return NULL;
	}
	if (ctrlr->admin_prp.prp) {
		return &ctrlr->admin_prp;
	}

	ctrlr->admin_prp.prp =
		xnvme_be_upcie_cplane_buf_alloc(ctrlr, 4096, &ctrlr->admin_prp.prp_addr);
	if (!ctrlr->admin_prp.prp) {
		XNVME_DEBUG("FAILED: allocating admin PRP scratch; errno(%d)", errno);
		return NULL;
	}

	return &ctrlr->admin_prp;
}

void
xnvme_be_upcie_ctrlr_admin_prp_release(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	if (!ctrlr || !ctrlr->admin_prp.prp) {
		return;
	}

	xnvme_be_upcie_cplane_buf_free(ctrlr, ctrlr->admin_prp.prp);
	memset(&ctrlr->admin_prp, 0, sizeof(ctrlr->admin_prp));
}

void
xnvme_be_upcie_cplane_free_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_qpair *qpair)
{
	struct nvme_cplane_msg msg = {0};

	if (!ctrlr || !qpair || !qpair->qid) {
		return;
	}

	msg.op = NVME_CPLANE_OP_FREE_IOQPAIR;
	msg.u.free_ioqpair.qid = qpair->qid;

	if (xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL)) {
		XNVME_DEBUG("FAILED: handing back qid(%u)", qpair->qid);
	}

	free(qpair->rpool);
	memset(qpair, 0, sizeof(*qpair));
}

/* The server never sends unsolicited, so anything readable means it closed */
static int
_server_gone(int sock)
{
	char byte;
	ssize_t nbytes;

	if (sock < 0) {
		return 1;
	}

	nbytes = recv(sock, &byte, 1, MSG_PEEK | MSG_DONTWAIT);
	if (nbytes < 0) {
		return (errno != EAGAIN) && (errno != EWOULDBLOCK) && (errno != EINTR);
	}

	return nbytes == 0;
}

/* Completes what is in flight with the status a deleted SQ gets; returns how many */
static int
_queue_fail_inflight(struct xnvme_queue_upcie *upcie_queue)
{
	struct nvme_request_pool *rpool = upcie_queue->qpair.rpool;
	uint8_t inflight[NVME_REQUEST_POOL_LEN];
	int failed = 0;

	memset(inflight, 1, sizeof(inflight));
	for (size_t i = 0; i < rpool->top; ++i) {
		inflight[rpool->stack[i]] = 0;
	}

	for (uint16_t cid = 0; cid < NVME_REQUEST_POOL_LEN; ++cid) {
		struct xnvme_cmd_ctx *ctx;

		if (!inflight[cid]) {
			continue;
		}

		ctx = rpool->reqs[cid].user;
		nvme_request_free(rpool, cid);
		upcie_queue->base.outstanding -= 1;

		memset(&ctx->cpl, 0, sizeof(ctx->cpl));
		ctx->cpl.cid = cid;
		ctx->cpl.status.sc = 0x08; /* Command Aborted due to SQ Deletion */
		ctx->cpl.status.dnr = 1;

		ctx->async.cb(ctx, ctx->async.cb_arg);
		failed++;
	}

	return failed;
}

int
xnvme_be_upcie_cplane_queue_poke_idle(struct xnvme_queue_upcie *upcie_queue)
{
	struct xnvme_be_upcie_state *state = (void *)upcie_queue->base.dev->be.state;
	struct timespec ts;
	uint64_t now_ns;

	if (++upcie_queue->pokes_idle < XNVME_BE_UPCIE_POKES_IDLE_MAX) {
		return 0;
	}
	upcie_queue->pokes_idle = 0;

	if (!upcie_queue->served_gone_ns) {
		if (!_server_gone(g_upcie_rte.connection.sock)) {
			return 0;
		}

		clock_gettime(CLOCK_MONOTONIC, &ts);
		upcie_queue->served_gone_ns =
			(uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
		return 0;
	}

	clock_gettime(CLOCK_MONOTONIC, &ts);
	now_ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
	if ((now_ns - upcie_queue->served_gone_ns) <
	    ((uint64_t)state->ctrlr->ctrl->base.timeout_ms * 1000000ULL)) {
		return 0;
	}

	return _queue_fail_inflight(upcie_queue);
}

void
xnvme_be_upcie_cplane_disconnect(void)
{
	if (!g_upcie_rte.connection.alive) {
		return;
	}

	if (g_upcie_rte.mem.dmem_alive) {
		dmamem_destroy(&g_upcie_rte.mem.dmem);
		g_upcie_rte.mem.dmem_alive = 0;
	}
	if (g_upcie_rte.connection.heap_base) {
		munmap(g_upcie_rte.connection.heap_base, g_upcie_rte.connection.heap_nbytes);
	}
	if (g_upcie_rte.connection.sock > 0) {
		close(g_upcie_rte.connection.sock);
	}

	memset(&g_upcie_rte.connection, 0, sizeof(g_upcie_rte.connection));
}
#endif
