// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
#include <errno.h>
#include <xnvme_dev.h>
#include <xnvme_be_windows.h>
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
	struct xnvme_be_windows_state *state = (void *)ctx->dev->be.state;
	const uint64_t ssw = ctx->dev->geo.ssw;

	memset(&overlapped, 0, sizeof(OVERLAPPED));

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		slba = ctx->cmd.nvm.slba << ssw;
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
		overlapped.Offset = ctx->cmd.nvm.slba;
		overlapped.OffsetHigh = (ctx->cmd.nvm.slba >> 32);
		if (!WriteFile(state->sync_handle, dbuf, dbuf_nbytes, &res, &overlapped)) {
			if (res != (ssize_t)dbuf_nbytes) {
				err = GetLastError();
				XNVME_DEBUG("FAILED: W res: %ld != dbuf_nbytes: %zu, errno: %d",
					    res, dbuf_nbytes, err);
			}
		}
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		slba = ctx->cmd.nvm.slba << ssw;
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

	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		if (FlushFileBuffers(state->sync_handle)) {
			res = dbuf_nbytes;
		} else {
			err = GetLastError();
			XNVME_DEBUG("Flush request failed with error code: %d", err);
		}
		break;

	default:
		XNVME_DEBUG("FAILED: nosys opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	ctx->cpl.result = res;
	if (res <= 0) {
		XNVME_DEBUG("FAILED: {ReadFile,WriteFile,FlushFileBuffers}(), err: %d", err);
		ctx->cpl.result = 0;
		ctx->cpl.status.sc = err;
		ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
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
