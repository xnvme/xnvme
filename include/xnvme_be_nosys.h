// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_NOSYS_H
#define __INTERNAL_XNVME_BE_NOSYS_H

extern struct xnvme_be xnvme_be_nosys;

int
xnvme_be_nosys_enumerate(struct xnvme_enumeration *list, int opts);

int
xnvme_be_nosys_dev_from_ident(const struct xnvme_ident *ident,
			      struct xnvme_dev **dev);

void
xnvme_be_nosys_dev_close(struct xnvme_dev *dev);

void *
xnvme_be_nosys_buf_alloc(const struct xnvme_dev *dev, size_t nbytes,
			 uint64_t *phys);

int
xnvme_be_nosys_buf_vtophys(const struct xnvme_dev *dev, void *buf,
			   uint64_t *phys);

void *
xnvme_be_nosys_buf_realloc(const struct xnvme_dev *dev, void *buf,
			   size_t nbytes, uint64_t *phys);

void
xnvme_be_nosys_buf_free(const struct xnvme_dev *dev, void *buf);

int
xnvme_be_nosys_async_init(struct xnvme_dev *dev, struct xnvme_async_ctx **ctx,
			  uint16_t depth, int flags);

int
xnvme_be_nosys_async_term(struct xnvme_async_ctx *ctx);

int
xnvme_be_nosys_async_poke(struct xnvme_async_ctx *ctx,
			  uint32_t max);

int
xnvme_be_nosys_async_wait(struct xnvme_async_ctx *ctx);

int
xnvme_be_nosys_cmd_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
			void *dbuf, size_t dbuf_nbytes, void *mbuf,
			size_t mbuf_nbytes, int opts, struct xnvme_req *req);

int
xnvme_be_nosys_cmd_pass_admin(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
			      void *dbuf, size_t dbuf_nbytes, void *mbuf,
			      size_t mbuf_nbytes, int opts,
			      struct xnvme_req *req);

#define XNVME_BE_NOSYS_FUNC {					\
	.cmd_pass = xnvme_be_nosys_cmd_pass,			\
	.cmd_pass_admin = xnvme_be_nosys_cmd_pass_admin,	\
								\
	.async_init = xnvme_be_nosys_async_init,		\
	.async_term = xnvme_be_nosys_async_term,		\
	.async_poke = xnvme_be_nosys_async_poke,		\
	.async_wait = xnvme_be_nosys_async_wait,		\
								\
	.buf_alloc = xnvme_be_nosys_buf_alloc,			\
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,		\
	.buf_realloc = xnvme_be_nosys_buf_realloc,		\
	.buf_free = xnvme_be_nosys_buf_free,			\
								\
	.enumerate = xnvme_be_nosys_enumerate,			\
								\
	.dev_from_ident = xnvme_be_nosys_dev_from_ident,	\
	.dev_close = xnvme_be_nosys_dev_close,			\
}

#endif /* __INTERNAL_XNVME_BE_NOSYS_H */
