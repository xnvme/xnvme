// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifndef __INTERNAL_XNVME_BE_WINDOWS_IORING_H
#define __INTERNAL_XNVME_BE_WINDOWS_IORING_H

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

#endif /* __INTERNAL_XNVME_BE_WINDOWS_IORING_H */
