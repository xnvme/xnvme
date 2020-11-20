// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_NOSYS_H
#define __INTERNAL_XNVME_BE_NOSYS_H

extern struct xnvme_be xnvme_be_nosys;

int
xnvme_be_nosys_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t mbuf_nbytes);

int
xnvme_be_nosys_sync_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			      void *mbuf, size_t mbuf_nbytes);

int
xnvme_be_nosys_sync_supported(struct xnvme_dev *dev, uint32_t opts);

int
xnvme_be_nosys_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			    size_t mbuf_nbytes);

int
xnvme_be_nosys_queue_poke(struct xnvme_queue *queue, uint32_t max);

int
xnvme_be_nosys_queue_wait(struct xnvme_queue *queue);

int
xnvme_be_nosys_queue_init(struct xnvme_queue *queue, int opts);

int
xnvme_be_nosys_queue_term(struct xnvme_queue *queue);

int
xnvme_be_nosys_async_supported(struct xnvme_dev *dev, uint32_t opts);

void *
xnvme_be_nosys_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys);

int
xnvme_be_nosys_buf_vtophys(const struct xnvme_dev *dev, void *buf, uint64_t *phys);

void *
xnvme_be_nosys_buf_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes, uint64_t *phys);

void
xnvme_be_nosys_buf_free(const struct xnvme_dev *dev, void *buf);

int
xnvme_be_nosys_enumerate(struct xnvme_enumeration *list, const char *sys_uri, int opts);

int
xnvme_be_nosys_dev_from_ident(const struct xnvme_ident *ident, struct xnvme_dev **dev);

void
xnvme_be_nosys_dev_close(struct xnvme_dev *dev);

#define XNVME_BE_NOSYS_SYNC {					\
	.cmd_io = xnvme_be_nosys_sync_cmd_io,			\
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,		\
	.supported = xnvme_be_nosys_sync_supported,		\
	.id = "ENOSYS",						\
	.enabled = 0,						\
}

#define XNVME_BE_NOSYS_QUEUE {					\
	.cmd_io = xnvme_be_nosys_async_cmd_io,			\
	.poke = xnvme_be_nosys_queue_poke,			\
	.wait = xnvme_be_nosys_queue_wait,			\
	.init = xnvme_be_nosys_queue_init,			\
	.term = xnvme_be_nosys_queue_term,			\
	.supported = xnvme_be_nosys_async_supported,		\
	.id = "ENOSYS",						\
	.enabled = 0,						\
}

#define XNVME_BE_NOSYS_MEM {					\
	.buf_alloc = xnvme_be_nosys_buf_alloc,			\
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,		\
	.buf_realloc = xnvme_be_nosys_buf_realloc,		\
	.buf_free = xnvme_be_nosys_buf_free,			\
}

#define XNVME_BE_NOSYS_DEV {					\
	.enumerate = xnvme_be_nosys_enumerate,			\
	.dev_from_ident = xnvme_be_nosys_dev_from_ident,	\
	.dev_close = xnvme_be_nosys_dev_close,			\
}

#endif /* __INTERNAL_XNVME_BE_NOSYS_H */
