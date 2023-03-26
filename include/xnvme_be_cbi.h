// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_CBI_H
#define __INTERNAL_XNVME_BE_CBI_H
struct xnvme_be_cbi_state {
	int fd;

	uint8_t _rsvd[124];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_cbi_state) == XNVME_BE_STATE_NBYTES, "Incorrect size");

extern struct xnvme_be_admin g_xnvme_be_cbi_admin_shim;
extern struct xnvme_be_async g_xnvme_be_cbi_async_emu;
extern struct xnvme_be_async g_xnvme_be_cbi_async_nil;
extern struct xnvme_be_async g_xnvme_be_cbi_async_posix;
extern struct xnvme_be_async g_xnvme_be_cbi_async_thrpool;
extern struct xnvme_be_mem g_xnvme_be_cbi_mem_posix;
extern struct xnvme_be_sync g_xnvme_be_cbi_sync_psync;

int
xnvme_be_cbi_sync_psync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			       void *mbuf, size_t mbuf_nbytes);

int
xnvme_be_cbi_sync_psync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
				size_t dvec_nbytes, struct iovec *mvec, size_t mvec_cnt,
				size_t mvec_nbytes);

#endif /* __INTERNAL_XNVME_BE_CBI_H */
