// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_FREEBSD_ENABLED
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");
#include <sys/param.h>
#include <sys/ioccom.h>
#include <sys/stat.h>

#include <paths.h>
#include <fcntl.h>
#include <unistd.h>
#include <dev/nvme/nvme.h>
#include <errno.h>
#include <xnvme_dev.h>
#include <xnvme_be_freebsd.h>

static int
xnvme_be_freebsd_nvme_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			 size_t mbuf_nbytes)
{
	struct xnvme_be_freebsd_state *state = (void *)ctx->dev->be.state;
	struct nvme_pt_command ptc = {0};
	int err;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOTSUP;
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

	memcpy(&ptc.cmd, &ctx->cmd, sizeof(ptc.cmd));
	ptc.buf = dbuf;
	ptc.len = dbuf_nbytes;
	ptc.is_read = (ptc.cmd.opc & 0x2) != 0;

	err = ioctl(state->fd.ns, NVME_PASSTHROUGH_CMD, &ptc);
	if (err < 0) {
		XNVME_DEBUG("FAILED: ioctl() err: %d, errno: %d", err, errno);
		return err;
	}

	memcpy(&ctx->cpl, &ptc.cpl, sizeof(ctx->cpl));

	return 0;
}
#endif

struct xnvme_be_sync g_xnvme_be_freebsd_sync_nvme = {
	.id = "nvme",
#ifdef XNVME_PLATFORM_FREEBSD_ENABLED
	.cmd_io = xnvme_be_freebsd_nvme_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
