// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_VFIO_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_vfio.h>

int
_xnvme_be_vfio_create_ioqpair(struct xnvme_be_vfio_state *state, int qd, int flags)
{
	struct xnvme_be_vfio_ctrlr *ctrlr = state->ctrlr;
	unsigned int qid = __builtin_ffsll(ctrlr->qidmap);
	int err;

	if (qid < 2) {
		XNVME_DEBUG("FAILED: no free queue identifiers");
		return -EBUSY;
	}

	qid--;

	XNVME_DEBUG("nvme_create_ioqpair(%p, %d, %d, %d)", ctrlr->ctrl, qid, qd + 1, flags);

	// nvme queue capacity must be one larger than the requested capacity
	// since only n-1 slots in an NVMe queue may be used
	err = nvme_create_ioqpair(ctrlr->ctrl, qid, qd + 1, qid, flags);
	if (err) {
		return -errno;
	}

	ctrlr->qidmap &= ~(1ULL << qid);

	return qid;
}

int
_xnvme_be_vfio_delete_ioqpair(struct xnvme_be_vfio_state *state, unsigned int qid)
{
	struct xnvme_be_vfio_ctrlr *ctrlr = state->ctrlr;

	if (nvme_delete_ioqpair(ctrlr->ctrl, qid)) {
		return -errno;
	}

	ctrlr->qidmap |= 1ULL << qid;

	return 0;
}

/**
 * Initialize the VFIO/libvfn controller.
 *
 * Allocates a shared xnvme_be_vfio_ctrlr, initializes the NVMe controller,
 * sets up IRQs and event FDs. The returned handle is stored in cref and
 * written to dev->be.state[0] by the platform.
 */
static void *
xnvme_be_vfio_ctrlr_init(struct xnvme_dev *dev)
{
	struct nvme_ctrl_opts ctrl_opts = {
		.nsqr = XNVME_BE_VFIO_NQUEUES_MAX - 1,
		.ncqr = XNVME_BE_VFIO_NQUEUES_MAX - 1,
	};
	struct xnvme_be_vfio_ctrlr *ctrlr;
	struct nvme_ctrl *ctrl;

	ctrlr = calloc(1, sizeof(*ctrlr));
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: calloc(ctrlr)");
		return NULL;
	}

	ctrl = calloc(1, sizeof(*ctrl));
	if (!ctrl) {
		XNVME_DEBUG("FAILED: calloc(ctrl)");
		free(ctrlr);
		return NULL;
	}
	if (nvme_init(ctrl, dev->ident.uri, &ctrl_opts)) {
		XNVME_DEBUG("FAILED: initializing nvme device: %d", errno);
		free(ctrl);
		free(ctrlr);
		return NULL;
	}

	ctrlr->ctrl = ctrl;
	ctrlr->qidmap = -1 & ~0x1;

	if (ctrl->pci.dev.irq_info.count < 2) {
		XNVME_DEBUG("FAILED: no interrupt vector");
		goto fail;
	}

	ctrlr->nefds = XNVME_MIN(XNVME_BE_VFIO_NQUEUES_MAX, ctrl->pci.dev.irq_info.count);

	ctrlr->efds = calloc(ctrlr->nefds, sizeof(int));
	if (!ctrlr->efds) {
		XNVME_DEBUG("FAILED: calloc(efds)");
		goto fail;
	}

	for (int i = 0; i < ctrlr->nefds; i++) {
		ctrlr->efds[i] = -1;
	}

	if (vfio_set_irq(&ctrl->pci.dev, ctrlr->efds, ctrlr->nefds)) {
		XNVME_DEBUG("FAILED: vfio set irqs");
	}

	{
		struct xnvme_be_vfio_state tmp_state = {.ctrlr = ctrlr};
		int qpid;

		qpid = _xnvme_be_vfio_create_ioqpair(&tmp_state, 31, 0x0);
		if (qpid < 0) {
			XNVME_DEBUG("FAILED: creating sync qpair: %d", qpid);
			goto fail_efds;
		}
		ctrlr->sq_sync = &ctrl->sq[qpid];
		ctrlr->cq_sync = &ctrl->cq[qpid];
	}

	return ctrlr;

fail_efds:
	free(ctrlr->efds);

fail:
	nvme_close(ctrl);
	free(ctrlr);
	return NULL;
}

static int
xnvme_be_vfio_ctrlr_term(void *handle)
{
	struct xnvme_be_vfio_ctrlr *ctrlr = handle;

	if (ctrlr->sq_sync) {
		nvme_delete_ioqpair(ctrlr->ctrl, ctrlr->sq_sync->id);
	}

	free(ctrlr->efds);
	nvme_close(ctrlr->ctrl);
	free(ctrlr);

	return 0;
}

int
xnvme_be_vfio_dev_open(struct xnvme_dev *dev)
{
	dev->ident.dtype =
		dev->opts.nsid ? XNVME_DEV_TYPE_NVME_NAMESPACE : XNVME_DEV_TYPE_NVME_CONTROLLER;
	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	dev->ident.nsid = dev->opts.nsid;

	return 0;
}

void
xnvme_be_vfio_dev_close(struct xnvme_dev *dev)
{
	(void)dev;
}

#endif

struct xnvme_be_dev g_xnvme_be_vfio_dev = {
#ifdef XNVME_BE_VFIO_ENABLED
	.dev_open = xnvme_be_vfio_dev_open,
	.dev_close = xnvme_be_vfio_dev_close,
	.id = "libvfn",
	.ctrlr_init = xnvme_be_vfio_ctrlr_init,
	.ctrlr_term = xnvme_be_vfio_ctrlr_term,
#else
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
	.id = "nosys",
	.ctrlr_init = NULL,
	.ctrlr_term = NULL,
#endif
};
