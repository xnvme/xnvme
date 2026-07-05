// SPDX-FileCopyrightText: Simon A. F. Lund
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_DMAMEM_ENABLED
#include <errno.h>
#include <xnvme_be_dmamem.h>
#include <xnvme_dev.h>

/**
 * Allocate a buffer from the RTE's dmamem_heap.
 *
 * The heap sits on one contiguous IOVA window, so offsets translate to
 * IOVAs with a single addition. Callers hand the returned VA to the
 * device via cmd.prp1 = dmamem_va_to_iova(...) later on.
 */
void *
xnvme_be_dmamem_buf_alloc(const struct xnvme_dev *XNVME_UNUSED(dev), size_t nbytes, uint64_t *phys)
{
	size_t offset = 0;
	void *buf;
	int err;

	err = dmamem_heap_alloc(&g_dmamem_rte.heap, nbytes, &offset);
	if (err) {
		errno = -err;
		return NULL;
	}

	buf = dmamem_heap_at_va(&g_dmamem_rte.heap, offset);
	if (!buf) {
		dmamem_heap_free(&g_dmamem_rte.heap, offset);
		errno = EFAULT;
		return NULL;
	}

	if (phys) {
		*phys = dmamem_heap_at_iova(&g_dmamem_rte.heap, offset);
	}

	return buf;
}

void
xnvme_be_dmamem_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	size_t offset;

	if (!buf) {
		return;
	}
	offset = (size_t)((char *)buf - (char *)g_dmamem_rte.dmem.cpu_va);

	dmamem_heap_free(&g_dmamem_rte.heap, offset);
}

int
xnvme_be_dmamem_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf, uint64_t *phys)
{
	*phys = dmamem_va_to_iova(&g_dmamem_rte.dmem, buf);

	return 0;
}

#endif

struct xnvme_be_mem g_xnvme_be_dmamem_mem = {
	.id = "dmamem",
#ifdef XNVME_BE_DMAMEM_ENABLED
	.buf_alloc = xnvme_be_dmamem_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_dmamem_buf_free,
	.buf_vtophys = xnvme_be_dmamem_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#endif
};
