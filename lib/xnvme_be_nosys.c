// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

int
xnvme_be_nosys_sync_cmd_io(struct xnvme_cmd_ctx *XNVME_UNUSED(ctx), void *XNVME_UNUSED(dbuf),
			   size_t XNVME_UNUSED(dbuf_nbytes), void *XNVME_UNUSED(mbuf),
			   size_t XNVME_UNUSED(mbuf_nbytes))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_sync_cmd_iov(struct xnvme_cmd_ctx *XNVME_UNUSED(ctx),
			    struct iovec *XNVME_UNUSED(dvec), size_t XNVME_UNUSED(dvec_cnt),
			    size_t XNVME_UNUSED(dvec_nbytes), struct iovec *XNVME_UNUSED(mvec),
			    size_t XNVME_UNUSED(mvec_cnt), size_t XNVME_UNUSED(mvec_nbytes))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_sync_cmd_admin(struct xnvme_cmd_ctx *XNVME_UNUSED(ctx), void *XNVME_UNUSED(dbuf),
			      size_t XNVME_UNUSED(dbuf_nbytes), void *XNVME_UNUSED(mbuf),
			      size_t XNVME_UNUSED(mbuf_nbytes))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_sync_supported(struct xnvme_dev *XNVME_UNUSED(dev), uint32_t XNVME_UNUSED(opts))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_queue_supported(struct xnvme_dev *XNVME_UNUSED(dev), uint32_t XNVME_UNUSED(opts))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_queue_cmd_io(struct xnvme_cmd_ctx *XNVME_UNUSED(ctx), void *XNVME_UNUSED(dbuf),
			    size_t XNVME_UNUSED(dbuf_nbytes), void *XNVME_UNUSED(mbuf),
			    size_t XNVME_UNUSED(mbuf_nbytes))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_queue_cmd_iov(struct xnvme_cmd_ctx *XNVME_UNUSED(ctx),
			     struct iovec *XNVME_UNUSED(dvec), size_t XNVME_UNUSED(dvec_cnt),
			     size_t XNVME_UNUSED(dvec_nbytes), struct iovec *XNVME_UNUSED(mvec),
			     size_t XNVME_UNUSED(mvec_cnt), size_t XNVME_UNUSED(mvec_nbytes))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_queue_poke(struct xnvme_queue *XNVME_UNUSED(queue), uint32_t XNVME_UNUSED(max))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_queue_wait(struct xnvme_queue *XNVME_UNUSED(queue))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_queue_init(struct xnvme_queue *XNVME_UNUSED(queue), int XNVME_UNUSED(opts))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_queue_term(struct xnvme_queue *XNVME_UNUSED(queue))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

void *
xnvme_be_nosys_buf_alloc(const struct xnvme_dev *XNVME_UNUSED(dev), size_t XNVME_UNUSED(nbytes),
			 uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	errno = ENOSYS;
	return NULL;
}

void *
xnvme_be_nosys_buf_realloc(const struct xnvme_dev *XNVME_UNUSED(dev), void *XNVME_UNUSED(buf),
			   size_t XNVME_UNUSED(nbytes), uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	errno = ENOSYS;
	return NULL;
}

void
xnvme_be_nosys_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *XNVME_UNUSED(buf))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
}

int
xnvme_be_nosys_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *XNVME_UNUSED(buf),
			   uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_enumerate(const char *XNVME_UNUSED(sys_uri), struct xnvme_opts *XNVME_UNUSED(opts),
			 xnvme_enumerate_cb XNVME_UNUSED(cb_func), void *XNVME_UNUSED(cb_args))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_dev_open(struct xnvme_dev *XNVME_UNUSED(dev))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

void
xnvme_be_nosys_dev_close(struct xnvme_dev *XNVME_UNUSED(dev))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	errno = ENOSYS;
	return;
}

#define XNVME_BE_NOSYS_ATTR                    \
	{                                      \
		.name = "nosys", .enabled = 1, \
	}

struct xnvme_be xnvme_be_nosys = {
	.sync = XNVME_BE_NOSYS_SYNC,
	.async = XNVME_BE_NOSYS_QUEUE,
	.mem = XNVME_BE_NOSYS_MEM,
	.dev = XNVME_BE_NOSYS_DEV,
	.attr = XNVME_BE_NOSYS_ATTR,
	.state = {0},
};
