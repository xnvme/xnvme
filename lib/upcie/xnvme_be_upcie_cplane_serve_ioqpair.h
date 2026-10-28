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

struct xnvme_be_upcie_cplane_serve_ioqpair {
	struct nvme_qpair qpair;
	struct xnvme_be_upcie_qpair_offsets offsets; ///< Where the heap holds it
	int foreign;                                 ///< In client memory
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

int
xnvme_be_upcie_cplane_serve_ioqpair_alloc_at(struct xnvme_dev *dev,
					     struct xnvme_be_upcie_cplane_serve_conn *conn,
					     int devidx, const struct nvme_cplane_msg *msg,
					     struct xnvme_be_upcie_cplane_serve_ioqpair *held,
					     uint32_t *qid);

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
