// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifdef XNVME_BE_IORING_ENABLED
#define NTDDI_VERSION NTDDI_WIN10_NI
#define _WIN32_WINNT NTDDI_WIN10_NI
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_IORING_ENABLED
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <Windows.h>
#include <stdio.h>
#include <winternl.h>
#include <time.h>
#include <intrin.h>
#include <ioringapi.h>
#include <xnvme_queue.h>
#include <xnvme_dev.h>
#include <xnvme_be_windows.h>
#include <xnvme_be_ioring.h>

#define MAX_IORING_SQ 0x10000
#define MAX_IORING_CQ 0x20000

struct xnvme_queue_ioring {
	struct xnvme_queue_base base;

	HIORING ring_handle;

	HINSTANCE lib_module;

	uint8_t _rsvd[216];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_ioring) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

typedef HRESULT(WINAPI *create_ioring_ptr)(IORING_VERSION ioringVersion, IORING_CREATE_FLAGS flags,
					   UINT32 submissionQueueSize, UINT32 completionQueueSize,
					   _Out_ HIORING *h);

typedef HRESULT(WINAPI *build_ioring_readfile_ptr)(_In_ HIORING ioRing, IORING_HANDLE_REF fileRef,
						   IORING_BUFFER_REF dataRef,
						   UINT32 numberOfBytesToRead, UINT64 fileOffset,
						   UINT_PTR userData, IORING_SQE_FLAGS flags);

typedef HRESULT(WINAPI *build_ioring_writefile_ptr)(_In_ HIORING ioRing, IORING_HANDLE_REF fileRef,
						    IORING_BUFFER_REF bufferRef,
						    UINT32 numberOfBytesToWrite, UINT64 fileOffset,
						    FILE_WRITE_FLAGS writeFlags, UINT_PTR userData,
						    IORING_SQE_FLAGS sqeFlags);

typedef HRESULT(WINAPI *submit_ioring_ptr)(_In_ HIORING ioRing, UINT32 waitOperations,
					   UINT32 milliseconds,
					   _Out_opt_ UINT32 *submittedEntries);

typedef HRESULT(WINAPI *pop_ioring_completion_ptr)(_In_ HIORING ioRing, _Out_ IORING_CQE *cqe);

typedef HRESULT(WINAPI *close_ioring_ptr)(_In_ HIORING ioRing);

static int
xnvme_be_ioring_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	struct xnvme_queue_ioring *queue = (void *)q;
	HRESULT err;
	IORING_CREATE_FLAGS flags;
	HINSTANCE handle = LoadLibrary(TEXT("kernelbase.dll"));

	if (!handle) {
		err = GetLastError();
		XNVME_DEBUG("Failed creating IO ring handle: %" PRId32, err);
		return err;
	}

	queue->lib_module = handle;

	flags.Required = IORING_CREATE_REQUIRED_FLAGS_NONE;
	flags.Advisory = IORING_CREATE_ADVISORY_FLAGS_NONE;

	create_ioring_ptr create_ioring_procadd =
		(create_ioring_ptr)GetProcAddress(queue->lib_module, "CreateIoRing");

	err = (create_ioring_procadd)(IORING_VERSION_3, flags, MAX_IORING_SQ, MAX_IORING_CQ,
				      &queue->ring_handle);
	if (err < 0) {
		XNVME_DEBUG("Failed creating IO ring handle: %" PRId32, err);
		return err;
	}

	return 0;
}

static int
xnvme_be_ioring_term(struct xnvme_queue *q)
{
	struct xnvme_queue_ioring *queue = (void *)q;
	close_ioring_ptr close_ioring_ptr_procadd =
		(close_ioring_ptr)GetProcAddress(queue->lib_module, "CloseIoRing");

	if (!queue) {
		XNVME_DEBUG("FAILED: queue: %p", (void *)queue);
		return -EINVAL;
	}

	if (queue->ring_handle != NULL) {
		(close_ioring_ptr_procadd)(queue->ring_handle);
	}

	if (queue->lib_module) {
		FreeLibrary(queue->lib_module);
	}
	return 0;
}

