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

/* XNVME_IDENT_URI_LEN comes from libxnvme_ident.h, which libxnvme.h includes
 * first; these headers carry no include guards and rely on that ordering. */

/**
 * Most controllers a single runtime records
 */
#define XNVME_CPLANE_MAX_CTRLRS 64

/**
 * What a runtime says about itself and what it holds
 */
struct xnvme_cplane_info {
	/**
	 * Connections the server has, the caller's own included
	 *
	 * One per client process, since a client reaches every controller the
	 * server holds over the one connection.
	 */
	uint32_t nconnections;

	uint32_t nctrlrs; ///< Controllers listed below

	/** Controllers held; exceeds `nctrlrs` when the list below truncated */
	uint32_t nctrlrs_held;

	/** Their identifiers, `nctrlrs` of them */
	char ctrlrs[XNVME_CPLANE_MAX_CTRLRS][XNVME_IDENT_URI_LEN];
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
 * Ask the runtime for `cplane_id` what it holds, without connecting to it
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
 */
struct xnvme_cplane_ctrlr_info {
	/**
	 * Clients that asked the server for this controller
	 *
	 * This controller rather than the runtime, and a client is counted
	 * once it has been given the descriptors, whether or not it went on to
	 * take a queue. The caller is not among them: asking initialises
	 * nothing.
	 */
	uint32_t nconnections;

	/** I/O queues in use; always equal, since a queue id covers the pair */
	uint32_t nsq_used;
	uint32_t ncq_used;

	/**
	 * I/O queues the controller allocated; the smaller limits queue pairs.
	 * Zero means the controller did not answer, not that it has none.
	 */
	uint32_t nsq_total;
	uint32_t ncq_total;

	uint32_t initialized; ///< Whether the controller is up and being served
};

/**
 * Ask whoever holds the controller at `uri` about it, without connecting
 *
 * The caller names a controller rather than a runtime, so the runtimes are
 * asked in turn until one says it holds this controller.
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
 * Holds a socket at `path`, and one named for each device beside it, and
 * answers processes that connect: hands over the descriptors and offsets they
 * need to build their own view of the runtime, creates queues on request,
 * submits admin commands on their behalf, and releases whatever a client held
 * when it disconnects.
 *
 * Threads are used internally: one reads every connection, and one per
 * controller carries the requests that reach it, so a client waiting on
 * something as slow as a Format does not hold up the rest. They are created
 * and joined here; a caller sees only the blocking call.
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
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_cplane_serve(struct xnvme_dev **devs, int ndevs, uint32_t cplane_id,
		   volatile sig_atomic_t *stop);

#endif /* __LIBXNVME_CPLANE_H */
