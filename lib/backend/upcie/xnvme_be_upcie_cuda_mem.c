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
xnvme_be_upcie_cuda_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys)
{
	const struct xnvme_be_upcie_state *state = (void *)dev->be.state;
	void *buf;

	buf = cudamem_dma_malloc(&g_upcie_cuda_rte.cuda_heap, nbytes);
	if (!buf) {
		errno = ENOMEM;
		return NULL;
	}
	if (phys) {
		*phys = dmamem_va_to_iova(state->dmem, buf);
	}

	return buf;
}

void
xnvme_be_upcie_cuda_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	cudamem_dma_free(&g_upcie_cuda_rte.cuda_heap, buf);
}

int
xnvme_be_upcie_cuda_buf_vtophys(const struct xnvme_dev *dev, void *buf, uint64_t *phys)
{
	const struct xnvme_be_upcie_state *state = (void *)dev->be.state;

	*phys = dmamem_va_to_iova(state->dmem, buf);
	if (!*phys) {
		XNVME_DEBUG("FAILED: buf(%p) is neither heap nor registered", buf);
		return -EINVAL;
	}

	return 0;
}

/**
 * Register a caller-allocated device buffer for DMA
 *
 * Resolves through this device's registry, so a buffer handed over here is
 * usable on this device as one from xnvme_buf_alloc() is; behind an IOMMU each
 * controller has IOVAs of its own, so register it with each device it is used
 * from. Registering the same range twice is cheap: the chunks it covers are
 * refcounted.
 */
int
xnvme_be_upcie_cuda_mem_map(const struct xnvme_dev *dev, void *vaddr, size_t nbytes,
			    uint64_t *phys)
{
	const struct xnvme_be_upcie_state *state = (void *)dev->be.state;

	return xnvme_be_upcie_dmamem_map(state->dmem, vaddr, nbytes, phys);
}

int
xnvme_be_upcie_cuda_mem_unmap(const struct xnvme_dev *dev, void *vaddr)
{
	const struct xnvme_be_upcie_state *state = (void *)dev->be.state;

	return xnvme_be_upcie_dmamem_unmap(state->dmem, vaddr);
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
