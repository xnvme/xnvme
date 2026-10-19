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

static int
_controller_get_registers(const struct xnvme_dev *dev, void *dbuf, size_t dbuf_nbytes)
{
	const struct xnvme_ident *ident = &dev->ident;
	char path[512] = {0};
	void *membase;
	int err = 0;
	int fd;

	switch (ident->dtype) {
	case XNVME_DEV_TYPE_NVME_NAMESPACE:
		snprintf(path, sizeof(path), "/sys/block/%s/device/device/resource0",
			 basename(ident->uri));
		break;

	case XNVME_DEV_TYPE_NVME_CONTROLLER:
		snprintf(path, sizeof(path), "/sys/class/nvme/%s/device/resource0",
			 basename(ident->uri));
		break;

	default:
		XNVME_DEBUG("FAILED: dev is not an NVMe controller nor namespace");
		return -EINVAL;
	}

	fd = open(path, O_RDONLY | O_SYNC);
	if (fd < 0) {
		err = -errno;
		XNVME_DEBUG("FAILED: open('%s') got err: %d", path, err);
		return err;
	}

	membase = mmap(NULL, getpagesize(), PROT_READ, MAP_SHARED, fd, 0);
	if (membase == MAP_FAILED) {
		err = -errno;

		XNVME_DEBUG("FAILED: mmap(), got err: %d", err);
		goto exit;
	}
	memcpy(dbuf, membase, dbuf_nbytes);
	munmap(membase, getpagesize());

exit:
	close(fd);

	return err;
}

static int
xnvme_be_linux_nvme_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			      void *mbuf, size_t mbuf_nbytes)
{
	int err;

	ctx->cmd.common.dptr.lnx_ioctl.data = (uint64_t)dbuf;
	ctx->cmd.common.dptr.lnx_ioctl.data_len = dbuf_nbytes;

	ctx->cmd.common.mptr = (uint64_t)mbuf;
	ctx->cmd.common.dptr.lnx_ioctl.metadata_len = mbuf_nbytes;

	err = xnvme_be_linux_nvme_ioctl(ctx->dev, NVME_IOCTL_ADMIN_CMD, ctx);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_linux_nvme_ioctl() err: %d", err);
		return err;
	}

	return 0;
}

static int
xnvme_be_linux_nvme_cmd_pseudo(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			       void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_linux_state *state = (void *)ctx->dev->be.state;
	int ioctl_req;
	int err;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_PSEUDO_OPC_SHOW_REGS:
		if (dbuf_nbytes != sizeof(struct xnvme_spec_ctrlr_bar)) {
			XNVME_DEBUG(
				"FAILED: dbuf_nbytes(%zu) != sizeof(struct xnvme_spec_ctrlr_bar)",
				dbuf_nbytes);
			return -EINVAL;
		}
		return _controller_get_registers(ctx->dev, dbuf, dbuf_nbytes);

	case XNVME_SPEC_PSEUDO_OPC_CONTROLLER_RESET:
		ioctl_req = NVME_IOCTL_RESET;
		break;

	case XNVME_SPEC_PSEUDO_OPC_SUBSYSTEM_RESET:
		ioctl_req = NVME_IOCTL_SUBSYS_RESET;
		break;

	case XNVME_SPEC_PSEUDO_OPC_NAMESPACE_RESCAN:
		ioctl_req = NVME_IOCTL_RESCAN;
		break;
	default:
		XNVME_DEBUG("FAILED: unsupported opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	err = ioctl(state->fd, ioctl_req);
	if (err < 0) {
		XNVME_DEBUG("FAILED: ioctl() err: %d", err);
		return err;
	}

	return 0;
}
#endif

struct xnvme_be_admin g_xnvme_be_linux_admin_nvme = {
	.id = "nvme",
#ifdef XNVME_PLATFORM_LINUX_ENABLED
	.cmd_admin = xnvme_be_linux_nvme_cmd_admin,
	.cmd_pseudo = xnvme_be_linux_nvme_cmd_pseudo,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#endif
};
