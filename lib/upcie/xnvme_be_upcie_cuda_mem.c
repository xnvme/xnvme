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
	int err;

	err = cudamem_heap_block_virt_to_phys(&g_upcie_cuda_rte.cuda_heap, buf, phys);
	if (!err) {
		return 0;
	}

	uint64_t vaddr = (uint64_t)buf;

	for (struct cudamem_mapping *m = g_upcie_cuda_rte.mappings; m; m = m->next) {
		if (vaddr >= m->vaddr && vaddr < m->vaddr + m->size) {
			size_t offset = vaddr - m->vaddr;
			size_t page_idx = offset / m->config->device_pagesize;
			size_t in_page_offset = offset % m->config->device_pagesize;

			if (page_idx >= m->nphys) {
				return -EINVAL;
			}

			*phys = m->phys_lut[page_idx] + in_page_offset;
			return 0;
		}
	}

	return -EINVAL;
}

int
xnvme_be_upcie_cuda_mem_map(const struct xnvme_dev *XNVME_UNUSED(dev), void *vaddr,
			    size_t nbytes, uint64_t *phys)
{
	struct cudamem_config *config = &g_upcie_cuda_rte.cuda_config;
	struct cudamem_mapping *m;
	int dmabuf_fd = -1;
	CUresult cr;
	int err;

	if (!vaddr || !nbytes) {
		return -EINVAL;
	}
	if ((uint64_t)vaddr % config->device_pagesize ||
	    nbytes % config->device_pagesize) {
		XNVME_DEBUG("FAILED: vaddr/nbytes not aligned to device_pagesize(%d)",
			    config->device_pagesize);
		return -EINVAL;
	}

	cr = cuMemGetHandleForAddressRange(&dmabuf_fd, (CUdeviceptr)vaddr, nbytes,
					   CU_MEM_RANGE_HANDLE_TYPE_DMA_BUF_FD, 0);
	if (cr != CUDA_SUCCESS) {
		XNVME_DEBUG("FAILED: cuMemGetHandleForAddressRange(), cr: %d", cr);
		return -EIO;
	}

	m = calloc(1, sizeof(*m));
	if (!m) {
		close(dmabuf_fd);
		return -ENOMEM;
	}

	err = dmabuf_attach(dmabuf_fd, &m->dmabuf);
	if (err) {
		XNVME_DEBUG("FAILED: dmabuf_attach(), err: %d", err);
		free(m);
		return err;
	}

	m->vaddr = (uint64_t)vaddr;
	m->size = nbytes;
	m->config = config;
	m->nphys = nbytes / config->device_pagesize;
	m->phys_lut = calloc(m->nphys, sizeof(*m->phys_lut));
	if (!m->phys_lut) {
		dmabuf_detach(&m->dmabuf);
		free(m);
		return -ENOMEM;
	}

	err = dmabuf_get_lut(&m->dmabuf, m->nphys, m->phys_lut, config->device_pagesize);
	if (err) {
		XNVME_DEBUG("FAILED: dmabuf_get_lut(), err: %d", err);
		free(m->phys_lut);
		dmabuf_detach(&m->dmabuf);
		free(m);
		return err;
	}

	m->next = g_upcie_cuda_rte.mappings;
	g_upcie_cuda_rte.mappings = m;

	if (phys) {
		*phys = m->phys_lut[0];
	}

	return 0;
}

int
xnvme_be_upcie_cuda_mem_unmap(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	uint64_t vaddr = (uint64_t)buf;
	struct cudamem_mapping **prev = &g_upcie_cuda_rte.mappings;

	for (struct cudamem_mapping *m = *prev; m; prev = &m->next, m = m->next) {
		if (m->vaddr == vaddr) {
			*prev = m->next;
			dmabuf_detach(&m->dmabuf);
			free(m->phys_lut);
			free(m);
			return 0;
		}
	}

	return -EINVAL;
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
