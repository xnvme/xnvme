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
#include <xnvme_be_state.h>

static int
xnvme_be_freebsd_nvme_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			    void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_freebsd_state *state = &xnvme_be_state(ctx->dev)->freebsd_state;
	struct nvme_pt_command ptc = {0};
	int err;

	memcpy(&ptc.cmd, &ctx->cmd, sizeof(ptc.cmd));
	ptc.buf = dbuf;
	ptc.len = dbuf_nbytes;
	ptc.is_read = (ptc.cmd.opc & 0x2) != 0;

	err = ioctl(state->ctrlr_fd, NVME_PASSTHROUGH_CMD, &ptc);
	if (err < 0) {
		XNVME_DEBUG("FAILED: ioctl() err: %d, errno: %d", err, errno);
		return err;
	}
	memcpy(&ctx->cpl, &ptc.cpl, sizeof(ctx->cpl));

	return 0;
}
#endif

struct xnvme_be_admin g_xnvme_be_freebsd_admin_nvme = {
	.id = "nvme",
#ifdef XNVME_PLATFORM_FREEBSD_ENABLED
	.cmd_admin = xnvme_be_freebsd_nvme_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#endif
};
