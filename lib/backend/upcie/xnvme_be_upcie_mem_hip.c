// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <errno.h>
#include <unistd.h>
#include <xnvme_be_upcie_hip.h>
#include <xnvme_hip_buf.h>
#include <xnvme_dev.h>
#include <xnvme_be_state.h>
#include <xnvme_buf.h>

static void *
xnvme_be_upcie_hip_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;
	void *buf;

	buf = hipmem_dma_malloc(&g_upcie_hip_rte.hip_heap, nbytes);
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
xnvme_be_upcie_hip_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	hipmem_dma_free(&g_upcie_hip_rte.hip_heap, buf);
}

static int
xnvme_be_upcie_hip_buf_vtophys(const struct xnvme_dev *dev, void *buf, uint64_t *phys)
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
_map(const struct xnvme_dev *dev, void *vaddr, size_t nbytes)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;
	int dmabuf_fd = -1;
	int err;

	if (!g_upcie_rte.connection.alive) {
		return xnvme_be_upcie_dmamem_map(state->dmem, vaddr, nbytes, NULL);
	}

	err = xnvme_hip_buf_export_dmabuf(vaddr, nbytes, &dmabuf_fd);
	if (err) {
		return err;
	}

	err = xnvme_be_upcie_served_mem_map(state->dmem, state->ctrlr, dmabuf_fd, vaddr, nbytes,
					    (uint32_t)g_upcie_hip_rte.hip_config.device_pagesize);
	close(dmabuf_fd);

	return err;
}

static int
_unmap(const struct xnvme_dev *dev, void *vaddr)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;

	if (g_upcie_rte.connection.alive) {
		return xnvme_be_upcie_served_mem_unmap(state->dmem, vaddr);
	}

	return xnvme_be_upcie_dmamem_unmap(state->dmem, vaddr);
}

/* Served, a range is installed for the controllers it was registered through only */
static int
_reach(const struct xnvme_dev *dev, void *vaddr, size_t nbytes)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;
	struct xnvme_buf_reg *reg;
	int err;

	if (!g_upcie_rte.connection.alive ||
	    xnvme_buf_reg_find(state->dmem, vaddr, nbytes, &reg) ||
	    xnvme_be_upcie_served_mem_reaches(state->ctrlr, reg->vaddr)) {
		return 0;
	}

	err = _map(dev, reg->vaddr, reg->nbytes);
	if (err) {
		reg->refs -= 1;
	}

	return err;
}

static int
_add(const struct xnvme_dev *dev, void *vaddr, size_t nbytes)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;
	int err;

	err = xnvme_buf_reg_add(state->dmem, vaddr, nbytes, 0);
	if (err) {
		_unmap(dev, vaddr);
	}

	return err;
}

static int
xnvme_be_upcie_hip_buf_register(const struct xnvme_dev *dev, void *vaddr, size_t nbytes)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;
	int err;

	err = xnvme_be_upcie_buf_reg_ref(state->dmem, g_upcie_hip_rte.hip_heap.vaddr,
					 g_upcie_hip_rte.hip_heap.size, vaddr, nbytes);
	if (err != -ENOENT) {
		return err ? err : _reach(dev, vaddr, nbytes);
	}

	err = _map(dev, vaddr, nbytes);
	if (err) {
		return err;
	}

	return _add(dev, vaddr, nbytes);
}

static int
xnvme_be_upcie_hip_buf_unregister(const struct xnvme_dev *dev, void *vaddr)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;
	struct xnvme_buf_reg *last;
	int err;

	err = xnvme_be_upcie_buf_reg_unref(state->dmem, g_upcie_hip_rte.hip_heap.vaddr,
					   g_upcie_hip_rte.hip_heap.size, vaddr, &last);
	if (err || !last) {
		return err;
	}

	err = _unmap(dev, last->vaddr);
	if (err) {
		return err;
	}
	xnvme_buf_reg_remove(last);

	return 0;
}

static int
xnvme_be_upcie_hip_buf_get_alignment(const struct xnvme_dev *XNVME_UNUSED(dev), size_t *align)
{
	*align = g_upcie_hip_rte.hip_config.device_pagesize;

	return 0;
}

#endif

struct xnvme_be_mem g_xnvme_be_upcie_mem_hip = {
	.id = "upcie-hip",
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
	.buf_alloc = xnvme_be_upcie_hip_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_upcie_hip_buf_free,
	.buf_vtophys = xnvme_be_upcie_hip_buf_vtophys,
	.buf_register = xnvme_be_upcie_hip_buf_register,
	.buf_unregister = xnvme_be_upcie_hip_buf_unregister,
	.buf_get_alignment = xnvme_be_upcie_hip_buf_get_alignment,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.buf_register = xnvme_be_nosys_buf_register,
	.buf_unregister = xnvme_be_nosys_buf_unregister,
#endif
};
