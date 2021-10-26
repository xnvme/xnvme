// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
#include <xnvme_be_vfio.h>
#include <xnvme_dev.h>
#include <sys/mman.h>
#include <errno.h>

void *
xnvme_be_vfio_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *XNVME_UNUSED(phys))
{
	void *vaddr;
	struct xnvme_be_vfio_state *state = (void *)dev->be.state;
	struct vfio_state *vfio = &state->ctrl->pci.vfio;

	XNVME_DEBUG("xnvme_be_vfio_buf_alloc(%p, %ld)", dev, nbytes);

	nbytes = ALIGN_UP(nbytes, __VFN_PAGESIZE);

	vaddr = mmap(NULL, nbytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (vaddr == MAP_FAILED) {
		XNVME_DEBUG("FAILED: mmap(): %s\n", strerror(errno));
		return NULL;
	}

	XNVME_DEBUG("vfio_map_vaddr(%p, %p, %ld, NULL)", vfio, vaddr, nbytes);
	if (vfio_map_vaddr(vfio, vaddr, nbytes, NULL)) {
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
	struct vfio_state *vfio = &state->ctrl->pci.vfio;
	struct vfio_iommu_mapping *mapping;
	size_t len;

	XNVME_DEBUG("xnvme_be_vfio_buf_free(%p, %p)", dev, buf);

	/*
	 * munmap() requires the length of the previous memory-mapped area, so
	 * grab the mapping and store the length.
	 */
	mapping = vfio_iommu_find_mapping(&vfio->iommu, buf);
	if (!mapping) {
		XNVME_DEBUG("no mapping for %p\n", buf);
		return;
	}

	len = mapping->len;

	if (vfio_unmap_vaddr(vfio, buf)) {
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
#endif

struct xnvme_be_mem g_xnvme_be_vfio_mem = {
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
	.buf_alloc = xnvme_be_vfio_buf_alloc,
	.buf_realloc = xnvme_be_vfio_buf_realloc,
	.buf_free = xnvme_be_vfio_buf_free,
	.buf_vtophys = xnvme_be_vfio_buf_vtophys,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
#endif
};
