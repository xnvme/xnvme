// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Handing a controller to another process, and what passes between them
 * =====================================================================
 *
 * The server of a controller serves clients over a unix socket. What crosses
 * at attach are descriptors, since nothing else moves one between unrelated
 * processes: the vfio device, the iommufd where there is one, and the heap.
 * Everything after that is a fixed-size message, and none of it is on the I/O
 * path.
 *
 * Asking what an server is holding is on the same channel, since connecting at
 * all is most of the answer: somebody is serving that identifier.
 *
 * A client allocates nothing from the shared heap either, for the same
 * reason it does not touch the admin queue: the allocator is the server's and
 * its free list has no lock. It asks, and is handed an offset. Allocation
 * belongs on the control plane in any case, being done once for memory used
 * many times.
 *
 * A client submits I/O itself, on queues it was allocated, ringing its own
 * doorbell. What it does not do is touch the admin queue, because there is one
 * of those and its head and tail are held by value; the server submits admin
 * commands on request instead. That costs nothing that matters, since admin is
 * not hot, and it is what lets the runtime record be written once and read
 * without a lock.
 *
 * Admin commands carry payloads, and the payload does not travel: the client
 * names an address the device can already reach, from memory it registered in
 * the shared address space or was allocated, and the server puts that in the
 * command. So an identify lands in the client's buffer without a copy, and
 * without the server having a mapping of it.
 *
 * Which admin commands an server submits is the server's business, and the
 * default is to submit what it is asked. The hook is there so that an server
 * which one day needs to refuse something has somewhere to do it.
 *
 * @file nvme_cplane.h
 * @version 0.9.0
 */

#ifndef __UPCIE_NVME_CPLANE_H
#define __UPCIE_NVME_CPLANE_H

/**
 * Bumped when the message layout changes, or when anything it describes does
 *
 * Starts at four, not one. The layouts it skips exist in pre-release builds
 * of this branch: one had no `index`, so the payload sat eight bytes earlier
 * and the whole message was shorter, two lacked the registration operations,
 * and three could not build a queue on a client's own memory. The version field is at the same
 * offset in all of them, so a server built from one would read a version it recognises and then
 * act on a payload that has moved. Skipping the numbers is what makes it refuse instead, and it
 * costs nothing to do.
 */
#define NVME_CPLANE_VERSION 4U

enum nvme_cplane_op {
	/**
	 * Client asks for a controller; the descriptors follow
	 *
	 * Sent once per controller a connection intends to use, naming it in
	 * `index`. The heap is the runtime's and comes back every time, so a
	 * client that has already mapped it closes the descriptor and keeps
	 * what it has; BAR0 is the controller's and differs each time.
	 *
	 * A server refuses a request for a queue or for memory on a connection
	 * that has not sent it, since the offsets in the reply mean nothing to
	 * a client with no mapping of the heap. Status needs no initialisation
	 * and an admin command names memory the client already has, so neither
	 * is gated.
	 */
	NVME_CPLANE_OP_INIT_CONNECTION = 1,
	NVME_CPLANE_OP_ALLOC_IOQPAIR = 2, ///< Client asks for a queue of a given depth
	NVME_CPLANE_OP_FREE_IOQPAIR = 3,  ///< Client hands a queue back
	NVME_CPLANE_OP_ADMIN_CMD = 4,     ///< Client asks for an admin command to be submitted
	NVME_CPLANE_OP_ALLOC_BUF = 5,     ///< Client asks for DMA memory it cannot allocate
	NVME_CPLANE_OP_FREE_BUF = 6,      ///< Client hands that memory back
	NVME_CPLANE_OP_STATUS = 7,        ///< Anybody asks what the server is holding

	/**
	 * Client asks for memory of its own to be made reachable
	 *
	 * The heap a server hands out is host memory it owns. A client with
	 * device memory has neither: it allocated the memory itself, and the
	 * addresses it resolves through are its own, where the controller
	 * consumes the server's. So it sends a dma-buf naming the region and
	 * the server describes it in terms the controller can use.
	 *
	 * The reply is an offset rather than an address, because the answer is
	 * a description and in the absence of an IOMMU that description is a
	 * table with an entry per granule. It goes in the heap, which the
	 * client has mapped, and the offset names it.
	 */
	NVME_CPLANE_OP_REGISTER_MEM = 8,
	NVME_CPLANE_OP_UNREGISTER_MEM = 9, ///< Client hands a registration back

