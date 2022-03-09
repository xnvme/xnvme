// Copyright (C) Rishabh Shukla <rishabh.sh@samsung.com>
// Copyright (C) Vikash Kumar <vikash.k5@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_WINDOWS_ASYNC_ENABLED
#include <errno.h>
#include <xnvme_queue.h>
#include <xnvme_dev.h>
#include <xnvme_be_windows.h>
#include <libxnvme_spec_fs.h>
#include <windows.h>

struct _ov_request {
	OVERLAPPED o;
	struct xnvme_cmd_ctx *data;
	TAILQ_ENTRY(_ov_request) link;
};

struct xnvme_queue_aio_ov {
	struct xnvme_queue_base base;
	HANDLE iocp_handle;
	TAILQ_HEAD(, _ov_request) reqs_ready;
	struct _ov_request *rp;
	uint8_t rsvd[198];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_aio_ov) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

int
_windows_async_iocp_term(struct xnvme_queue *q)
{
	struct xnvme_queue_aio_ov *queue = (void *)q;

	if (!queue) {
		XNVME_DEBUG("FAILED: queue: %p", (void *)queue);
		return -EINVAL;
	}

	CloseHandle(queue->iocp_handle);

	free(queue->rp);
	return 0;
}

int
_windows_async_iocp_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	ULONG queue_nbytes = 0;
	int err = 0;
	struct xnvme_queue_aio_ov *queue = (void *)q;
	struct xnvme_be_windows_state *state = (void *)queue->base.dev->be.state;

	uint32_t capacity = queue->base.capacity;
	queue_nbytes = (capacity + 1) * sizeof(struct _ov_request);

	queue->rp = calloc(1, queue_nbytes);
	if (!queue->rp) {
		err = GetLastError();
		XNVME_DEBUG("calloc() memory allocation failed err:%d", err);
		return err;
	}

	TAILQ_INIT(&queue->reqs_ready);

	for (uint32_t i = 0; i < capacity; ++i) {
		TAILQ_INSERT_HEAD(&queue->reqs_ready, &queue->rp[i], link);
	}

	queue->iocp_handle = CreateIoCompletionPort(state->async_handle, NULL, 0, 0);
	if (queue->iocp_handle == NULL) {
		err = GetLastError();
		XNVME_DEBUG("Cannot open completion port %d.", err);
		goto exit;
	}

	return 0;

exit:

	free(queue->rp);

	return err;
}

int
_windows_async_iocp_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_aio_ov *queue = (void *)q;
	uint32_t completed = 0;

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;

	for (uint32_t i = 0; i < max; i++) {
		struct xnvme_cmd_ctx *cmd_ctx = NULL;
		struct _ov_request *req = NULL;
		DWORD byte_count = 0;
		ULONG_PTR comp_key = 0;
		OVERLAPPED *overlapped = NULL;
		BOOL ret = 0;

		ret = GetQueuedCompletionStatus(queue->iocp_handle, &byte_count, &comp_key,
						&overlapped, 0);
		if (!ret || overlapped == NULL) {
			XNVME_DEBUG("INFO: ret: %d, overlapped: %p", overlapped);
			return completed;
		}
		req = CONTAINING_RECORD(overlapped, struct _ov_request, o);
		cmd_ctx = (struct xnvme_cmd_ctx *)req->data;

		if (overlapped->Internal == ERROR_SUCCESS) {
			cmd_ctx->cpl.result = byte_count;
		} else {
			cmd_ctx->cpl.result = 0;
			cmd_ctx->cpl.status.sc = GetLastError();
			cmd_ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
		}

		cmd_ctx->async.cb(cmd_ctx, cmd_ctx->async.cb_arg);
		completed += 1;
		queue->base.outstanding -= 1;

		TAILQ_INSERT_TAIL(&(queue)->reqs_ready, req, link);
	}

	return completed;
}

int
_windows_async_iocp_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t mbuf_nbytes)
{
	struct xnvme_queue_aio_ov *queue = (void *)ctx->async.queue;
	struct xnvme_be_windows_state *state = (void *)ctx->dev->be.state;
	int err = 0;
	struct _ov_request *req;
	const uint64_t ssw = ctx->dev->geo.ssw;
	uint64_t slba = ctx->cmd.nvm.slba << ssw;
	bool ret = 0;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOSYS;
	}

	req = TAILQ_FIRST(&queue->reqs_ready);
	TAILQ_REMOVE(&queue->reqs_ready, req, link);
	assert(req != NULL);

	LPOVERLAPPED ovl = &req->o;
	memset(ovl, 0, sizeof(OVERLAPPED));

	req->data = ctx;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_FS_OPC_WRITE:
		ovl->Offset = ctx->cmd.nvm.slba & 0xFFFFFFFF;
		ovl->OffsetHigh = (ctx->cmd.nvm.slba >> 32);
		ret = WriteFile(state->async_handle, dbuf, dbuf_nbytes, 0, ovl);
		if (!ret) {
			err = GetLastError();
			if (err != ERROR_IO_PENDING) {
				XNVME_DEBUG("Write request failed with error code: %d", err);
				return err;
			}
		}
		break;

	case XNVME_SPEC_NVM_OPC_WRITE:
		ovl->Offset = slba & 0xFFFFFFFF;
		ovl->OffsetHigh = (slba >> 32);
		ret = WriteFile(state->async_handle, dbuf, dbuf_nbytes, 0, ovl);
		if (!ret) {
			err = GetLastError();
			if (err != ERROR_IO_PENDING) {
				XNVME_DEBUG("Write request failed with error code: %d", err);
				return err;
			}
		}
		break;

	case XNVME_SPEC_FS_OPC_READ:
		ovl->Offset = ctx->cmd.nvm.slba & 0xFFFFFFFF;
		ovl->OffsetHigh = (ctx->cmd.nvm.slba >> 32);
		ret = ReadFile(state->async_handle, dbuf, dbuf_nbytes, 0, ovl);
		if (!ret) {
			err = GetLastError();
			if (err != ERROR_IO_PENDING) {
				XNVME_DEBUG("Read request failed with error code: %d", err);
				return err;
			}
		}
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		ovl->Offset = slba & 0xFFFFFFFF;
		ovl->OffsetHigh = (slba >> 32);
		ret = ReadFile(state->async_handle, dbuf, dbuf_nbytes, 0, ovl);
		if (!ret) {
			err = GetLastError();
			if (err != ERROR_IO_PENDING) {
				XNVME_DEBUG("Read request failed with error code: %d", err);
				return err;
			}
		}
		break;

	case XNVME_SPEC_FS_OPC_FLUSH:
		if (!FlushFileBuffers(state->async_handle)) {
			err = GetLastError();
			if (err != ERROR_IO_PENDING) {
				XNVME_DEBUG("Write request failed with error code: %d", err);
				return err;
			}
		}
		break;

	default:
		XNVME_DEBUG("FAILED: nosys opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	queue->base.outstanding += 1;

	return 0;
}
#endif

struct xnvme_be_async g_xnvme_be_windows_async_iocp = {
	.id = "iocp",
#ifdef XNVME_BE_WINDOWS_ASYNC_ENABLED
	.cmd_io = _windows_async_iocp_cmd_io,
	.poke = _windows_async_iocp_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = _windows_async_iocp_init,
	.term = _windows_async_iocp_term,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
#endif
};
