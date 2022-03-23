// Copyright (C) Mads Ynddal <m.ynddal@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_PSYNC_H
#define __INTERNAL_XNVME_BE_PSYNC_H

int
_xnvme_be_psync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
		       void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes), int fd);
int
_xnvme_be_psync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			size_t XNVME_UNUSED(dvec_nbytes), struct iovec *XNVME_UNUSED(mvec),
			size_t XNVME_UNUSED(mvec_cnt), size_t XNVME_UNUSED(mvec_nbytes), int fd);

#endif /* __INTERNAL_XNVME_BE_PSYNC_H */
