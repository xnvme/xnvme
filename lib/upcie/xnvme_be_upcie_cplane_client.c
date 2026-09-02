// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Connecting to a runtime another process owns
 *
 * The counterpart to serving. Where the server allocated memory and opened a
 * controller, this receives descriptors for both and builds a view of them: a
 * mapping of the same memory, a translation table for this process's
 * addresses, and a mapping of the BAR so that queues allocated later can be rung
 * from here.
 *
 * Nothing is allocated and nothing is owned. What this process wants from the
 * heap it asks for, and what it is allocated it hands back.
 */
#include <errno.h>

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <xnvme_be_upcie.h>

/**
 * Serialises a request against its reply on the shared socket
 *
 * A file-static rather than a member of the connection, so that letting go of
 * a runtime can zero the connection without destroying a lock somebody may be
 * about to take, and so that establishing the connection needs no lock of its
 * own to protect the lock.
 */
static pthread_mutex_t cplane_sock_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * Bytes of PRP scratch per request, as the server allocated it
 *
 * nvme_request_pool_init_prps_dmamem() carves the scratch in 4096-byte pages,
 * so a client walking the same allocation has to step by the same amount.
 */
#define XNVME_BE_UPCIE_PRP_NBYTES 4096

/**
 * Where clients of a given cplane_id look for the server
 *
 * One socket for the runtime, whatever it holds, because a request names the
 * controller it is about. The alternative is a socket per controller, and then
 * the file name is the addressing: a client has to know what a server holds
 * before it can ask what a server holds, and a controller with an awkward
 * identifier has to be spelled the same way at both ends.
 *
 * @param cplane_id The identifier the runtime is served under
 */
void
xnvme_be_upcie_cplane_socket_path(uint32_t cplane_id, char *path, size_t nbytes)
{
	snprintf(path, nbytes, "/tmp/xnvme-homi-%u.sock", cplane_id);
}

/**
 * Connect to a server's socket
 *
 * Nobody serving is reported as -ENOENT whichever way the kernel says it, so a
 * caller can tell "there is no server" from "something went wrong". That is not
 * an error to every caller: some decide to become the server instead.
 *
 * @return A connected socket, or negative errno
 */
int
xnvme_be_upcie_cplane_connect(const char *path)
{
	struct sockaddr_un addr = {.sun_family = AF_UNIX};
	int sock, err;

	if (!path) {
		return -EINVAL;
	}

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		return -errno;
	}
	snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr))) {
		err = -errno;
		close(sock);

		return ((err == -ENOENT) || (err == -ECONNREFUSED)) ? -ENOENT : err;
	}

	return sock;
}

/**
 * Ask the server for something and wait for the answer
 *
 * @param msg The request, replaced by the reply
 * @param fds Pre-allocated array of NVME_CPLANE_FDS_MAX, or NULL
 * @param nfds Set to how many descriptors arrived
 *
 * @return 0 on success, the server's status when it refused, negative errno on error
 */
int
xnvme_be_upcie_cplane_ask(int sock, struct nvme_cplane_msg *msg, int *fds, uint32_t *nfds)
{
	if (sock < 0) {
		return -ENOTCONN;
	}

	return nvme_cplane_request(sock, msg, fds, nfds);
}

/**
 * Ask about this controller, one asker at a time
 *
 * The controller is named in the message, so every controller this process
 * uses is asked about over the one socket. A request and its reply are two
 * calls on that stream, and threads working separate queues are not expected
 * to synchronise with each other, so they meet here; without it one could take
 * the answer meant for another.
 *
 * @param ctrlr A connected controller
 * @param msg The message to send, filled with the reply on return
 * @param fds Where to put descriptors that arrive, or NULL to expect none
 * @param nfds Set to how many arrived
 *
 * @return 0 on success, the server's status when it refused, negative errno
 */
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

/**
 * Which of the server's controllers is the one wanted
 *
 * Asked rather than derived. The server publishes the identifier it serves
 * each controller under, so a client matches against that instead of spelling
 * the same string into a file name and hoping the two agree.
 *
 * @return The index, or -ENODEV when this server does not hold it
 */
