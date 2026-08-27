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
	err = nvme_controller_delete_io_qpair_dmamem(ctrl, &held->qpair, &g_upcie_rte.mem.heap,
						     held->sq_offset, held->cq_offset,
						     held->prp_offset);
	memset(held, 0, sizeof(*held));

	return err;
}

#endif /* XNVME_BE_UPCIE_ENABLED */
