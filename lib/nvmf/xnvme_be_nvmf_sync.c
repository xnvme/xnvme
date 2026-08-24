// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#ifdef XNVME_BE_NVMF_ENABLED
#include <errno.h>
#include <unistd.h>
#include <xnvme_dev.h>
#include <xnvme_be_nvmf.h>

int
xnvme_be_nvmf_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			  size_t mbuf_nbytes)
{
	return -ENOSYS;
}

int
xnvme_be_nvmf_sync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			   size_t XNVME_UNUSED(dvec_nbytes), void *mbuf, size_t mbuf_nbytes)
{
	return -ENOSYS;
}
#endif

struct xnvme_be_sync g_xnvme_be_nvmf_sync = {
	.id = "nvmf",
#ifdef XNVME_BE_NVMF_ENABLED
	.cmd_io = xnvme_be_nvmf_sync_cmd_io,
	.cmd_iov = xnvme_be_nvmf_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
