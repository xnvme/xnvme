// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_IOQPAIR_H
#define __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_IOQPAIR_H
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_be_upcie.h>

struct xnvme_be_upcie_cplane_serve_conn;

/**
 * An I/O queue pair a client was given, kept with its connection
 *
 * `dev` says which controller it came off, since two controllers hand out the
 * same queue identifiers. A `foreign` pair lives in memory the client
 * registered, so only the queue and its identifier are the server's to give
 * back.
 */
struct xnvme_be_upcie_cplane_serve_ioqpair {
	struct nvme_qpair qpair;
	struct xnvme_be_upcie_qpair_offsets offsets; ///< Where the heap holds it
	int foreign;                                 ///< Whether it lives in client memory
	int dev;                                     ///< Index of its controller
};

/**
 * Create an I/O queue pair of `depth` entries on `dev` for a client
 *
 * @param held Filled with what it takes to give the pair back
 * @param out Filled with what the client is told
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_serve_ioqpair_alloc(struct xnvme_dev *dev, uint16_t depth,
					  struct xnvme_be_upcie_cplane_serve_ioqpair *held,
					  struct nvme_ioqpair *out);

/**
 * Create an I/O queue pair on `dev` in memory `conn` registered for controller `devidx`
 *
 * `msg` names the registration and the queue offsets in it. The offsets resolve
 * through the client's own description, so a client can name only memory it
 * registered, and only for the controller it registered it for.
 *
 * @return 0 on success, -ENOENT when `conn` has no such registration, another
 * negative errno on error
 */
int
xnvme_be_upcie_cplane_serve_ioqpair_alloc_at(struct xnvme_dev *dev,
					     struct xnvme_be_upcie_cplane_serve_conn *conn,
					     int devidx, const struct nvme_cplane_msg *msg,
					     struct xnvme_be_upcie_cplane_serve_ioqpair *held,
					     uint32_t *qid);

/**
 * Like xnvme_be_upcie_cplane_serve_ioqpair_alloc_at() with only the completion queue placed
 *
 * The submission queue and the PRP scratch come from the heap, as does a second
 * completion queue, which `out` names: the client keeps it a copy of the one
 * it placed.
 *
 * @return 0 on success, -ENOENT when `conn` has no such registration, another
 * negative errno on error
 */
int
xnvme_be_upcie_cplane_serve_ioqpair_alloc_cq_at(struct xnvme_dev *dev,
						struct xnvme_be_upcie_cplane_serve_conn *conn,
						int devidx, const struct nvme_cplane_msg *msg,
						struct xnvme_be_upcie_cplane_serve_ioqpair *held,
						struct nvme_ioqpair *out);

/**
 * Delete a queue pair from xnvme_be_upcie_cplane_serve_ioqpair_alloc() and free its memory
 *
 * `held` is cleared even when the controller refuses, which leaves its memory
 * allocated for as long as the server runs.
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_serve_ioqpair_free(struct xnvme_dev *dev,
					 struct xnvme_be_upcie_cplane_serve_ioqpair *held);

#endif /* XNVME_BE_UPCIE_ENABLED */
#endif /* __INTERNAL_XNVME_BE_UPCIE_CPLANE_SERVE_IOQPAIR_H */