static int
cplane_index_of(int sock, const char *bdf)
{
	struct nvme_cplane_msg msg = {0};
	uint32_t ndevs;
	char wanted[16] = {0};

	/* Truncated the way the server truncates when it publishes, so a long
	 * identifier does not read as a mismatch. */
	snprintf(wanted, sizeof(wanted), "%s", bdf);

	if (xnvme_be_upcie_cplane_ask_status(sock, &msg)) {
		return -ENOTCONN;
	}

	ndevs = msg.u.status.ndevs;
	for (uint32_t i = 0; i < ndevs; ++i) {
		struct nvme_cplane_msg per = {0};

		per.index = i;
		if (i && xnvme_be_upcie_cplane_ask_status(sock, &per)) {
			continue;
		}
		if (!strcmp(i ? per.u.status.bdf : msg.u.status.bdf, wanted)) {
			return (int)i;
		}
	}

	return -ENODEV;
}

/**
 * The process's connection to a runtime, opened on first need
 *
 * One socket for however many controllers this process goes on to use, since
 * every request names the controller it is about. Callers hold
 * cplane_sock_lock.
 *
 * @return The socket, or negative errno
 */
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

/**
 * Ask about this controller, handing over a descriptor with the request
 *
 * The registration operations name a region by descriptor rather than by
 * address, since an address in this process means nothing in the server's.
 *
 * @return 0 on success, the server's status when it refused, negative errno
 */
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

/**
 * Have the server describe memory this process allocated
 *
 * What comes back is an offset into the heap, where the server left a
 * description of the region in terms the controller can consume. The caller
 * builds its dmamem from that rather than from anything it could work out
 * locally, since the addresses are the server's to know.
 *
 * @param ctrlr A connected controller
 * @param dmabuf_fd The region, as a dma-buf this process exported
 * @param nbytes How much of it
 * @param page_size The granule this process's runtime hands out
 * @param desc_out Set to the description, within this process's heap mapping
 * @param offset_out Set to the offset the registration is handed back by, or NULL
 *
 * @return 0 on success, negative errno on error
 */
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
	msg.u.reg.nbytes = nbytes;
	msg.u.reg.page_size = page_size;

	err = xnvme_be_upcie_cplane_ask_ctrlr_fd(ctrlr, &msg, dmabuf_fd);
	if (err) {
		XNVME_DEBUG("FAILED: registering %" PRIu64 " bytes; err(%d)", nbytes, err);
		return err;
	}

	*desc_out = (const struct hostmem_shared_desc *)((char *)g_upcie_rte.connection.heap_base +
							 msg.u.reg.desc_offset);
	if (offset_out) {
		*offset_out = msg.u.reg.desc_offset;
	}

	return 0;
}

/**
 * Give a registration back, naming it by the offset it was answered with
 */
int
xnvme_be_upcie_cplane_unregister_client_mem(struct xnvme_be_upcie_ctrlr *ctrlr, uint64_t offset)
{
	struct nvme_cplane_msg msg = {0};

	if (!ctrlr || !offset) {
		return -EINVAL;
	}

	msg.op = NVME_CPLANE_OP_UNREGISTER_MEM;
	msg.u.reg.desc_offset = offset;

	return xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL);
}

/**
 * Build this process's view of a controller somebody else owns
 *
 * @param cplane_id Identifies the runtime, and with it the socket
 * @param bdf The controller wanted, or NULL to take the heap and nothing else
 *
 * @return 0 on success, -ENOENT when nobody is serving, -ENODEV when nobody is
 *         serving that controller, negative errno on error
 */
