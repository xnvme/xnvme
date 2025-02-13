// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_VFIO_ENABLED
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
	case XNVME_SPEC_FS_OPC_READ:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
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
		if (iommu_map_vaddr(ctrl->pci.dev.ctx, dbuf, dbuf_nbytes, &iova, 0)) {
			XNVME_DEBUG("FAILED: iommu_map_vaddr()");
			ret = -EINVAL;
			goto out;
		}

		nvme_rq_map_prp(ctrl, rq, (union nvme_cmd *)&ctx->cmd, iova, dbuf_nbytes);
	}

	if (mbuf) {
		if (iommu_map_vaddr(ctrl->pci.dev.ctx, mbuf, mbuf_nbytes, &iova, 0)) {
			XNVME_DEBUG("FAILED: iommu_map_vaddr()");
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

int
xnvme_be_vfio_sync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			   size_t XNVME_UNUSED(dvec_nbytes), void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_be_vfio_state *state = (void *)ctx->dev->be.state;
	struct nvme_ctrl *ctrl = state->ctrl;
	struct nvme_rq *rq;
	uint64_t iova;
	int ret = 0;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_FS_OPC_READ:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
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

	if (dvec) {
		nvme_rq_mapv(ctrl, rq, (union nvme_cmd *)&ctx->cmd, dvec, dvec_cnt);
	}

	if (mbuf) {
		if (iommu_map_vaddr(ctrl->pci.dev.ctx, mbuf, mbuf_nbytes, &iova, 0)) {
			XNVME_DEBUG("FAILED: iommu_map_vaddr()");
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
#ifdef XNVME_BE_VFIO_ENABLED
	.cmd_io = xnvme_be_vfio_sync_cmd_io,
	.cmd_iov = xnvme_be_vfio_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
