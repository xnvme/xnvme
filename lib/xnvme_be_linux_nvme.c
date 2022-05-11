// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_ENABLED
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/nvme_ioctl.h>
#include <libxnvme_spec_fs.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_nvme.h>

#ifdef XNVME_DEBUG_ENABLED
static const char *
ioctl_request_to_str(unsigned long req)
{
	switch (req) {
	case NVME_IOCTL_ID:
		return "NVME_IOCTL_ID";
	case NVME_IOCTL_ADMIN_CMD:
		return "NVME_IOCTL_ADMIN_CMD";
#ifdef NVME_IOCTL_ADMIN64_CMD
	case NVME_IOCTL_ADMIN64_CMD:
		return "NVME_IOCTL_ADMIN64_CMD";
#endif
	case NVME_IOCTL_SUBMIT_IO:
		return "NVME_IOCTL_SUBMIT_IO";
	case NVME_IOCTL_IO_CMD:
		return "NVME_IOCTL_IO_CMD";
#ifdef NVME_IOCTL_IO64_CMD
	case NVME_IOCTL_IO64_CMD:
		return "NVME_IOCTL_IO64_CMD";
#endif
#ifdef NVME_IOCTL_IO64_CMD_VEC
	case NVME_IOCTL_IO64_CMD_VEC:
		return "NVME_IOCTL_IO64_CMD_VEC";
#endif
#ifdef NVME_URING_CMD_IO
	case NVME_URING_CMD_IO:
		return "NVME_URING_CMD_IO";
#endif
#ifdef NVME_URING_CMD_IO_VEC
	case NVME_URING_CMD_IO_VEC:
		return "NVME_URING_CMD_IO_VEC";
#endif
	case NVME_IOCTL_RESET:
		return "NVME_IOCTL_RESET";
	case NVME_IOCTL_SUBSYS_RESET:
		return "NVME_IOCTL_SUBSYS_RESET";
	case NVME_IOCTL_RESCAN:
		return "NVME_IOCTL_RESCAN";

	default:
		return "NVME_IOCTL_UNKNOWN";
	}
}
#endif

struct _kernel_cpl {
	union {
		struct {
			uint32_t timeout_ms;
			uint32_t result;
			uint64_t rsvd;
		} res32;

		struct {
			uint32_t timeout_ms;
			uint32_t rsvd1;
			uint64_t result;
		} res64;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cpl) == sizeof(struct _kernel_cpl), "Incorrect size")

int
xnvme_be_linux_nvme_map_cpl(struct xnvme_cmd_ctx *ctx, unsigned long ioctl_req, int res)
{
	struct _kernel_cpl *kcpl = (void *)&ctx->cpl;

	switch (ioctl_req) {
	case NVME_IOCTL_ADMIN_CMD:
	case NVME_IOCTL_IO_CMD:
		ctx->cpl.result = kcpl->res32.result;
		break;
#ifdef NVME_IOCTL_IO64_CMD
	case NVME_IOCTL_IO64_CMD:
		ctx->cpl.result = kcpl->res64.result;
		break;
#endif
#ifdef NVME_IOCTL_IO64_CMD_VEC
	case NVME_IOCTL_IO64_CMD_VEC:
		ctx->cpl.result = kcpl->res64.result;
		break;
#endif
#ifdef NVME_IOCTL_ADMIN64_CMD
	case NVME_IOCTL_ADMIN64_CMD:
		ctx->cpl.result = kcpl->res64.result;
		break;
#endif
#ifdef NVME_URING_CMD_IO
	case NVME_URING_CMD_IO:
		break;
#endif
#ifdef NVME_URING_CMD_IO_VEC
	case NVME_URING_CMD_IO_VEC:
		break;
#endif
	default:
		XNVME_DEBUG("FAILED: ioctl_req: %lu, res: %d", ioctl_req, res);
		return -ENOSYS;
	}

	ctx->cpl.sqhd = 0;
	ctx->cpl.sqid = 0;
	ctx->cpl.cid = 0;
	ctx->cpl.status.val = 0;
	if (res) {
		ctx->cpl.status.sc = res & 0xFF;
		ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
	}

	return 0;
}

