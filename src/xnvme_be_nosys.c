// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

int
xnvme_be_nosys_enumerate(struct xnvme_enumeration *XNVME_UNUSED(list),
			 const char *XNVME_UNUSED(sys_uri),
			 int XNVME_UNUSED(opts))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_dev_from_ident(const struct xnvme_ident *XNVME_UNUSED(ident),
			      struct xnvme_dev **XNVME_UNUSED(dev))
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

int
xnvme_be_nosys_async_init(struct xnvme_dev *XNVME_UNUSED(dev),
			  struct xnvme_async_ctx **XNVME_UNUSED(ctx),
			  uint16_t XNVME_UNUSED(depth), int XNVME_UNUSED(flags))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_async_term(struct xnvme_dev *XNVME_UNUSED(dev),
			  struct xnvme_async_ctx *XNVME_UNUSED(ctx))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_async_poke(struct xnvme_dev *XNVME_UNUSED(dev),
			  struct xnvme_async_ctx *XNVME_UNUSED(ctx),
			  uint32_t XNVME_UNUSED(max))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

int
xnvme_be_nosys_async_wait(struct xnvme_dev *XNVME_UNUSED(dev),
			  struct xnvme_async_ctx *XNVME_UNUSED(ctx))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

void *
xnvme_be_nosys_buf_alloc(const struct xnvme_dev *XNVME_UNUSED(dev),
			 size_t XNVME_UNUSED(nbytes),
			 uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	errno = ENOSYS;
	return NULL;
}

void *
xnvme_be_nosys_buf_realloc(const struct xnvme_dev *XNVME_UNUSED(dev),
			   void *XNVME_UNUSED(buf), size_t XNVME_UNUSED(nbytes),
			   uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	errno = ENOSYS;
	return NULL;
}

void
xnvme_be_nosys_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev),
			void *XNVME_UNUSED(buf))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
}

int
xnvme_be_nosys_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev),
			   void *XNVME_UNUSED(buf),
			   uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
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
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
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
	XNVME_DEBUG("FAILED: not implemented(possibly intentional)");
	return -ENOSYS;
}

static const char *g_schemes[] = {
	"nosys",
};

#define XNVME_BE_NOSYS_ATTR {					\
	.name = "nosys",					\
	.enabled = 1,						\
	.schemes = g_schemes,					\
	.nschemes = sizeof g_schemes / sizeof(*g_schemes),	\
}

struct xnvme_be xnvme_be_nosys = {
	.func = XNVME_BE_NOSYS_FUNC,
	.attr = XNVME_BE_NOSYS_ATTR,
	.state = { 0 },
};