	/**
	 * Client asks for a queue built on memory it registered
	 *
	 * A queue the server allocates lives in the server's heap, which is
	 * host memory. That is what a client submitting from the host wants,
	 * and it is no use to a GPU issuing I/O itself: the kernel writing the
	 * submission entries has to reach them, so the queue has to live where
	 * the kernel runs.
	 *
	 * The client allocates the queue in its own memory and names it by
	 * offset into a region it registered, so it can only ever name memory
	 * of its own. What stays with the server is what only the server can
	 * do: the identifier space, and the admin queue the Create I/O
	 * Submission and Completion commands go on.
	 */
	NVME_CPLANE_OP_ALLOC_IOQPAIR_AT = 10,
};

/**
 * One message in either direction, request and reply sharing a layout
 *
 * Fixed size, so there is no framing to get wrong, and small enough that a
 * short read means the peer is gone rather than that more is coming.
 */
struct nvme_cplane_msg {
	uint32_t op;      ///< One of enum nvme_cplane_op
	uint32_t version; ///< NVME_CPLANE_VERSION as the sender knows it
	int32_t status;   ///< Replies only: 0, or a negative errno
	uint32_t nfds;    ///< Replies only: descriptors accompanying this message

	/**
	 * Which of the server's controllers this is about, from zero
	 *
	 * Addressing rather than payload, so it sits here and not in the
	 * union: every operation that reaches a controller needs it, and a
	 * client learns the range by asking for status at index zero.
	 *
	 * A server serves one socket for everything it holds. Naming the
	 * controller in the message is what makes that possible; without it
	 * the only place left to say which one is the socket's name, and then
	 * a client must know the set before it can ask what the set is.
	 */
	uint32_t index;
	uint32_t _rsvd;

	union {
		struct {
			uint64_t record_offset; ///< Where the record sits in the heap
			uint64_t heap_nbytes;   ///< What the client should expect to map
			uint64_t bar0_nbytes;   ///< Size of the BAR0 mapping to make
		} init;
		struct {
			struct nvme_ioqpair allocation; ///< Reply: the queue allocated
			uint16_t depth;                 ///< Request: entries wanted
			uint16_t _rsvd[3];
		} queue;
		struct {
			uint32_t qid; ///< The queue being handed back
		} release;
		struct {
			uint64_t nbytes; ///< Request: how much is wanted
			uint64_t offset; ///< Reply, and the request when freeing
		} mem;
		struct {
			/**
			 * How many controllers this server holds
			 *
			 * The reply describes the one named by `index`, and
			 * this says how far that runs, so a client walks the
			 * range rather than being told where to look.
			 */
			uint32_t ndevs;

			/**
			 * Connections the server has, all controllers over
			 *
			 * One per client process, since a client reaches
			 * every controller over the one connection.
			 */
			uint32_t nclients;

			/**
			 * Of those, the ones that asked for this controller
			 *
			 * A client is counted here once it has been given this
			 * controller's descriptors, whether or not it went on
			 * to take a queue.
			 */
			uint32_t nclients_ctrlr;

			uint32_t nqueues; ///< How many queues they hold on this controller

			/**
			 * What the controller allocated, from Get Features
			 * (Number of Queues). Zero means it did not answer,
			 * not that it has none; only the server can ask, since
			 * asking is an admin command.
			 */
			uint32_t nsq_total;
			uint32_t ncq_total;

			char bdf[16]; ///< The controller being served
		} status;
		struct {
			struct nvme_command cmd;    ///< Request: what to submit
			struct nvme_completion cpl; ///< Reply: what came back
		} admin;
		struct {
			uint64_t nbytes;      ///< Request: how much the descriptor covers
			uint64_t desc_offset; ///< Reply, and the request when releasing

