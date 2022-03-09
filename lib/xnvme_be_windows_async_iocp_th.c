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

enum io_state { NONE, IN_PROGRESS, SUCCESS, FAILED };

struct _thread_ctx {
	HANDLE iocp;
	HANDLE iothread;
	bool iocp_running;
};

struct _ov_request {
	OVERLAPPED o;
	struct xnvme_cmd_ctx *data;
	int err;
	uint32_t byte_count;
	enum io_state state;
	TAILQ_ENTRY(_ov_request) link;
};

struct xnvme_queue_aio_ov {
	struct xnvme_queue_base base;
	struct _thread_ctx *pt_ctx;
	TAILQ_HEAD(, _ov_request) reqs_ready;
	TAILQ_HEAD(, _ov_request) reqs_outstanding;
	struct _ov_request *rp;
	uint8_t rsvd[184];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_aio_ov) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

DWORD WINAPI
_windows_iocp(LPVOID context)
{
	DWORD byte_count;
	ULONG_PTR comp_key;
	OVERLAPPED *overlapped;
	struct _ov_request *req;
	struct _thread_ctx *ctx = (struct _thread_ctx *)context;
	bool ret;

	while (ctx->iocp_running) {
		byte_count = 0;
		comp_key = 0;
		overlapped = NULL;
		req = NULL;
		ret = GetQueuedCompletionStatus(ctx->iocp, &byte_count, &comp_key, &overlapped, 0);
		if (!ret && overlapped == NULL) {
			continue;
		}

		req = CONTAINING_RECORD(overlapped, struct _ov_request, o);

		if (overlapped->Internal == ERROR_SUCCESS) {
			req->err = 0;
			req->byte_count = byte_count;
			req->state = SUCCESS;
		} else {
			req->err = GetLastError();
			req->byte_count = 0;
			req->state = FAILED;
		}
	}
	return 0;
}

int
_windows_async_iocp_th_term(struct xnvme_queue *q)
{
	struct xnvme_queue_aio_ov *queue = (void *)q;

	if (!queue) {
		XNVME_DEBUG("FAILED: queue: %p", (void *)queue);
		return -EINVAL;
	}

	queue->pt_ctx->iocp_running = FALSE;
	CloseHandle(queue->pt_ctx->iocp);
	CloseHandle(queue->pt_ctx->iothread);

	free(queue->pt_ctx);
	free(queue->rp);
	return 0;
}

int
_windows_async_iocp_th_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	ULONG queue_nbytes;
	unsigned long thread_id;
	int err;
	struct _thread_ctx *pt_ctx;
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
	TAILQ_INIT(&queue->reqs_outstanding);

	queue->pt_ctx = calloc(1, sizeof(struct _thread_ctx));
	if (!queue->pt_ctx) {
		err = GetLastError();
		XNVME_DEBUG("malloc() memory allocation failed err:%d", err);
		goto exit;
	}

	pt_ctx = queue->pt_ctx;

	pt_ctx->iocp = CreateIoCompletionPort(state->async_handle, NULL, 0, 0);
	if (queue->pt_ctx->iocp == NULL) {
		err = GetLastError();
		XNVME_DEBUG("Cannot open completion port %d.", err);
		goto exit;
	}

	pt_ctx->iocp_running = TRUE;

	pt_ctx->iothread = CreateThread(NULL, 0, _windows_iocp, pt_ctx, 0, &thread_id);
	if (pt_ctx->iothread == NULL) {
		err = GetLastError();
		XNVME_DEBUG("CreateThread failed. Err:%d\n", err);
		goto exit;
	}

	return 0;

exit:

	CloseHandle(queue->pt_ctx->iocp);
	CloseHandle(queue->pt_ctx->iothread);

	free(queue->rp);
	free(queue->pt_ctx);

	return err;
}

int
_windows_async_iocp_th_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_aio_ov *queue = (void *)q;
	struct _ov_request *req = NULL;
	struct xnvme_cmd_ctx *cmd_ctx = NULL;
	uint32_t completed = 0;

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;

	req = TAILQ_FIRST(&queue->reqs_outstanding);
	assert(req != NULL);

	while (req != NULL && completed < max) {
		cmd_ctx = req->data;

		if (!cmd_ctx) {
			XNVME_DEBUG("-{[THIS SHOULD NOT HAPPEN]}-");
			XNVME_DEBUG("ctx is NULL! => NO REQ!");
			XNVME_DEBUG("unprocessed events might remain");

			queue->base.outstanding -= 1;

			return -EIO;
		}

		switch (req->state) {
		case IN_PROGRESS:
			req = TAILQ_NEXT(req, link);
			continue;

		case SUCCESS:
			req->err = 0;
			cmd_ctx->cpl.result = req->byte_count;
			break;

		case FAILED:
			cmd_ctx->cpl.result = 0;
			cmd_ctx->cpl.status.sc = req->err;
			cmd_ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
			break;

		default:
			break;
		}

		cmd_ctx->async.cb(cmd_ctx, cmd_ctx->async.cb_arg);
		completed += 1;
		queue->base.outstanding -= 1;

		TAILQ_REMOVE(&queue->reqs_outstanding, req, link);
		TAILQ_INSERT_TAIL(&(queue)->reqs_ready, req, link);

		req = TAILQ_FIRST(&queue->reqs_outstanding);
	}

	return completed;
}

int
_windows_async_iocp_th_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			      void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_queue_aio_ov *queue = (void *)ctx->async.queue;
	struct xnvme_be_windows_state *state = (void *)ctx->dev->be.state;
	int err;
	struct _ov_request *req;
	const uint64_t ssw = ctx->dev->geo.ssw;
	uint64_t slba = ctx->cmd.nvm.slba << ssw;
	bool ret;
	LPOVERLAPPED ovl;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOSYS;
	}

	req = TAILQ_FIRST(&queue->reqs_ready);
	assert(req != NULL);

	ovl = &req->o;
	memset(ovl, 0, sizeof(OVERLAPPED));

	req->data = ctx;
	req->state = IN_PROGRESS;

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

	TAILQ_REMOVE(&queue->reqs_ready, req, link);
	TAILQ_INSERT_TAIL(&queue->reqs_outstanding, req, link);
	queue->base.outstanding += 1;

	return 0;
}
#endif

struct xnvme_be_async g_xnvme_be_windows_async_iocp_th = {
	.id = "iocp_th",
#ifdef XNVME_BE_WINDOWS_ASYNC_ENABLED
	.cmd_io = _windows_async_iocp_th_cmd_io,
	.poke = _windows_async_iocp_th_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = _windows_async_iocp_th_init,
	.term = _windows_async_iocp_th_term,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
#endif
};
