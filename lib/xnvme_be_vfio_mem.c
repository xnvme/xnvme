// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_VFIO_ENABLED
#include <errno.h>
#include <sys/mman.h>
#include <xnvme_be_vfio.h>
#include <xnvme_dev.h>

void
xnvme_be_vfio_buf_free(const struct xnvme_dev *dev, void *buf)
{
	struct xnvme_be_vfio_state *state = (void *)dev->be.state;
	struct iommu_ctx *ctx = state->ctrl->pci.dev.ctx;
	size_t len;

	XNVME_DEBUG("xnvme_be_vfio_buf_free(%p, %p)", dev, buf);

	if (iommu_unmap_vaddr(ctx, buf, &len)) {
		XNVME_DEBUG("FAILED: iommu_unmap_vaddr(-, %p): %s\n", buf, strerror(errno));
		return;
	}

	if (munmap(buf, len)) {
		XNVME_DEBUG("FAILED: munmap(%p, %zu): %s\n", buf, len, strerror(errno));
	}
}

void *
xnvme_be_vfio_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys)
{
	void *vaddr;
	ssize_t len;
	struct xnvme_be_vfio_state *state = (void *)dev->be.state;
	struct iommu_ctx *ctx = state->ctrl->pci.dev.ctx;

	XNVME_DEBUG("xnvme_be_vfio_buf_alloc(%p, %ld)", dev, nbytes);

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
		xnvme_be_vfio_buf_free(dev, vaddr);
		return NULL;
	}

	return vaddr;
}

void *
xnvme_be_vfio_buf_realloc(const struct xnvme_dev *XNVME_UNUSED(dev), void *XNVME_UNUSED(buf),
			  size_t XNVME_UNUSED(nbytes), uint64_t *XNVME_UNUSED(phys))
{
	errno = ENOSYS;
	return NULL;
}

int
xnvme_be_vfio_buf_vtophys(const struct xnvme_dev *dev, void *buf, uint64_t *phys)
{
	struct xnvme_be_vfio_state *state = (void *)dev->be.state;
	struct iommu_ctx *ctx = state->ctrl->pci.dev.ctx;

	if (!iommu_translate_vaddr(ctx, buf, phys)) {
		XNVME_DEBUG("FAILED: iommu_translate_vaddr(-, %p): %s\n", buf, strerror(errno));
		return -EIO;
	}

	return 0;
}

int
xnvme_be_vfio_mem_map(const struct xnvme_dev *dev, void *vaddr, size_t nbytes, uint64_t *phys)
{
	struct xnvme_be_vfio_state *state = (void *)dev->be.state;
	struct iommu_ctx *ctx = state->ctrl->pci.dev.ctx;
	int err;

	XNVME_DEBUG("xnvme_be_vfio_mem_map(%p, %p)", dev, vaddr);

	if (!ALIGNED(((uintptr_t)vaddr | nbytes), __VFN_PAGESIZE)) {
		XNVME_DEBUG("FAILED: nbytes not page aligned\n");
		errno = EINVAL;
		return -EINVAL;
	}

	XNVME_DEBUG("iommu_map_vaddr(%p, %p, %ld, NULL)", ctx, vaddr, nbytes);
	err = iommu_map_vaddr(ctx, vaddr, nbytes, phys, 0);
	if (err) {
		XNVME_DEBUG("FAILED: iommu_map_vaddr(): %s\n", strerror(errno));
		return -errno;
	}

	return 0;
}

int
xnvme_be_vfio_mem_unmap(const struct xnvme_dev *dev, void *buf)
{
	struct xnvme_be_vfio_state *state = (void *)dev->be.state;
	struct iommu_ctx *ctx = state->ctrl->pci.dev.ctx;

	XNVME_DEBUG("xnvme_be_vfio_buf_unmap(%p, %p)", dev, buf);

	if (iommu_unmap_vaddr(ctx, buf, NULL)) {
		XNVME_DEBUG("FAILED: iommu_unmap_vaddr(-, %p): %s\n", buf, strerror(errno));
		return -errno;
	}

	return 0;
}

#endif

struct xnvme_be_mem g_xnvme_be_vfio_mem = {
	.id = "libvfn",
#ifdef XNVME_BE_VFIO_ENABLED
	.buf_alloc = xnvme_be_vfio_buf_alloc,
	.buf_realloc = xnvme_be_vfio_buf_realloc,
	.buf_free = xnvme_be_vfio_buf_free,
	.buf_vtophys = xnvme_be_vfio_buf_vtophys,
	.mem_map = xnvme_be_vfio_mem_map,
	.mem_unmap = xnvme_be_vfio_mem_unmap,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#endif
};
