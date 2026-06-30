// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <errno.h>

/* Full definition is HIP-gated in the header; forward-declare for the stubs. */
struct xnvme_hip_queue;

#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_upcie_hip.h>

int
xnvme_hip_queue_create(struct xnvme_dev *dev, uint16_t depth, struct xnvme_hip_queue **queue)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	struct xnvme_hip_queue *qpair;
	int err;

	err = hipMalloc((hipDeviceptr_t *)&qpair, sizeof(struct xnvme_hip_queue));
	if (err) {
		XNVME_DEBUG("FAILED: hipMalloc(qpair); hipError_t(%d)", err);
		return -ENOMEM;
	}

	// The spec says that for systems where memory ordering is not guaranteed, then one should
	// leave room in the queue to avoid races. Thus, we do so here, by allocating one more than
	// what is needed.
	err = nvme_controller_hip_create_io_qpair(state->ctrlr->ctrl,
						   (struct nvme_qpair_hip *)qpair, depth + 1,
						   &g_upcie_hip_rte.hip_heap);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_hip_create_io_qpair(); err(%d)", err);
		hipFree((hipDeviceptr_t)qpair);
		return -err;
	}

	*queue = qpair;
	return 0;
}

void
xnvme_hip_queue_destroy(struct xnvme_dev *dev, struct xnvme_hip_queue *queue)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;

	nvme_controller_hip_delete_io_qpair(state->ctrlr->ctrl, (struct nvme_qpair_hip *)queue,
					     &g_upcie_hip_rte.hip_heap);
	hipFree((hipDeviceptr_t)queue);
}

#else

int
xnvme_hip_queue_create(struct xnvme_dev *XNVME_UNUSED(dev), uint16_t XNVME_UNUSED(depth),
			struct xnvme_hip_queue **XNVME_UNUSED(queue))
{
	return -ENOSYS;
}

void
xnvme_hip_queue_destroy(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_hip_queue *XNVME_UNUSED(queue))
{
}

#endif