			/**
			 * Request: the granule the region is addressed in
			 *
			 * The client's, not the server's: it allocated the
			 * memory and knows what its runtime hands out, where
			 * the server has only a descriptor and no way to ask.
			 */
			uint32_t page_size;
			uint32_t _rsvd;
		} reg;
		struct {
			uint64_t desc_offset; ///< Request: which registration it lives in
			uint64_t sq_offset;   ///< Request: submission queue, from the region
			uint64_t cq_offset;   ///< Request: completion queue, from the region
			uint32_t qid;         ///< Reply: the identifier allocated
			uint16_t depth;       ///< Request: entries wanted
			uint16_t _rsvd;
		} queue_at;
	} u;
};

/* The wire format is fixed, so a change to its size is a change every peer has
 * to agree on. Catching that here is what makes the version discipline above
 * enforceable rather than remembered. */
UPCIE_STATIC_ASSERT(sizeof(struct nvme_cplane_msg) == 104,
		    "nvme_cplane_msg changed size; bump NVME_CPLANE_VERSION")

#define NVME_CPLANE_FDS_MAX 3 ///< device, iommufd, heap

/**
 * Send one message, optionally with descriptors attached
 *
 * @param sock A connected SOCK_STREAM unix socket
 * @param msg The message to send
 * @param fds Descriptors to attach, or NULL
 * @param nfds How many, at most NVME_CPLANE_FDS_MAX
 * @param flags Passed to sendmsg(2); MSG_DONTWAIT to avoid waiting
 *
 * @return 0 on success, -EAGAIN when MSG_DONTWAIT was given and the peer is
 *         not reading, negative errno on error
 */
static inline int
nvme_cplane_msg_send_flags(int sock, const struct nvme_cplane_msg *msg, const int *fds,
			   uint32_t nfds, int flags)
{
	char control[CMSG_SPACE(sizeof(int) * NVME_CPLANE_FDS_MAX)] = {0};
	struct iovec iov = {.iov_base = (void *)msg, .iov_len = sizeof(*msg)};
	struct msghdr hdr = {0};
	ssize_t nbytes;

	if ((sock < 0) || !msg || (nfds > NVME_CPLANE_FDS_MAX)) {
		return -EINVAL;
	}

	hdr.msg_iov = &iov;
	hdr.msg_iovlen = 1;

	if (nfds) {
		struct cmsghdr *cmsg;

		hdr.msg_control = control;
		hdr.msg_controllen = CMSG_SPACE(sizeof(int) * nfds);

		cmsg = CMSG_FIRSTHDR(&hdr);
		cmsg->cmsg_level = SOL_SOCKET;
		cmsg->cmsg_type = SCM_RIGHTS;
		cmsg->cmsg_len = CMSG_LEN(sizeof(int) * nfds);
		memcpy(CMSG_DATA(cmsg), fds, sizeof(int) * nfds);
	}

	nbytes = sendmsg(sock, &hdr, MSG_NOSIGNAL | flags);
	if (nbytes < 0) {
		return -errno;
	}
	if (nbytes != (ssize_t)sizeof(*msg)) {
		return -EPROTO; ///< A fixed-size message either fits or the peer is gone
	}

	return 0;
}

/**
 * Send one message, waiting for room where there is none
 */
static inline int
nvme_cplane_msg_send(int sock, const struct nvme_cplane_msg *msg, const int *fds, uint32_t nfds)
{
	return nvme_cplane_msg_send_flags(sock, msg, fds, nfds, 0);
}

/**
 * Send one message, or say that the peer is not reading
 *
 * For a server, where waiting is not an option: one thread answers every
 * connection, and a peer that stops reading its replies while carrying on
 * sending requests would otherwise stall that thread and with it everybody
 * else. The socket buffer is thousands of replies deep, so anything that
 * fills it is a client that has stopped reading, and the only useful thing to
 * do with such a client is to let it go.
 *
 * @return 0 on success, -EAGAIN when the peer is not reading, negative errno
 *         on error
 */
static inline int
nvme_cplane_msg_trysend(int sock, const struct nvme_cplane_msg *msg, const int *fds, uint32_t nfds)
{
	return nvme_cplane_msg_send_flags(sock, msg, fds, nfds, MSG_DONTWAIT);
}

