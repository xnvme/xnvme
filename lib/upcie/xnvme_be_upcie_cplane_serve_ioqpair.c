// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/*
 * The I/O queue pairs a server creates and deletes for its clients.
 */
#include <errno.h>

#include <libxnvme.h>
#include <libxnvme_cplane.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <inttypes.h>
#include <string.h>

#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_cplane_serve_ioqpair.h>
#include <xnvme_lock.h>

static int
_xnvme_be_upcie_cplane_serve_ioqpair_alloc(struct xnvme_dev *dev, uint16_t depth,
					   struct xnvme_be_upcie_cplane_serve_ioqpair *held,
					   struct nvme_ioqpair *out)
{
	struct xnvme_be_upcie_state *state;
	struct nvme_controller *ctrl;
	int err;

	if (!dev || !held || !out || !depth) {
		return -EINVAL;
	}

	state = (void *)dev->be.state;
	ctrl = state->ctrlr->ctrl;

	memset(held, 0, sizeof(*held));
	err = nvme_controller_create_io_qpair_dmamem(ctrl, &held->qpair, depth,
						     &g_upcie_rte.mem.heap, &held->offsets.sq,
						     &held->offsets.cq, &held->offsets.prp);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_create_io_qpair_dmamem(); err(%d)", err);
		return err;
	}

	memset(out, 0, sizeof(*out));
	out->sq_offset = held->offsets.sq;
	out->cq_offset = held->offsets.cq;
	out->prp_offset = held->offsets.prp;
	out->qid = held->qpair.qid;
	out->depth = held->qpair.depth;

	return 0;
}

int
xnvme_be_upcie_cplane_serve_ioqpair_alloc(struct xnvme_dev *dev, uint16_t depth,
					  struct xnvme_be_upcie_cplane_serve_ioqpair *held,
					  struct nvme_ioqpair *out)
{
	int err;

	xnvme_lock();
	err = _xnvme_be_upcie_cplane_serve_ioqpair_alloc(dev, depth, held, out);
	xnvme_unlock();

	return err;
}

static int
_xnvme_be_upcie_cplane_serve_ioqpair_free(struct xnvme_dev *dev,
					  struct xnvme_be_upcie_cplane_serve_ioqpair *held)
{
	struct xnvme_be_upcie_state *state;
	struct nvme_controller *ctrl;
	int err;

	if (!dev || !held) {
		return -EINVAL;
	}

	state = (void *)dev->be.state;
	ctrl = state->ctrlr->ctrl;

	err = nvme_controller_delete_io_qpair_dmamem(ctrl, &held->qpair, &g_upcie_rte.mem.heap,
						     held->offsets.sq, held->offsets.cq,
						     held->offsets.prp);
	memset(held, 0, sizeof(*held));

	return err;
}

int
xnvme_be_upcie_cplane_serve_ioqpair_free(struct xnvme_dev *dev,
					 struct xnvme_be_upcie_cplane_serve_ioqpair *held)
{
	int err;

	xnvme_lock();
	err = _xnvme_be_upcie_cplane_serve_ioqpair_free(dev, held);
	xnvme_unlock();

	return err;
}

#endif /* XNVME_BE_UPCIE_ENABLED */
