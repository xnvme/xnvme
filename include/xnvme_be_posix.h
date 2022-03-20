// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_POSIX_H
#define __INTERNAL_XNVME_BE_POSIX_H
struct xnvme_be_posix_state {
	int fd;

	uint8_t _rsvd[124];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_posix_state) == XNVME_BE_STATE_NBYTES,
		    "Incorrect size");

int
xnvme_be_posix_supported(struct xnvme_dev *dev, uint32_t opts);

int
xnvme_be_posix_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			   void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes));

extern struct xnvme_be_admin g_xnvme_be_posix_admin_shim;
extern struct xnvme_be_sync g_xnvme_be_posix_sync_psync;
extern struct xnvme_be_async g_xnvme_be_posix_async_nil;
extern struct xnvme_be_async g_xnvme_be_posix_async_emu;
extern struct xnvme_be_async g_xnvme_be_posix_async_aio;
extern struct xnvme_be_async g_xnvme_be_posix_async_thrpool;
extern struct xnvme_be_mem g_xnvme_be_posix_mem;
extern struct xnvme_be_dev g_xnvme_be_posix_dev;

#endif /* __INTERNAL_XNVME_BE_POSIX_H */
