// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <libxnvme.h>
#include <xnvme_cmd.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
#include <errno.h>
#include <xnvme_dev.h>
#include <xnvme_be_windows.h>
#include <xnvme_be_state.h>
#include <windows.h>

/**
 * Pass a file IO Command in Windows
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of data-payload in bytes
 * @param mbuf pointer to meta-payload
 * @param mbuf_nbytes size of meta-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
static int
xnvme_be_windows_sync_fs_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
				void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	DWORD res = 0;
	int err = 0;
	uint64_t slba = 0;
	OVERLAPPED overlapped;
	struct xnvme_be_windows_state *state = &xnvme_be_state(ctx->dev)->windows_state;
	const uint64_t ssw = ctx->dev->geo.ssw;

	memset(&overlapped, 0, sizeof(OVERLAPPED));

	switch (xnvme_cmd_sqe_get_opc(&ctx->cmd)) {
	case XNVME_SPEC_NVM_WRITE_OPCODE:
		slba = xnvme_cmd_sqe_get_cdw10_11(&ctx->cmd) << ssw;
		overlapped.Offset = slba;
		overlapped.OffsetHigh = (slba >> 32);
		if (!WriteFile(state->sync_handle, dbuf, dbuf_nbytes, &res, &overlapped)) {
			if (res != (ssize_t)dbuf_nbytes) {
				err = GetLastError();
				XNVME_DEBUG("FAILED: W res: %ld != dbuf_nbytes: %zu, errno: %d",
					    res, dbuf_nbytes, err);
			}
		}
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		overlapped.Offset = xnvme_cmd_sqe_get_cdw10_11(&ctx->cmd);
		overlapped.OffsetHigh = (xnvme_cmd_sqe_get_cdw10_11(&ctx->cmd) >> 32);
		if (!WriteFile(state->sync_handle, dbuf, dbuf_nbytes, &res, &overlapped)) {
			if (res != (ssize_t)dbuf_nbytes) {
				err = GetLastError();
				XNVME_DEBUG("FAILED: W res: %ld != dbuf_nbytes: %zu, errno: %d",
					    res, dbuf_nbytes, err);
			}
		}
		break;

	case XNVME_SPEC_NVM_READ_OPCODE:
		slba = xnvme_cmd_sqe_get_cdw10_11(&ctx->cmd) << ssw;
		overlapped.Offset = slba;
		overlapped.OffsetHigh = (slba >> 32);
		if (!ReadFile(state->sync_handle, dbuf, dbuf_nbytes, &res, &overlapped)) {
			if (res != (ssize_t)dbuf_nbytes) {
				err = GetLastError();
				XNVME_DEBUG("FAILED: R res: %ld != dbuf_nbytes: %zu, errno: %d",
					    res, dbuf_nbytes, err);
			}
		}
		break;

	case XNVME_SPEC_FS_OPC_READ:
		if (!ReadFile(state->sync_handle, dbuf, dbuf_nbytes, &res, &overlapped)) {
			if (res != (ssize_t)dbuf_nbytes) {
				err = GetLastError();
				XNVME_DEBUG("FAILED: R res: %ld != dbuf_nbytes: %zu, errno: %d",
					    res, dbuf_nbytes, err);
			}
		}
		break;

	case XNVME_SPEC_IO_FLUSH_OPCODE:
	case XNVME_SPEC_FS_OPC_FLUSH:
		if (FlushFileBuffers(state->sync_handle)) {
			res = dbuf_nbytes;
		} else {
			err = GetLastError();
			XNVME_DEBUG("Flush request failed with error code: %d", err);
		}
		break;

	default:
		XNVME_DEBUG("FAILED: nosys opcode: %d", xnvme_cmd_sqe_get_opc(&ctx->cmd));
		return -ENOSYS;
	}

	xnvme_cmd_cpl_set_dw0_1(&ctx->cpl, res);
	if (res <= 0) {
		XNVME_DEBUG("FAILED: {ReadFile,WriteFile,FlushFileBuffers}(), err: %d", err);
		xnvme_cmd_cpl_set_dw0_1(&ctx->cpl, 0);
		ctx->cpl.status = xnvme_bits_set(
			ctx->cpl.status, XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SC, err);
		ctx->cpl.status = xnvme_bits_set(ctx->cpl.status,
						 XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SCT,
						 XNVME_STATUS_CODE_TYPE_VENDOR);
	}

	return err;
}
#endif

struct xnvme_be_sync g_xnvme_be_windows_sync_fs = {
	.id = "file",
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
	.cmd_io = xnvme_be_windows_sync_fs_cmd_io,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
#endif
};