int
xnvme_be_upcie_cplane_init_connection(uint32_t cplane_id, const char *bdf,
				      struct xnvme_be_upcie_ctrlr *ctrlr)
{
	struct nvme_cplane_msg msg = {0};
	const struct hostmem_shared_desc *desc;
	const struct nvme_runtime_record *record;
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
		/* Whatever arrived is installed in this process already, so it is
		 * this process's to close, even when the reply was not the one
		 * asked for. */
		for (uint32_t i = 0; i < nfds; ++i) {
			close(fds[i]);
		}
		err = err ? err : -EPROTO;
		goto failed;
	}

	/* One heap per runtime, and the reply carries it every time, so the
	 * first controller maps it and the rest take the mapping this process
	 * already has. */
	if (g_upcie_rte.connection.alive) {
		heap_base = g_upcie_rte.connection.heap_base;
		close(fds[0]);
	} else {
		heap_base = mmap(NULL, msg.u.init.heap_nbytes, PROT_READ | PROT_WRITE, MAP_SHARED,
				 fds[0], 0);
		close(fds[0]);
		if (heap_base == MAP_FAILED) {
			XNVME_DEBUG("FAILED: mmap(heap); errno(%d)", errno);
			err = -errno;
			close(fds[1]);
			goto failed;
		}
	}

	bar0 = mmap(NULL, msg.u.init.bar0_nbytes, PROT_READ | PROT_WRITE, MAP_SHARED, fds[1], 0);
	close(fds[1]);
	if (bar0 == MAP_FAILED) {
		XNVME_DEBUG("FAILED: mmap(BAR0); errno(%d)", errno);
		err = -errno;
		goto failed;
	}

	record =
		(const struct nvme_runtime_record *)((char *)heap_base + msg.u.init.record_offset);
	if (record->version != NVME_RUNTIME_RECORD_VERSION) {
		XNVME_DEBUG("FAILED: record version(%u), expected(%u)", record->version,
			    NVME_RUNTIME_RECORD_VERSION);
		err = -EPROTO;
		goto failed;
	}

	/* The server read the physical addresses once, when it allocated; this
	 * process could not, so it takes them from where they were left. */
	desc = (const struct hostmem_shared_desc *)((char *)heap_base + record->desc_offset);

	if (!g_upcie_rte.connection.alive) {
		err = dmamem_from_shared_hostmem(&g_upcie_rte.mem.dmem, heap_base, desc,
						 xnvme_be_upcie_va_bits());
		if (err) {
			XNVME_DEBUG("FAILED: dmamem_from_shared_hostmem(); err(%d)", err);
			goto failed;
		}
		g_upcie_rte.mem.dmem_alive = 1;

		g_upcie_rte.connection.heap_base = heap_base;
		g_upcie_rte.connection.heap_nbytes = msg.u.init.heap_nbytes;
		g_upcie_rte.connection.alive = 1;
	}

	/* Without a controller to hand this to, the caller only wanted to know
	 * whether anybody is serving, and to have the heap. The socket stays,
	 * since the runtime is now this process's to speak to. */
	if (!ctrlr) {
		munmap(bar0, msg.u.init.bar0_nbytes);
		pthread_mutex_unlock(&cplane_sock_lock);

		return 0;
	}

	ctrlr->index = index;
	ctrlr->bar0 = bar0;
	ctrlr->bar0_nbytes = msg.u.init.bar0_nbytes;
	ctrlr->record = record;

	pthread_mutex_unlock(&cplane_sock_lock);

	return 0;

failed:
	if (bar0 != MAP_FAILED) {
		munmap(bar0, msg.u.init.bar0_nbytes);
	}
	if ((heap_base != MAP_FAILED) && !g_upcie_rte.connection.alive) {
		munmap(heap_base, msg.u.init.heap_nbytes);
	}

	/* Only where this call opened it: once the runtime is up, the socket is
	 * the runtime's and _rte_term() is what closes it. */
	if (!g_upcie_rte.connection.alive) {
		close(g_upcie_rte.connection.sock);
		g_upcie_rte.connection.sock = 0;
	}

	pthread_mutex_unlock(&cplane_sock_lock);

	return err;
}

/**
 * Ask whoever is serving an identifier what they are holding
 *
 * Connects, asks, and disconnects, so it disturbs nothing and holds nothing.
 * The connection itself carries most of the answer: if it succeeds, somebody
 * is serving that identifier.
 *
 * @param cplane_id Identifies the runtime, and with it the socket
 * @param msg Pre-allocated message; its status member is filled on success
 *
 * @return 0 on success, -ENOENT when nobody is serving, negative errno on error
 */
int
xnvme_be_upcie_cplane_query(uint32_t cplane_id, struct nvme_cplane_msg *msg)
{
	char path[256] = {0};

	xnvme_be_upcie_cplane_socket_path(cplane_id, path, sizeof(path));

	return xnvme_be_upcie_cplane_query_path(path, msg);
}

int
xnvme_be_upcie_cplane_ask_status(int sock, struct nvme_cplane_msg *msg)
{
	if ((sock < 0) || !msg) {
		return -EINVAL;
	}

	msg->op = NVME_CPLANE_OP_STATUS;

	return nvme_cplane_request(sock, msg, NULL, NULL);
}

