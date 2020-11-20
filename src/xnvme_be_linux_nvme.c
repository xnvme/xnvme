// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifdef XNVME_BE_LINUX_ENABLED
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <linux/nvme_ioctl.h>
#include <sys/ioctl.h>
#include <errno.h>
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
	case NVME_IOCTL_SUBMIT_IO:
		return "NVME_IOCTL_SUBMIT_IO";
	case NVME_IOCTL_IO_CMD:
		return "NVME_IOCTL_IO_CMD";
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
xnvme_be_linux_nvme_map_cpl(struct xnvme_cmd_ctx *ctx, unsigned long ioctl_req)
{
	struct _kernel_cpl *kcpl = (void *)&ctx->cpl;

	// Assign the completion-result
	switch (ioctl_req) {
	case NVME_IOCTL_ADMIN_CMD:
	case NVME_IOCTL_IO_CMD:
		ctx->cpl.result = kcpl->res32.result;
		break;
#ifdef NVME_IOCTL_IO64
	case NVME_IOCTL_IO64_CMD:
		ctx->cpl.result = kcpl->res64.result;
		break;
#endif
#ifdef NVME_IOCTL_ADMIN64
	case NVME_IOCTL_ADMIN64_CMD:
		ctx->cpl.result = kcpl->res64.result;
		break;
#endif
	default:
		return -1;
	}

	// Zero-out the remainder of the completion
	ctx->cpl.status.val = 0;
	ctx->cpl.sqhd = 0;
	ctx->cpl.sqid = 0;
	ctx->cpl.cid = 0;

	return 0;
}

static inline int
ioctl_wrap(struct xnvme_dev *dev, unsigned long ioctl_req, struct xnvme_cmd_ctx *ctx)
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;
	int err;

	err = ioctl(state->fd, ioctl_req, ctx);
	if (xnvme_be_linux_nvme_map_cpl(ctx, ioctl_req)) {
		return -ENOSYS;
	}

	if (!err) {
		return 0;
	}

	// Transform ioctl-errors to completion status-codes
	XNVME_DEBUG("FAILED: ioctl(%s), err(%d), errno(%d)", ioctl_request_to_str(ioctl_req),
		    err, errno);

	// Transform ioctl EINVAL to Invalid Field in Command
	if (err == -1 && errno == EINVAL) {
		ctx->cpl.status.val = 0x2;
	}
	if (!errno) {
		XNVME_DEBUG("INFO: !errno, setting errno=EIO");
		errno = EIO;
	}

	return -errno;
}

int
xnvme_be_linux_nvme_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t mbuf_nbytes)
{
	int err;

	ctx->cmd.common.dptr.lnx_ioctl.data = (uint64_t)dbuf;
	ctx->cmd.common.dptr.lnx_ioctl.data_len = dbuf_nbytes;

	ctx->cmd.common.mptr = (uint64_t)mbuf;
	ctx->cmd.common.dptr.lnx_ioctl.metadata_len = mbuf_nbytes;

	err = ioctl_wrap(ctx->dev, NVME_IOCTL_IO_CMD, ctx);
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

struct xnvme_be_sync g_linux_nvme = {
	.cmd_io = xnvme_be_linux_nvme_cmd_io,
	.cmd_admin = xnvme_be_linux_nvme_cmd_admin,
	.id = "nvme_ioctl",
	.enabled = 1,
	.supported = xnvme_be_linux_nvme_supported,
};

#endif