/**
 * Take one helping of a message off the wire
 *
 * Shared by the blocking and the non-blocking receive, since the awkward part,
 * the descriptors that arrive with the first byte, has to be done exactly once
 * per message and is easy to get wrong twice.
 *
 * @param nread In/out: bytes of msg already filled
 * @param flags Passed to recvmsg(2); MSG_DONTWAIT to avoid waiting
 *
 * @return 0 when msg is complete, -EAGAIN when more is needed, -ENOTCONN when
 *         the peer is gone, negative errno on error
 */
static inline int
nvme_cplane_msg_recv_chunk(int sock, struct nvme_cplane_msg *msg, size_t *nread, int *fds,
			   uint32_t *nfds, int flags)
{
	char control[CMSG_SPACE(sizeof(int) * NVME_CPLANE_FDS_MAX)] = {0};
	struct iovec iov = {0};
	struct msghdr hdr = {0};
	struct cmsghdr *cmsg;
	ssize_t nbytes;

	if ((sock < 0) || !msg || !nread || (*nread >= sizeof(*msg))) {
		return -EINVAL;
	}

	iov.iov_base = (char *)msg + *nread;
	iov.iov_len = sizeof(*msg) - *nread;
	hdr.msg_iov = &iov;
	hdr.msg_iovlen = 1;

	/* Descriptors arrive with the first byte, so the ancillary buffer is
	 * offered only while nothing has been read yet. */
	if (!*nread) {
		hdr.msg_control = control;
		hdr.msg_controllen = sizeof(control);
		if (nfds) {
			*nfds = 0;
		}
	}

	nbytes = recvmsg(sock, &hdr, flags);
	if (nbytes < 0) {
		return -errno; ///< -EAGAIN when nothing has arrived yet
	}
	if (nbytes == 0) {
		return -ENOTCONN;
	}

	if (!*nread) {
		int got[NVME_CPLANE_FDS_MAX];
		uint32_t ngot = 0;

		cmsg = CMSG_FIRSTHDR(&hdr);
		if (cmsg && (cmsg->cmsg_type == SCM_RIGHTS)) {
			ngot = (uint32_t)((cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int));
			if (ngot > NVME_CPLANE_FDS_MAX) {
				ngot = NVME_CPLANE_FDS_MAX;
			}
			memcpy(got, CMSG_DATA(cmsg), sizeof(int) * ngot);
		}

		/* Whatever fit is installed in this process the moment recvmsg
		 * returns, whether or not it was asked for, so a caller wanting
		 * none still has to close them. A server reading requests passes
		 * none and would otherwise leak a descriptor per request a peer
		 * chose to attach.
		 *
		 * A truncated block is a short list that looks complete, so it
		 * is refused rather than acted on, once what did arrive has been
		 * closed. */
		if (!fds || !nfds || (hdr.msg_flags & MSG_CTRUNC)) {
			for (uint32_t i = 0; i < ngot; ++i) {
				close(got[i]);
			}
			if (hdr.msg_flags & MSG_CTRUNC) {
				UPCIE_DEBUG("FAILED: ancillary data truncated");
				return -EPROTO;
			}
		} else {
			*nfds = ngot;
			memcpy(fds, got, sizeof(int) * ngot);
		}
	}

	*nread += (size_t)nbytes;

	return (*nread == sizeof(*msg)) ? 0 : -EAGAIN;
}

/**
 * Receive one message, waiting for as much of it as it takes
 *
 * @param sock A connected SOCK_STREAM unix socket
 * @param msg Pre-allocated message to fill
 * @param fds Pre-allocated array of NVME_CPLANE_FDS_MAX, or NULL to refuse any
 * @param nfds Set to how many descriptors arrived
 *
 * @return 0 on success, -ENOTCONN when the peer is gone, negative errno on error
 */
static inline int
nvme_cplane_msg_recv(int sock, struct nvme_cplane_msg *msg, int *fds, uint32_t *nfds)
{
	size_t nread = 0;
	int err;

	if ((sock < 0) || !msg) {
		return -EINVAL;
	}
	if (nfds) {
		*nfds = 0;
	}

	do {
		err = nvme_cplane_msg_recv_chunk(sock, msg, &nread, fds, nfds, 0);
	} while ((err == -EAGAIN) || (err == -EINTR));

	return err;
}

