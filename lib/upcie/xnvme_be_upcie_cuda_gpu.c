// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <errno.h>
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_upcie_cuda.h>

int
xnvme_cuda_queue_create(struct xnvme_dev *dev, uint16_t depth, struct xnvme_cuda_queue **queue)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	struct xnvme_cuda_queue *qpair;
	int err;

	err = cuMemAlloc((CUdeviceptr *)&qpair, sizeof(struct xnvme_cuda_queue));
	if (err) {
		XNVME_DEBUG("FAILED: cuMemAlloc(qpair); CUresult(%d)", err);
		return -ENOMEM;
	}

	err = xnvme_be_upcie_mproc_qids_lock(state->ctrlr);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_mproc_qids_lock(); err(%d)", err);
		cuMemFree((CUdeviceptr)qpair);
		return err;
	}

	// The spec says that for systems where memory ordering is not guaranteed, then one should
	// leave room in the queue to avoid races. Thus, we do so here, by allocating one more than
	// what is needed.
	err = nvme_controller_cuda_create_io_qpair(state->ctrlr->ctrl,
						   (struct nvme_qpair_cuda *)qpair, depth + 1,
						   &g_upcie_cuda_rte.cuda_heap, state->dmem);

	xnvme_be_upcie_mproc_qids_unlock(state->ctrlr);

	if (err) {
		XNVME_DEBUG("FAILED: nvme_controller_cuda_create_io_qpair(); err(%d)", err);
		cuMemFree((CUdeviceptr)qpair);
		return -err;
	}

	*queue = qpair;
	return 0;
}

void
xnvme_cuda_queue_destroy(struct xnvme_dev *dev, struct xnvme_cuda_queue *queue)
{
	struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	struct nvme_qpair_cuda qpair = {0};
	int cu_err, err;

	err = xnvme_be_upcie_mproc_qids_lock(state->ctrlr);
	if (err) {
		// Without the admin queue the device-side queues cannot be deleted, and
		// releasing the GPU memory they still reference would be worse than leaking it.
		XNVME_DEBUG("FAILED: xnvme_be_upcie_mproc_qids_lock(); err(%d)", err);
		return;
	}

	// nvme_controller_cuda_delete_io_qpair() does not return the queue identifier to the
	// pool, unlike its host counterpart. Read it off the device-side queue-pair and release
	// it here, so a create/destroy cycle does not drain the identifier space.
	cu_err = cuMemcpyDtoH(&qpair, (CUdeviceptr)queue, sizeof(qpair));
	if (cu_err) {
		XNVME_DEBUG("FAILED: cuMemcpyDtoH(device QP -> host QP); CUresult(%d)", cu_err);
	}

	nvme_controller_cuda_delete_io_qpair(state->ctrlr->ctrl, (struct nvme_qpair_cuda *)queue,
					     &g_upcie_cuda_rte.cuda_heap);

	if (!cu_err) {
		nvme_qid_free(state->ctrlr->ctrl->qids, qpair.qid);
	}

	xnvme_be_upcie_mproc_qids_unlock(state->ctrlr);

	cuMemFree((CUdeviceptr)queue);
}

#else

int
xnvme_cuda_queue_create(struct xnvme_dev *XNVME_UNUSED(dev), uint16_t XNVME_UNUSED(depth),
			struct xnvme_cuda_queue **XNVME_UNUSED(queue))
{
	return -ENOSYS;
}

void
xnvme_cuda_queue_destroy(struct xnvme_dev *XNVME_UNUSED(dev),
			 struct xnvme_cuda_queue *XNVME_UNUSED(queue))
{
}

#endif
