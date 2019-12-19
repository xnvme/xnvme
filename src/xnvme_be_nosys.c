// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <libxnvme.h>

#define XNVME_BE_NOSYS_BID 0x3838
#define XNVME_BE_NOSYS_NAME "NOSYS"

int
xnvme_be_nosys_ident_from_uri(const char *XNVME_UNUSED(uri),
			      struct xnvme_ident *XNVME_UNUSED(ident))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return -1;
}

int
xnvme_be_nosys_enumerate(struct xnvme_enumeration *XNVME_UNUSED(list),
			 int XNVME_UNUSED(opts))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return -1;
}

struct xnvme_dev *
xnvme_be_nosys_dev_open(const struct xnvme_ident *XNVME_UNUSED(ident),
			int XNVME_UNUSED(opts))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return NULL;
}

void
xnvme_be_nosys_dev_close(struct xnvme_dev *XNVME_UNUSED(dev))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return;
}

struct xnvme_async_ctx *
xnvme_be_nosys_async_init(struct xnvme_dev *XNVME_UNUSED(dev),
			  uint32_t XNVME_UNUSED(depth),
			  uint16_t XNVME_UNUSED(flags))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return NULL;
}

int
xnvme_be_nosys_async_term(struct xnvme_dev *XNVME_UNUSED(dev),
			  struct xnvme_async_ctx *XNVME_UNUSED(ctx))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return -1;
}

int
xnvme_be_nosys_async_poke(struct xnvme_dev *XNVME_UNUSED(dev),
			  struct xnvme_async_ctx *XNVME_UNUSED(ctx),
			  uint32_t XNVME_UNUSED(max))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return -1;
}

int
xnvme_be_nosys_async_wait(struct xnvme_dev *XNVME_UNUSED(dev),
			  struct xnvme_async_ctx *XNVME_UNUSED(ctx))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return -1;
}

void *
xnvme_be_nosys_buf_alloc(const struct xnvme_dev *XNVME_UNUSED(dev),
			 size_t XNVME_UNUSED(nbytes),
			 uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return NULL;
}

void *
xnvme_be_nosys_buf_realloc(const struct xnvme_dev *XNVME_UNUSED(dev),
			   void *XNVME_UNUSED(buf), size_t XNVME_UNUSED(nbytes),
			   uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return NULL;
}

void
xnvme_be_nosys_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev),
			void *XNVME_UNUSED(buf))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentionally)");
}

int
xnvme_be_nosys_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev),
			   void *XNVME_UNUSED(buf),
			   uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return -1;
}

int
xnvme_be_nosys_cmd_pass(struct xnvme_dev *XNVME_UNUSED(dev),
			struct xnvme_spec_cmd *XNVME_UNUSED(cmd),
			void *XNVME_UNUSED(dbuf),
			size_t XNVME_UNUSED(dbuf_nbytes),
			void *XNVME_UNUSED(mbuf),
			size_t XNVME_UNUSED(mbuf_nbytes),
			int XNVME_UNUSED(flags),
			struct xnvme_req *XNVME_UNUSED(req))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return -1;
}

int
xnvme_be_nosys_cmd_pass_admin(struct xnvme_dev *XNVME_UNUSED(dev),
			      struct xnvme_spec_cmd *XNVME_UNUSED(cmd),
			      void *XNVME_UNUSED(dbuf),
			      size_t XNVME_UNUSED(dbuf_nbytes),
			      void *XNVME_UNUSED(mbuf),
			      size_t XNVME_UNUSED(mbuf_nbytes),
			      int XNVME_UNUSED(flags),
			      struct xnvme_req *XNVME_UNUSED(req))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return -1;
}