static int
xnvme_be_ioring_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_ioring *queue = (void *)q;
	unsigned completed = 0;
	HRESULT err;
	pop_ioring_completion_ptr pop_ioring_completion_procadd =
		(pop_ioring_completion_ptr)GetProcAddress(queue->lib_module,
							  "PopIoRingCompletion");

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;

	for (unsigned i = 0; i < max; ++i) {
		struct xnvme_cmd_ctx *ctx;
		IORING_CQE cqe;

		err = (pop_ioring_completion_procadd)(queue->ring_handle, &cqe);
		if (err == S_OK) {
			ctx = (struct xnvme_cmd_ctx *)cqe.UserData;
			if (!ctx) {
				XNVME_DEBUG("-{[THIS SHOULD NOT HAPPEN]}-");
				XNVME_DEBUG("cqe->user_data is NULL! => NO REQ!");
				XNVME_DEBUG("cqe->res: %d", cqe.ResultCode);
				XNVME_DEBUG("cqe->rbytes: %u", cqe.Information);
				return -EIO;
			}

			ctx->cpl.result = cqe.ResultCode;
			if (cqe.ResultCode < 0) {
				ctx->cpl.result = 0;
				ctx->cpl.status.sc = -cqe.ResultCode;
				ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
			}

			ctx->async.cb(ctx, ctx->async.cb_arg);
			completed += 1;
		}
	};

	queue->base.outstanding -= completed;
	return completed;
}

static int
xnvme_be_ioring_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
		       size_t mbuf_nbytes)
{
	struct xnvme_queue_ioring *queue = (void *)ctx->async.queue;
	struct xnvme_be_windows_state *state = (void *)queue->base.dev->be.state;
	uint64_t ssw = 0;
	IORING_HANDLE_REF request_data_file = IoRingHandleRefFromHandle(0);
	IORING_BUFFER_REF request_data_buffer = IoRingBufferRefFromPointer(0);
	HRESULT err;
	uint32_t submitted_entries;
	build_ioring_readfile_ptr build_ioring_readfile_procadd =
		(build_ioring_readfile_ptr)GetProcAddress(queue->lib_module,
							  "BuildIoRingReadFile");
	build_ioring_writefile_ptr build_ioring_writefile_procadd =
		(build_ioring_writefile_ptr)GetProcAddress(queue->lib_module,
							   "BuildIoRingWriteFile");
	submit_ioring_ptr submit_ioring_procadd =
		(submit_ioring_ptr)GetProcAddress(queue->lib_module, "SubmitIoRing");

	if (queue->base.outstanding == queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOTSUP;
	}

	request_data_file.Handle.Handle = state->async_handle;
	request_data_buffer.Buffer.Address = dbuf;
	///< NOTE: opcode-dispatch (io)
	switch (ctx->cmd.common.opcode) {

	case XNVME_SPEC_NVM_OPC_READ:
		ssw = queue->base.dev->geo.ssw;
	/* fall through */
	case XNVME_SPEC_FS_OPC_READ:
		err = (build_ioring_readfile_procadd)(queue->ring_handle, request_data_file,
						      request_data_buffer, dbuf_nbytes,
						      ctx->cmd.nvm.slba << ssw, (UINT_PTR)ctx,
						      IOSQE_FLAGS_NONE);

		if (err < 0) {
			XNVME_DEBUG("Failed building IO ring read file structure: 0x%" PRIx32,
				    err);
			return err;
		}
		break;

	case XNVME_SPEC_NVM_OPC_WRITE:
		ssw = queue->base.dev->geo.ssw;
	/* fall through */
	case XNVME_SPEC_FS_OPC_WRITE:
		err = (build_ioring_writefile_procadd)(queue->ring_handle, request_data_file,
						       request_data_buffer, dbuf_nbytes,
						       ctx->cmd.nvm.slba << ssw,
						       FILE_WRITE_FLAGS_NONE, (UINT_PTR)ctx,
						       IOSQE_FLAGS_NONE);

		if (err < 0) {
			XNVME_DEBUG("Failed building IO ring write file structure: 0x%d\n", err);
			return err;
		}
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported opcode: 0x%" PRIx16 " for async",
			    ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	err = (submit_ioring_procadd)(queue->ring_handle, 0, 0, &submitted_entries);
	if (err < 0) {
		XNVME_DEBUG("io_uring_submit(0x%" PRIx16 "), err: %d", ctx->cmd.common.opcode,
			    err);
		return err;
	}

	queue->base.outstanding += 1;

	return 0;
}

#endif

struct xnvme_be_async g_xnvme_be_ioring_async = {
	.id = "io_ring",
#ifdef XNVME_BE_IORING_ENABLED
	.cmd_io = xnvme_be_ioring_cmd_io,
	.poke = xnvme_be_ioring_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_ioring_init,
	.term = xnvme_be_ioring_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
#endif
};
