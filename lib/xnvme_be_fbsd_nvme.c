// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_FBSD_ENABLED
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
#include <libxnvme_spec_fs.h>
#include <xnvme_dev.h>
#include <xnvme_be_fbsd.h>

XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd) == sizeof(struct nvme_command), "Incorrect size")
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cpl) == sizeof(struct nvme_completion),
		    "Incorrect size")

int
xnvme_be_fbsd_nvme_get_nsid_and_ctrlr_fd(int fd, uint32_t *nsid, int *ctrlr_fd)
{
	struct nvme_get_nsid get_nsid = {0};
	char ctrlr_path[1024] = {0};
	int nfd;
	int err;

	err = ioctl(fd, NVME_GET_NSID, &get_nsid);
	if (err) {
		return err < 0 ? err : -EIO;
	}

	snprintf(ctrlr_path, sizeof(ctrlr_path), _PATH_DEV "%s", get_nsid.cdev);

	*nsid = get_nsid.nsid;
	nfd = open(ctrlr_path, O_RDWR);
	if (nfd < 0) {
		XNVME_DEBUG("FAILED: open(%s), errno: %d", ctrlr_path, errno);
		return -errno;
	}

	*ctrlr_fd = nfd;

	return 0;
}

int
xnvme_be_fbsd_nvme_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
		      void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_fbsd_state *state = (void *)ctx->dev->be.state;
	struct nvme_pt_command ptc = {0};
	int err;

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

int
xnvme_be_fbsd_nvme_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			 void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_fbsd_state *state = (void *)ctx->dev->be.state;
	struct nvme_pt_command ptc = {0};
	int err;

	memcpy(&ptc.cmd, &ctx->cmd, sizeof(ptc.cmd));
	ptc.buf = dbuf;
	ptc.len = dbuf_nbytes;
	ptc.is_read = (ptc.cmd.opc & 0x2) != 0;

	err = ioctl(state->fd.ctrlr, NVME_PASSTHROUGH_CMD, &ptc);
	if (err < 0) {
		XNVME_DEBUG("FAILED: ioctl() err: %d, errno: %d", err, errno);
		return err;
	}
	memcpy(&ctx->cpl, &ptc.cpl, sizeof(ctx->cpl));

	return 0;
}

#endif

struct xnvme_be_sync g_xnvme_be_fbsd_sync_nvme = {
	.id = "nvme",
#ifdef XNVME_BE_FBSD_ENABLED
	.cmd_io = xnvme_be_fbsd_nvme_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};

struct xnvme_be_admin g_xnvme_be_fbsd_admin_nvme = {
	.id = "nvme",
#ifdef XNVME_BE_FBSD_ENABLED
	.cmd_admin = xnvme_be_fbsd_nvme_admin,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
#endif
};
