// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_CPLANE_CLIENT_H
#define __INTERNAL_XNVME_BE_UPCIE_CPLANE_CLIENT_H
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_be_upcie.h>

/**
 * Write the socket path clients of `cplane_id` connect to into `path`
 */
void
xnvme_be_upcie_cplane_socket_path(uint32_t cplane_id, char *path, size_t nbytes);

/**
 * Connect to the server at `path`
 *
 * @return A connected socket the caller closes, -ENOENT when nobody serves
 * `path`, or another negative errno
 */
int
xnvme_be_upcie_cplane_connect(const char *path);

/**
 * Check that the process at the other end of `sock` runs as this user
 *
 * The socket is the whole boundary: a server submits what a client sends, and a
 * client maps what a server hands over, so either side refuses a peer of
 * another user.
 *
 * @param sock A connected socket
 * @param root_ok Non-zero to accept a peer running as root as well
 *
 * @return 0 when the peer may be trusted, -EPERM when it runs as another user,
 * or another negative errno
 */
int
xnvme_be_upcie_cplane_peer_check(int sock, int root_ok);

/**
 * Send `msg` on `sock` and replace it with the reply
 *
 * @param fds Pre-allocated array of NVME_CPLANE_FDS_MAX, or NULL
 * @param nfds Set to how many descriptors arrived
 *
 * @return 0 on success, the server's result when it refused, negative errno on error
 */
int
xnvme_be_upcie_cplane_ask(int sock, struct nvme_cplane_msg *msg, int *fds, uint32_t *nfds);

/**
 * Like xnvme_be_upcie_cplane_ask(), about `ctrlr`, on the runtime's socket
 *
 * Serialised, since the threads of a process share the one socket.
 */
int
xnvme_be_upcie_cplane_ask_ctrlr(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_cplane_msg *msg,
				int *fds, uint32_t *nfds);

/**
 * Ask the server on `sock` about the controller at `index`; the reply lands in `msg`
 *
 * @return 0 on success, the server's result when it refused (-ERANGE past the
 * last controller), negative errno on error
 */
int
xnvme_be_upcie_cplane_ask_status(int sock, uint32_t index, struct nvme_cplane_msg *msg);

/**
 * Connect to the runtime of `cplane_id` and map the heap and the BAR0 of `bdf` into `ctrlr`
 *
 * @param bdf The controller wanted, or NULL to map the heap alone
 * @param ctrlr Filled when `bdf` is given
 *
 * @return 0 on success, -ENOENT when nobody serves `cplane_id`, -ENODEV when the
 * server does not hold `bdf`, another negative errno on error
 */
int
xnvme_be_upcie_cplane_init_connection(uint32_t cplane_id, const char *bdf,
				      struct xnvme_be_upcie_ctrlr *ctrlr);

/**
 * Build `ctrlr->ctrl` from the base and BAR0 mapping xnvme_be_upcie_cplane_init_connection() set
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_ctrlr_from_base(struct xnvme_be_upcie_ctrlr *ctrlr);

/**
 * Unmap the heap and close the runtime's socket, which releases what this process held
 */
void
xnvme_be_upcie_cplane_disconnect(void);

int
xnvme_be_upcie_cplane_ask_ctrlr_fd(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_cplane_msg *msg,
				   int fd);

int
xnvme_be_upcie_cplane_register_client_mem(struct xnvme_be_upcie_ctrlr *ctrlr, int dmabuf_fd,
					  uint64_t nbytes, uint32_t page_size,
					  const struct hostmem_shared_desc **desc_out,
					  uint64_t *offset_out);

int
xnvme_be_upcie_cplane_unregister_client_mem(struct xnvme_be_upcie_ctrlr *ctrlr, uint64_t offset);

int
xnvme_be_upcie_cplane_alloc_qpair_at(struct xnvme_be_upcie_ctrlr *ctrlr, uint64_t desc_offset,
				     uint64_t sq_offset, uint64_t cq_offset, uint16_t depth,
				     uint32_t *qid);

int
xnvme_be_upcie_cplane_free_qpair_at(struct xnvme_be_upcie_ctrlr *ctrlr, uint32_t qid);

/**
 * Have the server create an I/O queue pair of `depth` entries on `ctrlr`, and map it into `qpair`
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_alloc_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_qpair *qpair,
				  uint16_t depth);

/**
 * Hand back a queue pair from xnvme_be_upcie_cplane_alloc_qpair()
 */
void
xnvme_be_upcie_cplane_free_qpair(struct xnvme_be_upcie_ctrlr *ctrlr, struct nvme_qpair *qpair);

/**
 * The xnvme_be_upcie_sync_cmd_admin() of a client: the server submits the command
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_sync_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes);

/**
 * Allocate `nbytes` of the server's heap through the socket of `ctrlr`
 *
 * @return The buffer on success, NULL with errno set on error
 */
void *
xnvme_be_upcie_cplane_buf_alloc(struct xnvme_be_upcie_ctrlr *ctrlr, size_t nbytes, uint64_t *phys);

/**
 * Free a buffer from xnvme_be_upcie_cplane_buf_alloc()
 */
void
xnvme_be_upcie_cplane_buf_free(struct xnvme_be_upcie_ctrlr *ctrlr, void *buf);

/**
 * What a client's poke that reaped nothing does
 *
 * A server shutting down deletes its clients' queues without completing what is in flight
 * on them. Once the socket says the server is gone and the command timeout has passed,
 * this completes those commands as aborted, so a caller draining the queue returns.
 *
 * @return How many commands it completed
 */
int
xnvme_be_upcie_cplane_queue_poke_idle(struct xnvme_queue_upcie *upcie_queue);

/**
 * The I/O queue for synchronous commands on `ctrlr`; a client asks for it on first use
 *
 * @return The queue on success, NULL with errno set on error
 */
struct nvme_qpair *
xnvme_be_upcie_ctrlr_ioq(struct xnvme_be_upcie_ctrlr *ctrlr);

/**
 * A client's PRP scratch for admin payloads on `ctrlr`, allocated on first use
 *
 * @return The request carrying the scratch, NULL with errno set on error
 */
struct nvme_request *
xnvme_be_upcie_ctrlr_admin_prp(struct xnvme_be_upcie_ctrlr *ctrlr);

/**
 * Free the scratch of xnvme_be_upcie_ctrlr_admin_prp(), when there is one
 */
void
xnvme_be_upcie_ctrlr_admin_prp_release(struct xnvme_be_upcie_ctrlr *ctrlr);

#endif /* XNVME_BE_UPCIE_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_CPLANE_CLIENT_H */
