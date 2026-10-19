// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_POSIX_H
#define __INTERNAL_XNVME_BE_POSIX_H

/**
 * The POSIX interfaces of Linux, FreeBSD and macOS
 *
 * The async is POSIX AIO, aio_read() and friends; the sync is pread(), pwrite()
 * and fsync(); the memory is page-aligned by sysconf(_SC_PAGESIZE). The one
 * part that is not POSIX is the vectored sync, preadv() and pwritev(): a BSD
 * extension that Linux, FreeBSD and macOS all provide, so a platform with POSIX
 * alone would not build xnvme_be_posix_sync.c.
 */
struct xnvme_be_posix_state {
	int fd;

	uint8_t _rsvd[124];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_posix_state) == XNVME_BE_STATE_NBYTES,
		    "Incorrect size");

extern struct xnvme_be_admin g_xnvme_be_shim_admin;
extern struct xnvme_be_async g_xnvme_be_emu_async;
extern struct xnvme_be_async g_xnvme_be_nil_async;
extern struct xnvme_be_async g_xnvme_be_posix_async;
extern struct xnvme_be_async g_xnvme_be_thrpool_async;
extern struct xnvme_be_mem g_xnvme_be_posix_mem;
extern struct xnvme_be_sync g_xnvme_be_posix_sync;

int
xnvme_be_posix_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t mbuf_nbytes);

int
xnvme_be_posix_sync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			    size_t dvec_nbytes, void *mbuf, size_t mbuf_nbytes);

#endif /* __INTERNAL_XNVME_BE_POSIX_H */
