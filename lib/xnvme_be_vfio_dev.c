// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_VFIO_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_vfio.h>
#include <xnvme_be_cref.h>

static int
_vfio_ctrlr_destructor(void *ctrlr)
{
	nvme_close((struct nvme_ctrl *)ctrlr);

	return 0;
}

int
_xnvme_be_vfio_create_ioqpair(struct xnvme_be_vfio_state *state, int qd, int flags)
{
	unsigned int qid = __builtin_ffsll(state->qidmap);
	int err;

	if (qid < 2) {
		XNVME_DEBUG("no free queue identifiers\n");
		return -EBUSY;
	}

	qid--;

	XNVME_DEBUG("nvme_create_ioqpair(%p, %d, %d, %d)", state->ctrl, qid, qd + 1, flags);

	// nvme queue capacity must be one larger than the requested capacity
	// since only n-1 slots in an NVMe queue may be used
	err = nvme_create_ioqpair(state->ctrl, qid, qd + 1, qid, flags);
	if (err) {
		return -errno;
	}

	state->qidmap &= ~(1ULL << qid);

	return qid;
}

int
_xnvme_be_vfio_delete_ioqpair(struct xnvme_be_vfio_state *state, unsigned int qid)
{
	if (nvme_delete_ioqpair(state->ctrl, qid)) {
		return -errno;
	}

	state->qidmap |= 1ULL << qid;

	return 0;
}

int
xnvme_be_vfio_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_vfio_state *state = (void *)dev->be.state;
	struct nvme_ctrl *ctrl;
	int qpid;
	int err;

	ctrl = xnvme_be_cref_lookup(dev->ident.uri);
	if (!ctrl) {
		struct nvme_ctrl_opts ctrl_opts = {
			.nsqr = XNVME_BE_VFIO_NQUEUES_MAX - 1,
			.ncqr = XNVME_BE_VFIO_NQUEUES_MAX - 1,
		};

		ctrl = calloc(1, sizeof(struct nvme_ctrl));
		if (!ctrl) {
			XNVME_DEBUG("FAILED: calloc()");
			return -ENOMEM;
		}
		err = nvme_init(ctrl, dev->ident.uri, &ctrl_opts);
		if (err) {
			XNVME_DEBUG("FAILED: initializing nvme device: %d", errno);
			return -errno;
		}

		state->ctrl = ctrl;
		state->qidmap = -1 & ~0x1;

		if (state->ctrl->pci.dev.irq_info.count < 2) {
			XNVME_DEBUG("FAILED: no interrupt vector");
			return -ENOSYS;
		}

		state->nefds =
			XNVME_MIN(XNVME_BE_VFIO_NQUEUES_MAX, state->ctrl->pci.dev.irq_info.count);

		state->efds = calloc(state->nefds, sizeof(int));
		if (!state->efds) {
			XNVME_DEBUG("FAILED: calloc()");
			return -ENOMEM;
		}

		for (int i = 0; i < state->nefds; i++) {
			state->efds[i] = -1;
		}

		if (vfio_set_irq(&state->ctrl->pci.dev, state->efds, state->nefds)) {
			XNVME_DEBUG("FAILED: vfio set irqs");
		}

		// create queues for sync backend requests
		qpid = _xnvme_be_vfio_create_ioqpair(state, 31, 0x0);

		if (qpid < 0) {
			XNVME_DEBUG("FAILED: initializing qpairs for sync IO: %d", err);
			free(state->efds);
			return -errno;
		}
		state->sq_sync = &state->ctrl->sq[qpid];
		state->cq_sync = &state->ctrl->cq[qpid];

		err = xnvme_be_cref_insert(dev->ident.uri, xnvme_be_vfio.attr.name, ctrl,
					   _vfio_ctrlr_destructor);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_be_cref_insert()");
			free(state->efds);
			return err;
		}
	}

	dev->ident.dtype =
		dev->opts.nsid ? XNVME_DEV_TYPE_NVME_NAMESPACE : XNVME_DEV_TYPE_NVME_CONTROLLER;
	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	dev->ident.nsid = dev->opts.nsid;

	return 0;
}

void
xnvme_be_vfio_dev_close(struct xnvme_dev *dev)
{
	struct xnvme_be_vfio_state *state = (void *)dev->be.state;

	if (xnvme_be_cref_deref(state->ctrl, XNVME_BE_CREF_DESTROY_IMMEDIATE)) {
		XNVME_DEBUG("FAILED: xnvme_be_cref_deref()");
	}
	if (state->efds) {
		free(state->efds);
	}
}

#endif

struct xnvme_be_dev g_xnvme_be_vfio_dev = {
#ifdef XNVME_BE_VFIO_ENABLED
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_vfio_dev_open,
	.dev_close = xnvme_be_vfio_dev_close,
	.id = "libvfn",
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
	.id = "nosys",
#endif
};
