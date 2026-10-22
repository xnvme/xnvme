// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#include <errno.h>

#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_debug.h>
#include <xnvme_be_nvmf_mem.h>
#include <xnvme_be_nvmf_qpair.h>

#define NVMF_DEBUG_CATEGORY NVMF_DEBUG_CATEGORY_CMD_IO

int
xnvme_be_nvmf_queue_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	return -ENOSYS;
}

int
xnvme_be_nvmf_queue_term(struct xnvme_queue *q)
{
	return -ENOSYS;
}

int
xnvme_be_nvmf_queue_poke(struct xnvme_queue *q, uint32_t max)
{
	return -ENOSYS;
}

xnvme_be_nvmf_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			   void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	return -ENOSYS;
}

int
xnvme_be_nvmf_async_cmd_iov(struct xnvme_cmd_ctx *XNVME_UNUSED(ctx),
			    struct iovec *XNVME_UNUSED(dvec), size_t XNVME_UNUSED(dvec_cnt),
			    size_t XNVME_UNUSED(dvec_nbytes), void *XNVME_UNUSED(mbuf),
			    size_t XNVME_UNUSED(mbuf_nbytes))
{
	return -ENOSYS;
}

struct xnvme_be_async g_xnvme_be_nvmf_async = {
	.id = "nvmf",
	.cmd_io = xnvme_be_nvmf_async_cmd_io,
	.cmd_iov = xnvme_be_nvmf_async_cmd_iov,
	.poke = xnvme_be_nvmf_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nvmf_queue_init,
	.term = xnvme_be_nvmf_queue_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
};