static inline int
ioctl_wrap(struct xnvme_dev *dev, unsigned long ioctl_req, struct xnvme_cmd_ctx *ctx)
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;
	int err;

	err = ioctl(state->fd, ioctl_req, ctx);
	XNVME_DEBUG("INFO: ioctl(%s), err(%d), errno(%d)", ioctl_request_to_str(ioctl_req), err,
		    errno);
	if (!err) {
		return xnvme_be_linux_nvme_map_cpl(ctx, ioctl_req, 0);
	}

	if ((err == -1) && errno) {
		XNVME_DEBUG("INFO: retconv: -1 and set errno")
		if (xnvme_be_linux_nvme_map_cpl(ctx, ioctl_req, errno)) {
			XNVME_DEBUG("FAILED: xnvme_be_linux_nvme_map_cpl(errno:%d)", errno);
		}
		return -errno;
	} else if (err < 0) {
		XNVME_DEBUG("INFO: retconv: -errno; unexpected ioctl() return-val convention.")
		if (xnvme_be_linux_nvme_map_cpl(ctx, ioctl_req, err)) {
			XNVME_DEBUG("FAILED: xnvme_be_linux_nvme_map_cpl(err:%d)", err);
		}
		return err;
	}

	XNVME_DEBUG("INFO: retconv: NVMe-completion-status-code. encoded in lower 16-bit")
	if (xnvme_be_linux_nvme_map_cpl(ctx, ioctl_req, err)) {
		XNVME_DEBUG("FAILED: xnvme_be_linux_nvme_map_cpl(err:%d)", err);
	}

	return -EIO;
}

#ifdef NVME_IOCTL_IO64_CMD_VEC
int
xnvme_be_linux_nvme_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			    size_t XNVME_UNUSED(dvec_nbytes), struct iovec *mvec, size_t mvec_cnt,
			    size_t XNVME_UNUSED(mvec_nbytes))
{
	struct nvme_passthru_cmd64 *kcmd = (void *)ctx;
	int err;

	/**
	 * NOTE: see xnvme_be_linux_nvme_cmd_io()
	 */
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

	kcmd->addr = (uint64_t)dvec;
	kcmd->vec_cnt = dvec_cnt;
	kcmd->metadata = (uint64_t)mvec;
	kcmd->metadata_len = mvec_cnt;

	err = ioctl_wrap(ctx->dev, NVME_IOCTL_IO64_CMD_VEC, ctx);
	if (err) {
		XNVME_DEBUG("FAILED: ioctl_wrap(), err: %d", err);
		return err;
	}

	return 0;
}
#else
int
xnvme_be_linux_nvme_cmd_iov(struct xnvme_cmd_ctx *XNVME_UNUSED(ctx),
			    struct iovec *XNVME_UNUSED(dvec), size_t XNVME_UNUSED(dvec_cnt),
			    size_t XNVME_UNUSED(dvec_nbytes), struct iovec *XNVME_UNUSED(mvec),
			    size_t XNVME_UNUSED(mvec_cnt), size_t XNVME_UNUSED(mvec_nbytes))
{
	XNVME_DEBUG("FAILED: NVME_IOCTL_IO64_CMD_VEC; ENOSYS");
	return -ENOSYS;
}
#endif

int
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
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
		break;
	}

	kcmd->addr = (uint64_t)dbuf;
	kcmd->data_len = dbuf_nbytes;
	kcmd->metadata = (uint64_t)mbuf;
	kcmd->metadata_len = mbuf_nbytes;

#ifdef NVME_IOCTL_IO64_CMD
	err = ioctl_wrap(ctx->dev, NVME_IOCTL_IO64_CMD, ctx);
#else
	err = ioctl_wrap(ctx->dev, NVME_IOCTL_IO_CMD, ctx);
#endif
	if (err) {
		XNVME_DEBUG("FAILED: ioctl_wrap(), err: %d", err);
		return err;
	}

	return 0;
}

int
xnvme_be_linux_nvme_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			      void *mbuf, size_t mbuf_nbytes)
{
	int err;

	ctx->cmd.common.dptr.lnx_ioctl.data = (uint64_t)dbuf;
	ctx->cmd.common.dptr.lnx_ioctl.data_len = dbuf_nbytes;

	ctx->cmd.common.mptr = (uint64_t)mbuf;
	ctx->cmd.common.dptr.lnx_ioctl.metadata_len = mbuf_nbytes;

	err = ioctl_wrap(ctx->dev, NVME_IOCTL_ADMIN_CMD, ctx);
	if (err) {
		XNVME_DEBUG("FAILED: ioctl_wrap() err: %d", err);
		return err;
	}

	return 0;
}

int
xnvme_be_linux_nvme_dev_nsid(struct xnvme_dev *dev)
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;

	return ioctl(state->fd, NVME_IOCTL_ID);
}

int
xnvme_be_linux_nvme_supported(struct xnvme_dev *dev, uint32_t XNVME_UNUSED(opts))
{
	int nsid = xnvme_be_linux_nvme_dev_nsid(dev);

	return nsid > 0;
}
#endif

struct xnvme_be_sync g_xnvme_be_linux_sync_nvme = {
	.id = "nvme",
#ifdef XNVME_BE_LINUX_ENABLED
	.cmd_io = xnvme_be_linux_nvme_cmd_io,
	.cmd_iov = xnvme_be_linux_nvme_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};

struct xnvme_be_admin g_xnvme_be_linux_admin_nvme = {
	.id = "nvme",
#ifdef XNVME_BE_LINUX_ENABLED
	.cmd_admin = xnvme_be_linux_nvme_cmd_admin,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
#endif
};