/**
 * Receive what has arrived of a message, without waiting for the rest
 *
 * For a server that multiplexes connections on one thread: a peer that writes
 * half a message and pauses must not hold the reader, so the caller keeps
 * `nread` per connection and comes back when the socket is readable again.
 *
 * @param nread In/out: bytes filled so far; zero before the first call
 *
 * @return 0 when msg is complete, -EAGAIN when more is needed, -ENOTCONN when
 *         the peer is gone, negative errno on error
 */
static inline int
nvme_cplane_msg_recv_some(int sock, struct nvme_cplane_msg *msg, size_t *nread, int *fds,
			  uint32_t *nfds)
{
	return nvme_cplane_msg_recv_chunk(sock, msg, nread, fds, nfds, MSG_DONTWAIT);
}

/**
 * Ask a peer for something and wait for its answer
 *
 * @param sock A connected SOCK_STREAM unix socket
 * @param msg The request, replaced by the reply
 * @param fds Pre-allocated array of NVME_CPLANE_FDS_MAX, or NULL
 * @param nfds Set to how many descriptors arrived
 *
 * @return 0 on success, the peer's status when it refused, negative errno on error
 */
static inline int
nvme_cplane_request_with(int sock, struct nvme_cplane_msg *msg, const int *send_fds,
			 uint32_t send_nfds, int *fds, uint32_t *nfds)
{
	int err;

	msg->version = NVME_CPLANE_VERSION;
	msg->status = 0;

	err = nvme_cplane_msg_send(sock, msg, send_fds, send_nfds);
	if (err) {
		return err;
	}

	err = nvme_cplane_msg_recv(sock, msg, fds, nfds);
	if (err) {
		return err;
	}

	if (msg->version != NVME_CPLANE_VERSION) {
		UPCIE_DEBUG("FAILED: peer speaks version(%u), this is version(%u)", msg->version,
			    NVME_CPLANE_VERSION);
		return -EPROTO;
	}

	return msg->status;
}

/**
 * Ask a peer for something, sending nothing but the message
 */
static inline int
nvme_cplane_request(int sock, struct nvme_cplane_msg *msg, int *fds, uint32_t *nfds)
{
	return nvme_cplane_request_with(sock, msg, NULL, 0, fds, nfds);
}

/**
 * Who is on the other end of a connection
 *
 * An server deciding whether to serve somebody needs to know who they are, and
 * SO_PEERCRED is the answer the kernel vouches for: the peer cannot claim a
 * uid it does not have. What an server does with it is policy and belongs to
 * the server, which is why nothing here decides.
 *
 * @param sock A connected unix socket
 * @param cred Pre-allocated credentials to fill
 *
 * @return 0 on success, negative errno on error
 */
static inline int
nvme_cplane_peer_cred(int sock, struct ucred *cred)
{
	socklen_t nbytes = sizeof(*cred);

	if ((sock < 0) || !cred) {
		return -EINVAL;
	}

	if (getsockopt(sock, SOL_SOCKET, SO_PEERCRED, cred, &nbytes)) {
		return -errno;
	}

	return 0;
}

/**
 * Whether an admin command is one a client may ask for
 *
 * Accepts everything, and exists so that an server which needs to intervene has
 * one place to do it rather than having to invent one later. A list of
 * permitted opcodes was the first instinct and it would suggest a boundary
 * that is not there: a client holds the device fd, so it can reset the
 * controller without asking, and refusing it a Format here would protect
 * nothing while making the common case answer to a list nobody maintains.
 *
 * An server serving clients it does not trust has a larger problem than this
 * function.
 *
 * @param cmd The command a client asked to have submitted
 *
 * @return Non-zero when the command may be submitted on a client's behalf
 */
static inline int
nvme_cplane_admin_permitted(const struct nvme_command *cmd)
{
	(void)cmd;

	return 1;
}

#endif /* __UPCIE_NVME_CPLANE_H */
