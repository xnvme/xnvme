// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <errno.h>
#include <xnvme_be_upcie_hip.h>
#include <xnvme_dev.h>

void *
xnvme_be_upcie_hip_buf_alloc(const struct xnvme_dev *XNVME_UNUSED(dev), size_t nbytes,
			     uint64_t *phys)
{
	void *buf;

	buf = hipmem_dma_malloc(&g_upcie_hip_rte.hip_heap, nbytes);
	if (!buf) {
		errno = ENOMEM;
		return NULL;
	}
	if (phys) {
		*phys = hipmem_dma_v2p(&g_upcie_hip_rte.hip_heap, buf);
	}

	return buf;
}

void
xnvme_be_upcie_hip_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	hipmem_dma_free(&g_upcie_hip_rte.hip_heap, buf);
}

int
xnvme_be_upcie_hip_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf,
			       uint64_t *phys)
{
	return hipmem_heap_block_virt_to_phys(&g_upcie_hip_rte.hip_heap, buf, phys);
}

#endif

struct xnvme_be_mem g_xnvme_be_upcie_hip_mem = {
	.id = "upcie-hip",
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
	.buf_alloc = xnvme_be_upcie_hip_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_upcie_hip_buf_free,
	.buf_vtophys = xnvme_be_upcie_hip_buf_vtophys,
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
