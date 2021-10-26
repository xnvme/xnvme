// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Michael Bang <mi.bang@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
#include <xnvme_dev.h>
#include <errno.h>
#include <xnvme_queue.h>
#include <xnvme_be_vfio.h>

int
xnvme_be_vfio_sync_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			     void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_vfio_state *state = (void *)ctx->dev->be.state;
	struct nvme_ctrl *ctrl = state->ctrl;
	union nvme_cmd *cmd = (union nvme_cmd *)&ctx->cmd;
	struct nvme_cqe *cqe = (struct nvme_cqe *)&ctx->cpl;

	dbuf_nbytes = ALIGN_UP(dbuf_nbytes, 4096);

	if (nvme_oneshot(ctrl, ctrl->adminq.sq, cmd, dbuf, dbuf_nbytes, cqe)) {
		XNVME_DEBUG("FAILED: nvme_oneshot()");
		return -errno;
	}

	return 0;
}
#endif

struct xnvme_be_admin g_xnvme_be_vfio_admin = {
	.id = "nvme",
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
	.cmd_admin = xnvme_be_vfio_sync_cmd_admin,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
#endif
};
