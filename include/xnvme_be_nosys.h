// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_NOSYS_H
#define __INTERNAL_XNVME_BE_NOSYS_H

int
xnvme_be_nosys_ident_from_uri(const char *uri, struct xnvme_ident *ident);

int
xnvme_be_nosys_enumerate(struct xnvme_enumeration *list, int opts);

struct xnvme_dev *
xnvme_be_nosys_dev_open(const struct xnvme_ident *ident, int opts);

void
xnvme_be_nosys_dev_close(struct xnvme_dev *dev);

void *
xnvme_be_nosys_buf_alloc(const struct xnvme_dev *dev, size_t nbytes,
			 uint64_t *phys);

void *
xnvme_be_nosys_buf_realloc(const struct xnvme_dev *dev, void *buf,
			   size_t nbytes, uint64_t *phys);

void
xnvme_be_nosys_buf_free(const struct xnvme_dev *dev, void *buf);

int
xnvme_be_nosys_buf_vtophys(const struct xnvme_dev *dev, void *buf,
			   uint64_t *phys);

struct xnvme_async_ctx *
xnvme_be_nosys_async_init(struct xnvme_dev *dev, uint32_t depth,
			  uint16_t flags);

int
xnvme_be_nosys_async_term(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx);

int
xnvme_be_nosys_async_poke(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx,
			  uint32_t max);

int
xnvme_be_nosys_async_wait(struct xnvme_dev *dev, struct xnvme_async_ctx *ctx);

int
xnvme_be_nosys_cmd_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
			void *dbuf, size_t dbuf_nbytes, void *mbuf,
			size_t mbuf_nbytes, int opts, struct xnvme_req *req);

int
xnvme_be_nosys_cmd_pass_admin(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
			      void *dbuf, size_t dbuf_nbytes, void *mbuf,
			      size_t mbuf_nbytes, int opts,
			      struct xnvme_req *req);

#endif /* __INTERNAL_XNVME_BE_NOSYS_H */
