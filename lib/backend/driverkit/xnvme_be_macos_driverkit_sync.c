// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_MACOS_ENABLED
#include <xnvme_be_macos_driverkit.h>
#include <xnvme_dev.h>

#include <mach/mach_error.h>
#include <errno.h>

static int
cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_be_macos_driverkit_state *state =
		(struct xnvme_be_macos_driverkit_state *)ctx->dev->be.state;
	size_t output_cnt = sizeof(NvmeSubmitCmd);
	NvmeSubmitCmd cmd = {0};
	NvmeSubmitCmd cmd_return = {0};
	uint64_t _buf_base_offset;
	struct buffer *buf;
	kern_return_t ret;

	cmd.queue_id = state->qid_sync;

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

	if (dbuf) {
		buf = buffer_find(state->buffers, (uint64_t)dbuf);
		if (!buf) {
			XNVME_DEBUG("FAILED: buffer_find()");
			return -EINVAL;
		}
		_buf_base_offset = ((uint64_t)dbuf) - buf->vaddr;
		cmd.dbuf_nbytes = dbuf_nbytes;
		cmd.dbuf_offset = _buf_base_offset;
		cmd.dbuf_token = buf->vaddr;
		XNVME_DEBUG("INFO: buf token: %llx: %llx, %llx", (uint64_t)buf->vaddr,
			    (uint64_t)dbuf, cmd.dbuf_offset);
	}
	if (mbuf) {
		buf = buffer_find(state->buffers, (uint64_t)mbuf);
		if (!buf) {
			XNVME_DEBUG("FAILED: buffer_find()");
			return -EINVAL;
		}
		_buf_base_offset = ((uint64_t)mbuf) - buf->vaddr;
		cmd.mbuf_nbytes = mbuf_nbytes;
		cmd.mbuf_offset = _buf_base_offset;
		cmd.mbuf_token = buf->vaddr;
		XNVME_DEBUG("INFO: buf token: %llx: %llx, %llx", (uint64_t)buf->vaddr,
			    (uint64_t)mbuf, cmd.mbuf_offset);
	}
	memcpy(cmd.cmd, &ctx->cmd, sizeof(struct xnvme_spec_cmd));

	ret = IOConnectCallStructMethod(state->ctrlr->device_connection, NVME_ONESHOT, &cmd,
					sizeof(NvmeSubmitCmd), &cmd_return, &output_cnt);
	if (ret != kIOReturnSuccess) {
		XNVME_DEBUG("FAILED: IOConnectCallStructMethod(NVME_ONESHOT); "
			    "ret(0x%08x), '%s'",
			    ret, mach_error_string(ret));
		return -EIO;
	}
	memcpy(cmd.cpl, &ctx->cpl, sizeof(struct xnvme_spec_cpl));

	return 0;
}
#endif

struct xnvme_be_sync g_xnvme_be_macos_driverkit_sync = {
#ifdef XNVME_PLATFORM_MACOS_ENABLED
	.cmd_io = cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
	.id = "driverkit",
};
