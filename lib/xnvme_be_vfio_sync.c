// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
#include <errno.h>
#include <xnvme_dev.h>
#include <xnvme_be_vfio.h>

int
xnvme_be_vfio_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			  size_t mbuf_nbytes)
{
	struct xnvme_be_vfio_state *state = (void *)ctx->dev->be.state;
	struct nvme_ctrl *ctrl = state->ctrl;
	struct nvme_rq *rq;
	uint64_t iova;
	int ret = 0;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_READ:
		break;

	case XNVME_SPEC_FS_OPC_READ:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
		break;

	case XNVME_SPEC_NVM_OPC_WRITE:
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
		break;
	}

	rq = nvme_rq_acquire(state->sq_sync);
	if (!rq) {
		return -errno;
	}

	if (dbuf) {
		if (!vfio_map_vaddr(ctrl->pci.dev.vfio, dbuf, dbuf_nbytes, &iova)) {
			XNVME_DEBUG("FAILED: vfio_iommu_vaddr_to_iova()");
			ret = -EINVAL;
			goto out;
		}

		nvme_rq_map_prp(rq, (union nvme_cmd *)&ctx->cmd, iova, dbuf_nbytes);
	}

	if (mbuf) {
		if (!vfio_map_vaddr(state->ctrl->pci.dev.vfio, mbuf, mbuf_nbytes, &iova)) {
			XNVME_DEBUG("FAILED: vfio_iommu_vaddr_to_iova()");
			ret = -EINVAL;
			goto out;
		}

		ctx->cmd.common.mptr = iova;
	}

	nvme_rq_exec(rq, (union nvme_cmd *)&ctx->cmd);

	if (nvme_rq_spin(rq, (struct nvme_cqe *)&ctx->cpl)) {
		ret = -errno;
	}

out:
	nvme_rq_release(rq);

	return ret;
}
#endif

struct xnvme_be_sync g_xnvme_be_vfio_sync = {
	.id = "nvme",
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
	.cmd_io = xnvme_be_vfio_sync_cmd_io,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
#endif
};
