// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <errno.h>
#include <unistd.h>
#include <xnvme_be_upcie_cuda.h>
#include <xnvme_cuda_buf.h>
#include <xnvme_dev.h>
#include <xnvme_be_state.h>

static void *
xnvme_be_upcie_cuda_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;
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

static void
xnvme_be_upcie_cuda_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	cudamem_dma_free(&g_upcie_cuda_rte.cuda_heap, buf);
}

static int
xnvme_be_upcie_cuda_buf_vtophys(const struct xnvme_dev *dev, void *buf, uint64_t *phys)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;

	*phys = dmamem_va_to_iova(state->dmem, buf);
	if (!*phys) {
		XNVME_DEBUG("FAILED: buf(%p) is neither heap nor registered", buf);
		return -EINVAL;
	}

	return 0;
}

static int
xnvme_be_upcie_cuda_buf_register(const struct xnvme_dev *dev, void *vaddr, size_t nbytes)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;
	int dmabuf_fd = -1;
	int err;

	if (!g_upcie_rte.connection.alive) {
		return xnvme_be_upcie_dmamem_map(state->dmem, vaddr, nbytes, NULL);
	}

	err = xnvme_cuda_buf_export_dmabuf(vaddr, nbytes, &dmabuf_fd);
	if (err) {
		return err;
	}

	err = xnvme_be_upcie_served_mem_map(
		state->dmem, state->ctrlr, dmabuf_fd, vaddr, nbytes,
		(uint32_t)g_upcie_cuda_rte.cuda_config.device_pagesize);
	close(dmabuf_fd);

	return err;
}

static int
xnvme_be_upcie_cuda_buf_unregister(const struct xnvme_dev *dev, void *vaddr)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;

	if (g_upcie_rte.connection.alive) {
		return xnvme_be_upcie_served_mem_unmap(state->dmem, state->ctrlr, vaddr);
	}

	return xnvme_be_upcie_dmamem_unmap(state->dmem, vaddr);
}

static int
xnvme_be_upcie_cuda_buf_get_alignment(const struct xnvme_dev *XNVME_UNUSED(dev), size_t *align)
{
	*align = g_upcie_cuda_rte.cuda_config.device_pagesize;

	return 0;
}

#endif

struct xnvme_be_mem g_xnvme_be_upcie_mem_cuda = {
	.id = "upcie-cuda",
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
	.buf_alloc = xnvme_be_upcie_cuda_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_upcie_cuda_buf_free,
	.buf_vtophys = xnvme_be_upcie_cuda_buf_vtophys,
	.buf_register = xnvme_be_upcie_cuda_buf_register,
	.buf_unregister = xnvme_be_upcie_cuda_buf_unregister,
	.buf_get_alignment = xnvme_be_upcie_cuda_buf_get_alignment,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.buf_register = xnvme_be_nosys_buf_register,
	.buf_unregister = xnvme_be_nosys_buf_unregister,
#endif
};
