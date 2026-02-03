// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_MACOS_ENABLED
#include <xnvme_be_macos_driverkit.h>

#include <mach/mach_error.h>

#include <xnvme_dev.h>
#include <errno.h>
#include <sys/mman.h>

void *
xnvme_be_macos_driverkit_buf_alloc(const struct xnvme_dev *dev, size_t nbytes,
				   uint64_t *XNVME_UNUSED(phys))
{
	struct xnvme_be_macos_driverkit_state *state =
		(struct xnvme_be_macos_driverkit_state *)dev->be.state;
	long sz = sysconf(_SC_PAGESIZE);
	void *ptr;
	uint64_t *buf;

	if (sz == -1) {
		XNVME_DEBUG("FAILED: sysconf(), errno: %d", errno);
		return NULL;
	}
	XNVME_DEBUG("INFO: Page size: %lx", sz);

	// nbytes has to be a multiple of alignment. Therefore, we round up to the nearest
	// multiple.
	XNVME_DEBUG("INFO: Unaligned size: %lx", nbytes);
	nbytes = (1 + ((nbytes - 1) / sz)) * (sz);
	XNVME_DEBUG("INFO: Aligned size: %lx", nbytes);

	ptr = xnvme_buf_virt_alloc(sz, nbytes);
	buf = (uint64_t *)ptr;
	buf[0] = nbytes;        // Put actual size inside allocated memory. Driver on the other end
				// doesn't correctly get this.
	buf[1] = (uint64_t)ptr; // Our token for the memory reference

	XNVME_DEBUG("INFO: buf token: %llx", (uint64_t)ptr);
	kern_return_t ret = IOConnectCallStructMethod(state->device_connection, NVME_ALLOC_BUFFER,
						      ptr, nbytes, NULL, 0);
	if (ret != kIOReturnSuccess) {
		XNVME_DEBUG("FAILED: IOConnectCallStructMethod(NVME_ALLOC_BUFFER); "
			    "ret(0x%08x), '%s'",
			    ret, mach_error_string(ret));

		errno = ENOMEM;
		return NULL;
	}

	buffer_add(state->buffers, (struct buffer){.vaddr = (uint64_t)ptr, .nbytes = nbytes});
	return ptr;
}

void
xnvme_be_macos_driverkit_buf_free(const struct xnvme_dev *dev, void *buf)
{
	struct xnvme_be_macos_driverkit_state *state =
		(struct xnvme_be_macos_driverkit_state *)dev->be.state;
	size_t _output_cnt = 0;
	struct buffer *_buf;
	kern_return_t ret;

	if (!buf) {
		XNVME_DEBUG("FAILED: !buf");
		return;
	}
	_buf = buffer_find(state->buffers, (uint64_t)buf);
	if (!_buf) {
		XNVME_DEBUG("FAILED: buffer_find()");
		return;
	}

	XNVME_DEBUG("INFO: buf token: %llx", (uint64_t)_buf->vaddr);
	ret = IOConnectCallStructMethod(state->device_connection, NVME_DEALLOC_BUFFER,
					&_buf->vaddr, sizeof(_buf->vaddr), NULL, &_output_cnt);
	if (ret != kIOReturnSuccess) {
		XNVME_DEBUG("FAILED: IOConnectCallStructMethod(NVME_DEALLOC_BUFFER); "
			    "ret(0x%08x), '%s'",
			    ret, mach_error_string(ret));
		return;
	}
	// Doing a short-hand of 'buffer_remove' to skip the search;
	_buf->in_use = false;
}

#endif

struct xnvme_be_mem g_xnvme_be_macos_driverkit_mem = {
#ifdef XNVME_PLATFORM_MACOS_ENABLED
	.buf_alloc = xnvme_be_macos_driverkit_buf_alloc,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_macos_driverkit_buf_free,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#endif
	.id = "driverkit",
};
