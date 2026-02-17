// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_upcie_cuda.h>

int
xnvme_be_upcie_cuda_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
				 void *mbuf, size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_queue_upcie *upcie_queue = (struct xnvme_queue_upcie *)ctx->async.queue;
	struct nvme_command *cmd = (struct nvme_command *)&ctx->cmd;
	struct nvme_request *req;
	int err;

	if (upcie_queue->base.outstanding == ctx->async.queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

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

	req = nvme_request_alloc(upcie_queue->qpair.rpool);
	if (!req) {
		XNVME_DEBUG("FAILED: nvme_request_alloc(); errno(%d)", errno);
		return -errno;
	}

	req->user = ctx;
	cmd->cid = req->cid;

	if (dbuf) {
		nvme_request_prep_command_prps_contig_cuda(req, &g_upcie_cuda_rte.cuda_heap, dbuf,
							   dbuf_nbytes, cmd);
	}

	if (mbuf) {
		ctx->cmd.common.mptr = cudamem_dma_v2p(&g_upcie_cuda_rte.cuda_heap, mbuf);
	}

	err = nvme_qpair_enqueue(&upcie_queue->qpair, cmd);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_enqueue();");
		nvme_request_free(upcie_queue->qpair.rpool, req->cid);
		return err;
	}

	upcie_queue->base.outstanding += 1;

	return 0;
}

int
xnvme_be_upcie_cuda_async_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
				  size_t XNVME_UNUSED(dvec_nbytes), void *mbuf,
				  size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_queue_upcie *upcie_queue = (struct xnvme_queue_upcie *)ctx->async.queue;
	struct nvme_command *cmd = (struct nvme_command *)&ctx->cmd;
	struct nvme_request *req;
	int err;

	if (upcie_queue->base.outstanding == ctx->async.queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

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

	req = nvme_request_alloc(upcie_queue->qpair.rpool);
	if (!req) {
		XNVME_DEBUG("FAILED: nvme_request_alloc(); errno(%d)", errno);
		return -errno;
	}

	req->user = ctx;
	cmd->cid = req->cid;

	if (dvec) {
		nvme_request_prep_command_prps_iov_cuda(req, &g_upcie_cuda_rte.cuda_heap, dvec,
							dvec_cnt, cmd);
	}

	if (mbuf) {
		ctx->cmd.common.mptr = cudamem_dma_v2p(&g_upcie_cuda_rte.cuda_heap, mbuf);
	}

	err = nvme_qpair_enqueue(&upcie_queue->qpair, cmd);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_qpair_enqueue();");
		nvme_request_free(upcie_queue->qpair.rpool, req->cid);
		return err;
	}

	upcie_queue->base.outstanding += 1;

	return 0;
}

#endif

struct xnvme_be_async g_xnvme_be_upcie_cuda_async = {
	.id = "upcie-cuda",
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
	.cmd_io = xnvme_be_upcie_cuda_async_cmd_io,
	.cmd_iov = xnvme_be_upcie_cuda_async_cmd_iov,
	.poke = xnvme_be_upcie_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_upcie_queue_init,
	.term = xnvme_be_upcie_queue_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
#endif
};
