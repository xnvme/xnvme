// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#include <errno.h>
#include <unistd.h>
#include <xnvme_dev.h>

#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_debug.h>
#include <xnvme_be_nvmf_mem.h>
#include <xnvme_be_nvmf_ns.h>
#include <xnvme_be_nvmf_qpair.h>

static int
xnvme_be_nvmf_admin_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			      void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	return xnvme_be_nvmf_cmd_sync(TO_NVMF_DEV_STATE(ctx->dev)->admin_qpair, ctx, dbuf,
				      dbuf_nbytes);
}

struct xnvme_be_admin g_xnvme_be_nvmf_admin = {
	.id = "nvmf",
	.cmd_admin = xnvme_be_nvmf_admin_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
};
