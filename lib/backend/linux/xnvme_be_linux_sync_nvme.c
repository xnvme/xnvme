// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_LINUX_ENABLED
#include <errno.h>
#include <fcntl.h>
#include <linux/nvme_ioctl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xnvme_be_linux.h>
#include <xnvme_spec.h>

#ifdef NVME_IOCTL_IO64_CMD_VEC
static int
xnvme_be_linux_nvme_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			    size_t dvec_nbytes, void *mbuf, size_t mbuf_nbytes)
{
	struct nvme_passthru_cmd64 *kcmd = (void *)ctx;
	int err;

	/**
	 * NOTE: see xnvme_be_linux_nvme_cmd_io()
	 */
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_FS_OPC_READ:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.nvm.nlb = (dvec_nbytes / ctx->dev->geo.lba_nbytes) - 1;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.nvm.nlb = (dvec_nbytes / ctx->dev->geo.lba_nbytes) - 1;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
		break;
	}

	kcmd->addr = (uint64_t)dvec;
	kcmd->vec_cnt = dvec_cnt;
	kcmd->metadata = (uint64_t)mbuf;
	kcmd->metadata_len = mbuf_nbytes;

	err = xnvme_be_linux_nvme_ioctl(ctx->dev, NVME_IOCTL_IO64_CMD_VEC, ctx);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_linux_nvme_ioctl(), err: %d", err);
		return err;
	}

	return 0;
}
#else
static int
xnvme_be_linux_nvme_cmd_iov(struct xnvme_cmd_ctx *XNVME_UNUSED(ctx),
			    struct iovec *XNVME_UNUSED(dvec), size_t XNVME_UNUSED(dvec_cnt),
			    size_t XNVME_UNUSED(dvec_nbytes), void *XNVME_UNUSED(mbuf),
			    size_t XNVME_UNUSED(mbuf_nbytes))
{
	XNVME_DEBUG("FAILED: NVME_IOCTL_IO64_CMD_VEC; ENOSYS");
	return -ENOSYS;
}
#endif

static int
xnvme_be_linux_nvme_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t mbuf_nbytes)
{
#ifdef NVME_IOCTL_IO64_CMD
	struct nvme_passthru_cmd64 *kcmd = (void *)ctx;
#else
	struct nvme_passthru_cmd *kcmd = (void *)ctx;
#endif
	int err;

	/**
	 * NOTE: For the pseudo-spec encapsulating file-system operations in NVMe-commands aka
	 * SPEC_FS, then the slba aka "offset" is given in units of bytes, this is converted here
	 * to an NVM-io command by shifting to obtain a unit of blocks. This of course requires
	 * that the SPEC_FS operation is aligned to the the block-width, if it is not, then it will
	 * silently write to "truncated" address.
	 */
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_FS_OPC_READ:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.nvm.nlb = (dbuf_nbytes / ctx->dev->geo.lba_nbytes) - 1;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.nvm.nlb = (dbuf_nbytes / ctx->dev->geo.lba_nbytes) - 1;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
		break;
	}

	kcmd->addr = (uint64_t)dbuf;
	kcmd->data_len = dbuf_nbytes;
	kcmd->metadata = (uint64_t)mbuf;
	kcmd->metadata_len = mbuf_nbytes;

#ifdef NVME_IOCTL_IO64_CMD
	err = xnvme_be_linux_nvme_ioctl(ctx->dev, NVME_IOCTL_IO64_CMD, ctx);
#else
	err = xnvme_be_linux_nvme_ioctl(ctx->dev, NVME_IOCTL_IO_CMD, ctx);
#endif
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_linux_nvme_ioctl(), err: %d", err);
		return err;
	}

	return 0;
}
#endif

struct xnvme_be_sync g_xnvme_be_linux_sync_nvme = {
	.id = "nvme",
#ifdef XNVME_PLATFORM_LINUX_ENABLED
	.cmd_io = xnvme_be_linux_nvme_cmd_io,
	.cmd_iov = xnvme_be_linux_nvme_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
