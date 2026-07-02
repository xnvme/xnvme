// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <errno.h>

/* Full definition is HIP-gated in the header; forward-declare for the stubs. */
struct xnvme_hip_gpuinit_queue;

#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_upcie_hip.h>
#include <xnvme_be_upcie_hip_gpuinit.h>

int
xnvme_hip_gpuinit_queue_create(struct xnvme_dev *dev, uint16_t depth,
			       struct xnvme_hip_gpuinit_queue **queue)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	struct xnvme_hip_gpuinit_queue *q;
	int err;

	/* The queue wrapper and its host-side bridge live in host memory; the
	 * device-resident qpair is allocated separately in GPU memory below.
	 */
	q = calloc(1, sizeof(*q));
	if (!q) {
		XNVME_DEBUG("FAILED: calloc(gpuinit queue); errno(%d)", errno);
		return -errno;
	}

	err = hipMalloc((hipDeviceptr_t *)&q->qpair, sizeof(struct nvme_qpair_hip));
	if (err) {
		XNVME_DEBUG("FAILED: hipMalloc(qpair); hipError_t(%d)", err);
		free(q);
		return -ENOMEM;
	}

	/* Leave one extra slot: on systems without guaranteed memory ordering the
	 * ring needs headroom to avoid producer/consumer races.
	 */
	err = nvme_controller_hip_create_io_qpair(state->ctrlr->ctrl, q->qpair, depth + 1,
						  &g_upcie_hip_rte.hip_heap, &q->bridge);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_hip_create_io_qpair(); err(%d)", err);
		hipFree((hipDeviceptr_t)q->qpair);
		free(q);
		return -err;
	}

	/* One doorbell/CQ bridge poller per qpair, running for the queue lifetime. */
	err = nvme_hip_db_bridge_start(&q->bridge);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_hip_db_bridge_start(); err(%d)", err);
		nvme_controller_hip_delete_io_qpair(state->ctrlr->ctrl, q->qpair,
						    &g_upcie_hip_rte.hip_heap, &q->bridge);
		hipFree((hipDeviceptr_t)q->qpair);
		free(q);
		return err;
	}

	*queue = q;
	return 0;
}

void
xnvme_hip_gpuinit_queue_destroy(struct xnvme_dev *dev, struct xnvme_hip_gpuinit_queue *queue)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;

	/* delete_io_qpair terminates the bridge (stops the poller and frees the
	 * host SQ/CQ/shadow allocations).
	 */
	nvme_controller_hip_delete_io_qpair(state->ctrlr->ctrl, queue->qpair,
					    &g_upcie_hip_rte.hip_heap, &queue->bridge);
	hipFree((hipDeviceptr_t)queue->qpair);
	free(queue);
}

#else

int
xnvme_hip_gpuinit_queue_create(struct xnvme_dev *XNVME_UNUSED(dev), uint16_t XNVME_UNUSED(depth),
			       struct xnvme_hip_gpuinit_queue **XNVME_UNUSED(queue))
{
	return -ENOSYS;
}

void
xnvme_hip_gpuinit_queue_destroy(struct xnvme_dev *XNVME_UNUSED(dev),
				struct xnvme_hip_gpuinit_queue *XNVME_UNUSED(queue))
{
}

#endif
