// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <errno.h>
#include <xnvme_be_upcie_cuda.h>
#include <xnvme_dev.h>

void *
xnvme_be_upcie_cuda_buf_alloc(const struct xnvme_dev *XNVME_UNUSED(dev), size_t nbytes,
			      uint64_t *phys)
{
	void *buf;

	buf = cudamem_dma_malloc(&g_upcie_cuda_rte.cuda_heap, nbytes);
	if (!buf) {
		errno = ENOMEM;
		return NULL;
	}
	if (phys) {
		*phys = cudamem_dma_v2p(&g_upcie_cuda_rte.cuda_heap, buf);
	}

	return buf;
}

void
xnvme_be_upcie_cuda_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	cudamem_dma_free(&g_upcie_cuda_rte.cuda_heap, buf);
}

int
xnvme_be_upcie_cuda_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf,
				uint64_t *phys)
{
	struct cudamem_heap *heap = &g_upcie_cuda_rte.cuda_heap;

	if (cudamem_heap_contains(heap, buf)) {
		return cudamem_heap_block_virt_to_phys(heap, buf, phys);
	}

	return cudamem_mapping_virt_to_phys(&g_upcie_cuda_rte.mappings, buf, phys);
}

int
xnvme_be_upcie_cuda_mem_map(const struct xnvme_dev *XNVME_UNUSED(dev), void *vaddr, size_t nbytes,
			    uint64_t *phys)
{
	int err;

	err = cudamem_mapping_add(&g_upcie_cuda_rte.mappings, &g_upcie_cuda_rte.cuda_config, vaddr,
				  nbytes, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: cudamem_mapping_add(), err: %d", err);
		return err;
	}

	if (phys) {
		err = cudamem_mapping_virt_to_phys(&g_upcie_cuda_rte.mappings, vaddr, phys);
		if (err) {
			XNVME_DEBUG("FAILED: cudamem_mapping_virt_to_phys(), err: %d", err);
			cudamem_mapping_remove(&g_upcie_cuda_rte.mappings, vaddr);
			return err;
		}
	}

	return 0;
}

int
xnvme_be_upcie_cuda_mem_unmap(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	return cudamem_mapping_remove(&g_upcie_cuda_rte.mappings, buf);
}

#endif

struct xnvme_be_mem g_xnvme_be_upcie_cuda_mem = {
	.id = "upcie-cuda",
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
	.buf_alloc = xnvme_be_upcie_cuda_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_upcie_cuda_buf_free,
	.buf_vtophys = xnvme_be_upcie_cuda_buf_vtophys,
	.mem_map = xnvme_be_upcie_cuda_mem_map,
	.mem_unmap = xnvme_be_upcie_cuda_mem_unmap,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#endif
};
