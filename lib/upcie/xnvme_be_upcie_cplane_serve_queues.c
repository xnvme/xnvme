// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Taking a queue off a controller, and giving it back
 *
 * What a client is given comes from here, and the identifier space and the
 * admin queue behind it are the server's however the memory was found.
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
#include <xnvme_be_upcie_cplane_serve.h>

/**
 * Allocate a queue for a client and describe it in offsets
 *
 * The memory and the controller's own record of the queue both come from here:
 * the submission and completion queues are carved from the heap and the
 * controller is told to create the pair over them. What comes back is kept in
 * the connection's own record, since that is who has to give it back.
 *
 * @param dev A device this process opened
 * @param depth Entries the client asked for
 * @param held Pre-allocated slot in the connection's record
 * @param out Pre-allocated allocation to fill
 *
 * @return 0 on success, negative errno on error
 */
int
serve_ioqpair_alloc(struct xnvme_dev *dev, uint16_t depth, struct serve_ioqpair *held,
		    struct serve_qalloc *out)
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
						     &g_upcie_rte.mem.heap, &held->sq_offset,
						     &held->cq_offset, &held->prp_offset);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_create_io_qpair_dmamem(); err(%d)", err);
		return err;
	}

	memset(out, 0, sizeof(*out));
	out->sq_offset = held->sq_offset;
	out->cq_offset = held->cq_offset;
	out->prp_offset = held->prp_offset;
	out->qid = held->qpair.qid;
	out->depth = held->qpair.depth;

	return 0;
}

/**
 * Build a queue on memory the client registered
 *
 * The offsets are resolved through the client's own description, so a client
 * can only ever name memory it registered; naming somebody else's is not a
 * thing the protocol can express.
 *
 * @param dev A device this process opened
 * @param conn The connection asking, whose registrations bound what it may name
 * @param devidx The controller the queue is for, which the registration must match
 * @param msg The request
 * @param held Pre-allocated record to fill
 * @param qid Set to the identifier allocated
 *
 * @return 0 on success, negative errno on error
 */
int
serve_ioqpair_alloc_at(struct xnvme_dev *dev, struct serve_conn *conn, int devidx,
		       const struct nvme_cplane_msg *msg, struct serve_ioqpair *held,
		       uint32_t *qid)
{
	const struct hostmem_shared_desc *desc = NULL;
	uint64_t sq_addr = 0, cq_addr = 0;
	uint16_t depth;
	int err;

	if (!dev || !conn || !msg || !held || !qid) {
		return -EINVAL;
	}

	depth = msg->u.queue_at.depth;
	if (!depth) {
		return -EINVAL;
	}

	/* The controller has to match as well as the offset. A mapping was
	 * installed into one controller's domain, so resolving through it for
	 * another gives addresses that controller has nothing mapped at, and
	 * nothing downstream would catch that. */
	for (int i = 0; i < conn->nregs; ++i) {
		if ((conn->regs[i].reg.desc_offset == msg->u.queue_at.desc_offset) &&
		    (conn->regs[i].dev == devidx)) {
			desc = (const void *)((char *)g_upcie_rte.mem.dmem.base_va +
					      conn->regs[i].reg.desc_offset);
			break;
		}
	}
	if (!desc) {
		XNVME_DEBUG("FAILED: connection has no registration at offset(%" PRIu64
			    ") for controller %d",
			    msg->u.queue_at.desc_offset, devidx);
		return -ENOENT;
	}

	err = hostmem_shared_desc_addr(desc, msg->u.queue_at.sq_offset,
				       (uint64_t)depth * sizeof(struct nvme_command), &sq_addr);
	if (err) {
		XNVME_DEBUG("FAILED: resolving the submission queue; err(%d)", err);
		return err;
	}

	err = hostmem_shared_desc_addr(desc, msg->u.queue_at.cq_offset,
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

/**
 * Delete a queue allocated earlier and release what went with it
 *
 * @param dev A device this process opened
 * @param held The connection's record of the queue
 *
 * @return 0 on success, negative errno on error
 */
int
serve_ioqpair_free(struct xnvme_dev *dev, struct serve_ioqpair *held)
{
	struct xnvme_be_upcie_state *state;
	struct nvme_controller *ctrl;
	int err;

	if (!dev || !held) {
		return -EINVAL;
	}

	state = (void *)dev->be.state;
	ctrl = state->ctrlr->ctrl;

	/* The queue goes before its memory does: the controller has to stop
	 * being able to reach an address before it stops resolving, which this
	 * does in one call.
	 *
	 * A failure there keeps both rather than returning them, so what is
	 * lost is one queue's memory for as long as this server runs. The
	 * record of it goes either way: the server has nothing further it can
	 * do with a queue the controller will not give up, and holding the slot
	 * would only stop the client's other queues from coming back. */
	if (held->foreign) {
		err = xnvme_be_upcie_ctrlr_qpair_delete_at(dev, held->qpair.qid);
	} else {
		err = nvme_controller_delete_io_qpair_dmamem(
			ctrl, &held->qpair, &g_upcie_rte.mem.heap, held->sq_offset,
			held->cq_offset, held->prp_offset);
	}
	memset(held, 0, sizeof(*held));

	return err;
}

#endif /* XNVME_BE_UPCIE_ENABLED */
