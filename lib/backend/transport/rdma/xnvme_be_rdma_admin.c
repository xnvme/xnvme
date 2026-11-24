// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <errno.h>
#include <unistd.h>

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#include <xnvme_dev.h>

#include <xnvme_transport_mem.h>
#include <xnvme_transport_debug.h>
#include <xnvme_transport_ctrlr.h>
#include <xnvme_transport_qpair.h>
#include <xnvme_transport_ns.h>

#include <xnvme_be_rdma.h>

static int
xnvme_be_rdma_admin_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			      void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_be_rdma_state *state = TO_TRANSPORT_DEV_STATE(ctx->dev);
	struct xnvme_transport_qpair *admin_qpair = state->admin_qpair;
	int err;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: metadata buffers are not supported");
		return -ENOSYS;
	}

	// Admin queues can be shared between multiple xNVMe devices, so we lock the qpair during
	// the command
	pthread_mutex_lock(&admin_qpair->lock);
	err = xnvme_transport_cmd_sync(admin_qpair, ctx, dbuf, dbuf_nbytes);
	pthread_mutex_unlock(&admin_qpair->lock);

	return err;
}

struct xnvme_be_admin g_xnvme_be_rdma_admin = {
	.id = "rdma",
	.cmd_admin = xnvme_be_rdma_admin_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
};