int
xnvme_be_upcie_cplane_query_path(const char *path, struct nvme_cplane_msg *msg)
{
	int sock, err;

	if (!path || !msg) {
		return -EINVAL;
	}

	sock = xnvme_be_upcie_cplane_connect(path);
	if (sock < 0) {
		return sock;
	}

	err = xnvme_be_upcie_cplane_ask_status(sock, msg);
	close(sock);

	return err;
}

/**
 * Build a controller from what the server published
 *
 * Nothing here touches the device: it is already open, in another process, and
 * cannot be opened a second time. What this makes is a local structure
 * describing it, with this process's own mapping of BAR0 so that an allocated
 * queue can be rung from here.
 *
 * @param ctrlr Pre-allocated controller to fill
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_ctrlr_from_record(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	const struct nvme_runtime_record *record;
	struct nvme_controller *ctrl;

	if (!ctrlr || !ctrlr->ctrl || !ctrlr->record) {
		return -ENOTCONN;
	}

	ctrl = ctrlr->ctrl;
	record = ctrlr->record;

	memset(ctrl, 0, sizeof(*ctrl));
	ctrl->timeout_ms = (int)record->timeout_ms;
	ctrl->cc = record->cc;
	ctrl->func.bars[0].region = ctrlr->bar0;
	ctrl->func.bars[0].size = ctrlr->bar0_nbytes;
	ctrl->func.bars[0].fd = -1; ///< The server holds it
	snprintf(ctrl->func.bdf, sizeof(ctrl->func.bdf), "%s", record->bdf);

	/* The admin queue stays with the server, so this leaves it empty rather
	 * than pointing at something no process here may drive. */

	return 0;
}

/**
 * Ask for a queue built on memory this process registered
 *
 * What comes back is only an identifier: the memory is already this process's,
 * and the doorbells follow from the identifier, so there is nothing else for
 * the server to say.
 *
 * @param ctrlr A connected controller
 * @param desc_offset The registration the queue lives in
 * @param sq_offset Submission queue, from the registered region's base
 * @param cq_offset Completion queue, from the registered region's base
 * @param depth Entries in each
 * @param qid Set to the identifier allocated
 *
 * @return 0 on success, negative errno on failure
 */
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
	msg.u.queue_at.desc_offset = desc_offset;
	msg.u.queue_at.sq_offset = sq_offset;
	msg.u.queue_at.cq_offset = cq_offset;
	msg.u.queue_at.depth = depth;

	err = xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: asking for a queue on registered memory; err(%d)", err);
		return err;
	}

	*qid = msg.u.queue_at.qid;

	return 0;
}

/**
 * Hand back a queue built on this process's own memory
 *
 * @param ctrlr A connected controller
 * @param qid The identifier handed out earlier
 *
 * @return 0 on success, negative errno on failure
 */
int
xnvme_be_upcie_cplane_free_qpair_at(struct xnvme_be_upcie_ctrlr *ctrlr, uint32_t qid)
{
	struct nvme_cplane_msg msg = {0};

	if (!ctrlr) {
		return -EINVAL;
	}

	msg.op = NVME_CPLANE_OP_FREE_IOQPAIR;
	msg.u.release.qid = qid;

	return xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL);
}

