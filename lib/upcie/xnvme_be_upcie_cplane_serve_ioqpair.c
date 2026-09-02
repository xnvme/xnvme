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
#include <xnvme_be_upcie_cplane_serve_conn.h>
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

	if (held->foreign) {
		err = xnvme_be_upcie_ctrlr_qpair_delete_at(dev, held->qpair.qid);
	} else {
		err = nvme_controller_delete_io_qpair_dmamem(
			ctrl, &held->qpair, &g_upcie_rte.mem.heap, held->offsets.sq,
			held->offsets.cq, held->offsets.prp);
	}
	memset(held, 0, sizeof(*held));

	return err;
}

/*
 * The description of the registration `conn` made at `desc_offset` for controller `devidx`;
 * a mapping is installed into one controller's domain, so it must be this one's
 */
static const struct hostmem_shared_desc *
_registration_desc(const struct xnvme_be_upcie_cplane_serve_conn *conn, int devidx,
		   uint64_t desc_offset)
{
	for (int i = 0; i < conn->nregs; ++i) {
		if ((conn->regs[i].reg.desc_offset == desc_offset) &&
		    (conn->regs[i].dev == devidx)) {
			return (const void *)((char *)g_upcie_rte.mem.dmem.base_va + desc_offset);
		}
	}

	XNVME_DEBUG("FAILED: connection has no registration at offset(%" PRIu64
		    ") for controller %d",
		    desc_offset, devidx);

	return NULL;
}

static int
_xnvme_be_upcie_cplane_serve_ioqpair_alloc_at(struct xnvme_dev *dev,
					      struct xnvme_be_upcie_cplane_serve_conn *conn,
					      int devidx, const struct nvme_cplane_msg *msg,
					      struct xnvme_be_upcie_cplane_serve_ioqpair *held,
					      uint32_t *qid)
{
	const struct hostmem_shared_desc *desc;
	uint64_t sq_addr = 0, cq_addr = 0;
	uint16_t depth;
	int err;

	if (!dev || !conn || !msg || !held || !qid) {
		return -EINVAL;
	}

	depth = msg->u.alloc_ioqpair_at.depth;
	if (!depth) {
		return -EINVAL;
	}

	desc = _registration_desc(conn, devidx, msg->u.alloc_ioqpair_at.desc_offset);
	if (!desc) {
		return -ENOENT;
	}

	err = hostmem_shared_desc_addr(desc, msg->u.alloc_ioqpair_at.sq_offset,
				       (uint64_t)depth * sizeof(struct nvme_command), &sq_addr);
	if (err) {
		XNVME_DEBUG("FAILED: resolving the submission queue; err(%d)", err);
		return err;
	}

	err = hostmem_shared_desc_addr(desc, msg->u.alloc_ioqpair_at.cq_offset,
				       (uint64_t)depth * sizeof(struct nvme_completion), &cq_addr);
	if (err) {
		XNVME_DEBUG("FAILED: resolving the completion queue; err(%d)", err);
		return err;
	}

	err = xnvme_be_upcie_ctrlr_qpair_create_at(dev, sq_addr, cq_addr, depth, qid);
	if (err) {
		return err;
	}

	memset(held, 0, sizeof(*held));
	held->foreign = 1;
	held->qpair.qid = (uint16_t)*qid;
	held->qpair.depth = depth;

	return 0;
}

int
xnvme_be_upcie_cplane_serve_ioqpair_alloc_at(struct xnvme_dev *dev,
					     struct xnvme_be_upcie_cplane_serve_conn *conn,
					     int devidx, const struct nvme_cplane_msg *msg,
					     struct xnvme_be_upcie_cplane_serve_ioqpair *held,
					     uint32_t *qid)
{
	int err;

	xnvme_lock();
	err = _xnvme_be_upcie_cplane_serve_ioqpair_alloc_at(dev, conn, devidx, msg, held, qid);
	xnvme_unlock();

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
