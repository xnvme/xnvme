// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
#include <errno.h>
#include <sys/mman.h>
#include <xnvme_be_vfio.h>
#include <xnvme_dev.h>

void *
xnvme_be_vfio_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *XNVME_UNUSED(phys))
{
	void *vaddr;
	ssize_t len;
	struct xnvme_be_vfio_state *state = (void *)dev->be.state;
	struct vfio_container *vfio = state->ctrl->pci.dev.vfio;

	XNVME_DEBUG("xnvme_be_vfio_buf_alloc(%p, %ld)", dev, nbytes);

	len = pgmap(&vaddr, nbytes);
	if (len < 0) {
		XNVME_DEBUG("FAILED: pgmap(): %s\n", strerror(errno));
		return NULL;
	}

	XNVME_DEBUG("vfio_map_vaddr(%p, %p, %ld, NULL)", vfio, vaddr, len);
	if (vfio_map_vaddr(vfio, vaddr, len, NULL)) {
		XNVME_DEBUG("FAILED: vfio_map_vaddr(): %s\n", strerror(errno));
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

void
xnvme_be_vfio_buf_free(const struct xnvme_dev *dev, void *buf)
{
	struct xnvme_be_vfio_state *state = (void *)dev->be.state;
	struct vfio_container *vfio = state->ctrl->pci.dev.vfio;
	size_t len;

	XNVME_DEBUG("xnvme_be_vfio_buf_free(%p, %p)", dev, buf);

	if (vfio_unmap_vaddr(vfio, buf, &len)) {
		XNVME_DEBUG("FAILED: vfio_pci_unmap_vaddr(-, %p): %s\n", buf, strerror(errno));
	}

	if (munmap(buf, len)) {
		XNVME_DEBUG("FAILED: munmap(%p, %zu): %s\n", buf, len, strerror(errno));
	}
}

int
xnvme_be_vfio_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *XNVME_UNUSED(buf),
			  uint64_t *XNVME_UNUSED(phys))
{
	// TODO: should this thing just map *buf to the iommu, i.e. same as is
	// being done in `xnvme_be_vfio_buf_alloc` using `vfio_pci_dma_map()`?
	errno = ENOSYS;
	return -ENOSYS;
}

int
xnvme_be_vfio_mem_map(const struct xnvme_dev *dev, void *vaddr, size_t nbytes, uint64_t *phys)
{
	struct xnvme_be_vfio_state *state = (void *)dev->be.state;
	struct vfio_container *vfio = state->ctrl->pci.dev.vfio;
	int err;

	XNVME_DEBUG("xnvme_be_vfio_mem_map(%p, %p)", dev, vaddr);

	if (!ALIGNED(((uintptr_t)vaddr | nbytes), __VFN_PAGESIZE)) {
		XNVME_DEBUG("FAILED: nbytes not page aligned\n");
		errno = EINVAL;
		return -EINVAL;
	}

	XNVME_DEBUG("vfio_map_vaddr(%p, %p, %ld, NULL)", vfio, vaddr, nbytes);
	err = vfio_map_vaddr(vfio, vaddr, nbytes, phys);
	if (err) {
		XNVME_DEBUG("FAILED: vfio_map_vaddr(): %s\n", strerror(errno));
		return -errno;
	}

	return 0;
}

int
xnvme_be_vfio_mem_unmap(const struct xnvme_dev *dev, void *buf)
{
	struct xnvme_be_vfio_state *state = (void *)dev->be.state;
	struct vfio_container *vfio = state->ctrl->pci.dev.vfio;

	XNVME_DEBUG("xnvme_be_vfio_buf_unmap(%p, %p)", dev, buf);

	if (vfio_unmap_vaddr(vfio, buf, NULL)) {
		XNVME_DEBUG("FAILED: vfio_unmap_vaddr(-, %p): %s\n", buf, strerror(errno));
		return -errno;
	}

	return 0;
}

#endif

struct xnvme_be_mem g_xnvme_be_vfio_mem = {
	.id = "libvfn",
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
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