int
xnvme_be_upcie_cplane_alloc_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_qpair *qpair,
				  uint16_t depth)
{
	struct nvme_cplane_msg msg = {0};
	char *base = g_upcie_rte.connection.heap_base;
	int dstrd, err;

	if (!qpair || !ctrlr || !ctrlr->bar0) {
		return -ENOTCONN;
	}

	msg.op = NVME_CPLANE_OP_ALLOC_IOQPAIR;
	msg.u.queue.depth = depth;

	err = xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: asking for a queue; err(%d)", err);
		return err;
	}

	dstrd = nvme_reg_cap_get_dstrd(nvme_mmio_cap_read(ctrlr->bar0));

	memset(qpair, 0, sizeof(*qpair));
	qpair->qid = msg.u.queue.allocation.qid;
	qpair->depth = msg.u.queue.allocation.depth;
	qpair->sq = base + msg.u.queue.allocation.sq_offset;
	qpair->cq = base + msg.u.queue.allocation.cq_offset;
	qpair->sqdb = (char *)ctrlr->bar0 + XNVME_BE_UPCIE_DOORBELL_OFFSET +
		      ((2 * qpair->qid) << (2 + dstrd));
	qpair->cqdb = (char *)ctrlr->bar0 + XNVME_BE_UPCIE_DOORBELL_OFFSET +
		      ((2 * qpair->qid + 1) << (2 + dstrd));
	qpair->tail_last_written = UINT16_MAX;
	qpair->phase = 1;

	qpair->rpool = calloc(1, sizeof(*qpair->rpool));
	if (!qpair->rpool) {
		return -errno;
	}
	nvme_request_pool_init(qpair->rpool);

	/* Strided by the page size the server allocated with, not by this
	 * process's runtime configuration, which a connected process never
	 * fills in: a stride of zero aims every PRP list at the same page. */
	for (uint16_t i = 0; i < NVME_REQUEST_POOL_LEN; ++i) {
		void *prp = base + msg.u.queue.allocation.prp_offset +
			    ((size_t)i * XNVME_BE_UPCIE_PRP_NBYTES);

		qpair->rpool->reqs[i].prp = prp;
		qpair->rpool->reqs[i].prp_addr = dmamem_va_to_iova(&g_upcie_rte.mem.dmem, prp);
	}
	qpair->rpool->prps = base + msg.u.queue.allocation.prp_offset;

	return 0;
}

/**
 * The controller's I/O queue for this process, asked for if it has none yet
 *
 * Deferred rather than taken at open: an I/O queue is dedicated to the
 * client holding it, so a process that only asks the controller about itself
 * should not cost the server one. Where this process owns the controller the
 * queue was created with it, and this hands that one back.
 */
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
		/* The server builds its queue when it opens the controller, so
		 * one missing here is a controller that never came up. */
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

/**
 * The controller's PRP scratch for admin payloads, allocated on first need
 *
 * There is one admin queue and it is shared, so what an admin command needs in
 * order to describe a payload is a single page, held for as long as the
 * controller is. An I/O queue is dedicated to the client holding it, so taking
 * one for this made a process pay for a queue it may never submit I/O on.
 *
 * Only payloads spanning more than two pages reach the list itself; smaller
 * ones are described by the command's own PRP fields and never come here.
 *
 * @return A request whose PRP list is this controller's, NULL with errno set
 * where the page could not be allocated.
 */
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
		xnvme_be_upcie_buf_alloc_on(ctrlr, 4096, &ctrlr->admin_prp.prp_addr);
	if (!ctrlr->admin_prp.prp) {
		XNVME_DEBUG("FAILED: allocating admin PRP scratch; errno(%d)", errno);
		return NULL;
	}

	return &ctrlr->admin_prp;
}

/**
 * Give back the admin PRP scratch, where one was ever taken
 */
void
xnvme_be_upcie_ctrlr_admin_prp_release(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	if (!ctrlr || !ctrlr->admin_prp.prp) {
		return;
	}

	xnvme_be_upcie_buf_free_on(ctrlr, ctrlr->admin_prp.prp);
	memset(&ctrlr->admin_prp, 0, sizeof(ctrlr->admin_prp));
}

/**
 * Hand an allocated queue back and release what was built around it
 *
 * @param qpair A queue pair from xnvme_be_upcie_cplane_alloc_qpair()
 */
void
xnvme_be_upcie_cplane_free_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_qpair *qpair)
{
	struct nvme_cplane_msg msg = {0};

	if (!ctrlr || !qpair || !qpair->qid) {
		return;
	}

	msg.op = NVME_CPLANE_OP_FREE_IOQPAIR;
	msg.u.release.qid = qpair->qid;

	if (xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL)) {
		XNVME_DEBUG("FAILED: handing back qid(%u)", qpair->qid);
	}

	free(qpair->rpool);
	memset(qpair, 0, sizeof(*qpair));
}

/**
 * Let go of a runtime this process connected to
 *
 * The socket is the process's rather than any one controller's, so it goes
 * with the runtime and every controller has closed by the time this runs.
 * Closing it is what tells the server to reclaim whatever is still held, so it
 * goes last and it always goes.
 */
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
