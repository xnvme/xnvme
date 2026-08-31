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

int
xnvme_be_nvmf_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			  size_t mbuf_nbytes)
{
	struct xnvme_be_nvmf_state *state = ctx->dev->be.state;
	struct xnvme_be_nvmf_ctrlr *ctrlr = state->ctrlr;
	struct xnvme_be_nvmf_qpair *qpair = ctrlr->sync_qpair;
	int err;

	pthread_mutex_lock(&ctrlr->lock);
	err = -ENOSYS;
	pthread_mutex_unlock(&ctrlr->lock);

	return err;
}

int
xnvme_be_nvmf_sync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			   size_t XNVME_UNUSED(dvec_nbytes), void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_be_nvmf_state *state = ctx->dev->be.state;
	struct xnvme_be_nvmf_ctrlr *ctrlr = state->ctrlr;
	struct xnvme_be_nvmf_qpair *qpair = ctrlr->sync_qpair;
	int err;

	pthread_mutex_lock(&ctrlr->lock);
	err = -ENOSYS;
	pthread_mutex_unlock(&ctrlr->lock);

	return err;
}

struct xnvme_be_sync g_xnvme_be_nvmf_sync = {
	.id = "nvmf",
	.cmd_io = xnvme_be_nvmf_sync_cmd_io,
	.cmd_iov = xnvme_be_nvmf_sync_cmd_iov,
};
