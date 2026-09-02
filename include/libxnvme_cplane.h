// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Sharing a controller with other processes
 *
 * One process opens a controller and serves it; others connect and do I/O
 * against the same device. The server hands over descriptors rather than naming
 * objects in the filesystem, because a vfio device file cannot be bound twice,
 * so what a client needs cannot be found by name.
 *
 * @note uPCIe only, so -ENOSYS elsewhere.
 *
 * @note This API is experimental and may change without notice.
 *
 * @file libxnvme_cplane.h
 */

#ifndef __LIBXNVME_CPLANE_H
#define __LIBXNVME_CPLANE_H

#include <signal.h>
#include <stdint.h>

#define XNVME_CPLANE_MAX_CTRLRS 64

/**
 * What a runtime says about itself and what it holds
 *
 * A client reaches every controller the server holds over one connection, so
 * `nconnections` counts client processes. `nctrlrs_held` exceeds `nctrlrs`
 * when there were more than XNVME_CPLANE_MAX_CTRLRS to list.
 */
struct xnvme_cplane_info {
	uint32_t nconnections; ///< Connections the server has, the caller's included
	uint32_t nctrlrs;      ///< Controllers listed in `ctrlrs`
	uint32_t nctrlrs_held; ///< Controllers held
	char ctrlrs[XNVME_CPLANE_MAX_CTRLRS][XNVME_IDENT_URI_LEN]; ///< Their identifiers
};

/**
 * Report whether a process currently serves the runtime for `cplane_id`
 *
 * Connecting is most of the answer: a socket that answers has a process behind
 * it, since the address stops accepting when its holder dies.
 *
 * @param cplane_id Identifier the runtime was created with
 *
 * @return 1 when a process serves it, 0 when none does, negative errno on
 *         error, including -ENOSYS where sharing is unavailable.
 */
int
xnvme_cplane_server_alive(uint32_t cplane_id);

/**
 * Ask the server of `cplane_id` what it holds
 *
 * @param cplane_id Identifier the runtime was created with
 * @param info Filled on success
 *
 * @return On success, 0 is returned. On error, negative errno is returned:
 *         -ENOENT when nobody serves this id, -ENOSYS where sharing is
 *         unavailable.
 */
int
xnvme_cplane_get_info(uint32_t cplane_id, struct xnvme_cplane_info *info);

/**
 * What a runtime says about one controller it holds
 *
 * `nconnections` counts the clients given this controller's descriptors,
 * whether or not they took a queue; asking for this is not one of them. A queue
 * identifier covers a pair, so `nsq_used` and `ncq_used` are equal. The smaller
 * of `nsq_total` and `ncq_total` limits the queue pairs, and zero means the
 * controller did not answer, not that it has none.
 */
struct xnvme_cplane_ctrlr_info {
	uint32_t nconnections; ///< Clients of this controller
	uint32_t nsq_used;     ///< I/O submission queues in use
	uint32_t ncq_used;     ///< I/O completion queues in use
	uint32_t nsq_total;    ///< I/O submission queues the controller allocated
	uint32_t ncq_total;    ///< I/O completion queues the controller allocated
	uint32_t initialized;  ///< Whether the controller is up and being served
};

/**
 * Ask the server holding the controller at `uri` about it
 *
 * The servers are asked in turn until one holds the controller. Neither this
 * nor xnvme_cplane_get_info() becomes a client: the connection only asks.
 *
 * @param uri Device identifier, as passed to xnvme_dev_open()
 * @param info Filled on success
 *
 * @return On success, 0 is returned. On error, negative errno is returned:
 *         -ENOENT when nobody serves this controller, -ENOSYS where sharing
 *         is unavailable.
 */
int
xnvme_cplane_get_ctrlr_info(const char *uri, struct xnvme_cplane_ctrlr_info *info);

/**
 * Serve clients of the given devices until told to stop
 *
 * Holds one socket, named from `cplane_id`, and answers processes that
 * connect: hands over the descriptors and offsets they need to map the heap
 * and a controller's BAR0, creates queues on request, submits admin commands
 * on their behalf, and releases whatever a client held when it disconnects.
 *
 * One thread reads every connection and one per controller runs the requests
 * that reach the controller; they are created and joined here. An admin
 * command holds xnvme_lock() for as long as it runs, as it does outside the
 * server, so a Format delays the server's other queue and memory requests.
 *
 * Blocks until `*stop` becomes non-zero, which a signal handler is expected to
 * do. Returns when the last thing it was serving has been let go, so a caller
 * can close its devices afterwards.
 *
 * @param devs Devices this process has opened
 * @param ndevs How many
 * @param cplane_id Identifier to serve under; the sockets are named from it
 * @param stop Set to non-zero to bring the server down; typed for a signal
 * handler, since that is what usually sets it
 * @param ready Called once, from this thread, when the socket is bound and
 * clients can connect; NULL when the caller does not care. Anything a caller
 * announces before this call it is announcing on faith, since the bind can
 * still fail. Optional.
 * @param ready_arg Handed back to `ready`
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_cplane_serve(struct xnvme_dev **devs, int ndevs, uint32_t cplane_id,
		   volatile sig_atomic_t *stop, void (*ready)(void *), void *ready_arg);

#endif /* __LIBXNVME_CPLANE_H */
