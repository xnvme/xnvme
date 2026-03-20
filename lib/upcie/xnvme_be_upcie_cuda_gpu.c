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

	// The spec says that for systems where memory ordering is not guaranteed, then one should
	// leave room in the queue to avoid races. Thus, we do so here, by allocating one more than
	// what is needed.
	err = nvme_controller_cuda_create_io_qpair(state->ctrlr->ctrl,
						   (struct nvme_qpair_cuda *)qpair, depth + 1,
						   &g_upcie_cuda_rte.cuda_heap);
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

	nvme_controller_cuda_delete_io_qpair(state->ctrlr->ctrl, (struct nvme_qpair_cuda *)queue,
					     &g_upcie_cuda_rte.cuda_heap);
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
