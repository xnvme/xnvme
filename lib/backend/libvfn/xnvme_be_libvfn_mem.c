// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LIBVFN_ENABLED
#include <errno.h>
#include <sys/mman.h>
#include <xnvme_be_libvfn.h>
#include <xnvme_dev.h>
#include <xnvme_be_state.h>
#include <xnvme_buf.h>

static void
xnvme_be_libvfn_buf_free(const struct xnvme_dev *dev, void *buf)
{
	struct xnvme_be_libvfn_state *state = &xnvme_be_state(dev)->libvfn_state;
	struct iommu_ctx *ctx = state->ctrlr->ctrl->pci.dev.ctx;
	size_t len;

	XNVME_DEBUG("xnvme_be_libvfn_buf_free(%p, %p)", dev, buf);

	if (iommu_unmap_vaddr(ctx, buf, &len)) {
		XNVME_DEBUG("FAILED: iommu_unmap_vaddr(-, %p): %s\n", buf, strerror(errno));
		return;
	}

	if (munmap(buf, len)) {
		XNVME_DEBUG("FAILED: munmap(%p, %zu): %s\n", buf, len, strerror(errno));
	}
}

static void *
xnvme_be_libvfn_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys)
{
	void *vaddr;
	ssize_t len;
	struct xnvme_be_libvfn_state *state = &xnvme_be_state(dev)->libvfn_state;
	struct iommu_ctx *ctx = state->ctrlr->ctrl->pci.dev.ctx;

	XNVME_DEBUG("xnvme_be_libvfn_buf_alloc(%p, %ld)", dev, nbytes);

	len = pgmap(&vaddr, nbytes);
	if (len < 0) {
		XNVME_DEBUG("FAILED: pgmap(): %s\n", strerror(errno));
		return NULL;
	}

	XNVME_DEBUG("iommu_map_vaddr(%p, %p, %ld, NULL)", ctx, vaddr, len);
	if (iommu_map_vaddr(ctx, vaddr, len, NULL, 0)) {
		XNVME_DEBUG("FAILED: iommu_map_vaddr(): %s\n", strerror(errno));
		return NULL;
	}

	if (phys && !iommu_translate_vaddr(ctx, vaddr, phys)) {
		XNVME_DEBUG("FAILED: iommu_translate_vaddr(-, %p): %s\n", vaddr, strerror(errno));
		xnvme_be_libvfn_buf_free(dev, vaddr);
		return NULL;
	}

	return vaddr;
}

static int
xnvme_be_libvfn_buf_vtophys(const struct xnvme_dev *dev, void *buf, uint64_t *phys)
{
	struct xnvme_be_libvfn_state *state = &xnvme_be_state(dev)->libvfn_state;
	struct iommu_ctx *ctx = state->ctrlr->ctrl->pci.dev.ctx;

	if (!iommu_translate_vaddr(ctx, buf, phys)) {
		XNVME_DEBUG("FAILED: iommu_translate_vaddr(-, %p): %s\n", buf, strerror(errno));
		return -EIO;
	}

	return 0;
}

static int
xnvme_be_libvfn_buf_register(const struct xnvme_dev *dev, void *vaddr, size_t nbytes)
{
	struct xnvme_be_libvfn_state *state = &xnvme_be_state(dev)->libvfn_state;
	struct iommu_ctx *ctx = state->ctrlr->ctrl->pci.dev.ctx;
	struct xnvme_buf_reg *reg;
	bool first, last;
	uint64_t iova;
	int err;

	XNVME_DEBUG("xnvme_be_libvfn_buf_register(%p, %p)", dev, vaddr);

	if (!vaddr || !nbytes || !ALIGNED(((uintptr_t)vaddr | nbytes), __VFN_PAGESIZE)) {
		XNVME_DEBUG("FAILED: vaddr(%p) and nbytes(%zu) must be non-zero and page aligned",
			    vaddr, nbytes);
		return -EINVAL;
	}

	err = xnvme_buf_reg_find(ctx, vaddr, nbytes, &reg);
	if (!err) {
		reg->refs += 1;
		return 0;
	}
	if (err != -ENOENT) {
		return err;
	}

	/*
	 * libvfn takes a range whose start it maps already as mapped, whatever its length, and
	 * unmaps the whole mapping a range lies in, so mapped memory is left to its owner
	 */
	first = iommu_translate_vaddr(ctx, vaddr, &iova);
	last = iommu_translate_vaddr(ctx, (uint8_t *)vaddr + nbytes - 1, &iova);
	if (first && last) {
		XNVME_DEBUG("INFO: vaddr(%p) is mapped already, as from xnvme_buf_alloc()", vaddr);
		return 0;
	}
	if (first || last) {
		XNVME_DEBUG("FAILED: vaddr(%p, %zu) overlaps mapped memory", vaddr, nbytes);
		return -EBUSY;
	}

	XNVME_DEBUG("iommu_map_vaddr(%p, %p, %ld, NULL)", ctx, vaddr, nbytes);
	if (iommu_map_vaddr(ctx, vaddr, nbytes, NULL, 0)) {
		XNVME_DEBUG("FAILED: iommu_map_vaddr(): %s\n", strerror(errno));
		return -errno;
	}

	err = xnvme_buf_reg_add(ctx, vaddr, nbytes, 0);
	if (err) {
		iommu_unmap_vaddr(ctx, vaddr, NULL);
	}

	return err;
}

static int
xnvme_be_libvfn_buf_unregister(const struct xnvme_dev *dev, void *buf)
{
	struct xnvme_be_libvfn_state *state = &xnvme_be_state(dev)->libvfn_state;
	struct iommu_ctx *ctx = state->ctrlr->ctrl->pci.dev.ctx;
	struct xnvme_buf_reg *reg;
	uint64_t iova;
	int err;

	XNVME_DEBUG("xnvme_be_libvfn_buf_unregister(%p, %p)", dev, buf);

	err = xnvme_buf_reg_find(ctx, buf, 1, &reg);
	if ((err == -ENOENT) && buf && iommu_translate_vaddr(ctx, buf, &iova)) {
		XNVME_DEBUG("INFO: buf(%p) was not registered, as from xnvme_buf_alloc()", buf);
		return 0;
	}
	if (err) {
		XNVME_DEBUG("FAILED: buf(%p) is not registered", buf);
		return -EINVAL;
	}

	if (reg->refs > 1) {
		reg->refs -= 1;
		return 0;
	}

	if (iommu_unmap_vaddr(ctx, reg->vaddr, NULL)) {
		XNVME_DEBUG("FAILED: iommu_unmap_vaddr(-, %p): %s\n", reg->vaddr, strerror(errno));
		return -errno;
	}

	xnvme_buf_reg_remove(reg);

	return 0;
}

static int
xnvme_be_libvfn_buf_get_alignment(const struct xnvme_dev *XNVME_UNUSED(dev), size_t *align)
{
	*align = __VFN_PAGESIZE;

	return 0;
}

#endif

struct xnvme_be_mem g_xnvme_be_libvfn_mem = {
	.id = "libvfn",
#ifdef XNVME_BE_LIBVFN_ENABLED
	.buf_alloc = xnvme_be_libvfn_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_libvfn_buf_free,
	.buf_vtophys = xnvme_be_libvfn_buf_vtophys,
	.buf_register = xnvme_be_libvfn_buf_register,
	.buf_unregister = xnvme_be_libvfn_buf_unregister,
	.buf_get_alignment = xnvme_be_libvfn_buf_get_alignment,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.buf_register = xnvme_be_nosys_buf_register,
	.buf_unregister = xnvme_be_nosys_buf_unregister,
#endif
};
