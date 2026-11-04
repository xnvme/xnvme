// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_cplane_client.h>
#include <xnvme_dev.h>

/**
 * Allocate a buffer from the RTE's dmamem_heap.
 *
 * Offsets into the heap resolve to device addresses through the dmamem
 * translator, so the same allocation works whichever way the target is
 * attached; the caller hands the returned VA to the device as a PRP later.
 */
void *
xnvme_be_upcie_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys)
{
	size_t offset = 0;
	void *buf;
	int err;

	if (g_upcie_rte.connection.alive) {
		struct xnvme_be_upcie_state *state = dev ? (void *)dev->be.state : NULL;

		return xnvme_be_upcie_cplane_buf_alloc(state ? state->ctrlr : NULL, nbytes, phys);
	}

	err = dmamem_heap_alloc(&g_upcie_rte.mem.heap, nbytes, &offset);
	if (err) {
		errno = -err;
		return NULL;
	}

	buf = dmamem_heap_at_va(&g_upcie_rte.mem.heap, offset);
	if (!buf) {
		dmamem_heap_free(&g_upcie_rte.mem.heap, offset);
		errno = EFAULT;
		return NULL;
	}

	if (phys) {
		*phys = dmamem_heap_at_iova(&g_upcie_rte.mem.heap, offset);
	}

	return buf;
}

void
xnvme_be_upcie_buf_free(const struct xnvme_dev *dev, void *buf)
{
	size_t offset;

	if (!buf) {
		return;
	}

	if (g_upcie_rte.connection.alive) {
		struct xnvme_be_upcie_state *state = dev ? (void *)dev->be.state : NULL;

		xnvme_be_upcie_cplane_buf_free(state ? state->ctrlr : NULL, buf);
		return;
	}

	offset = (size_t)((char *)buf - (char *)g_upcie_rte.mem.dmem.cpu_va);
	dmamem_heap_free(&g_upcie_rte.mem.heap, offset);
}

int
xnvme_be_upcie_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf, uint64_t *phys)
{
	*phys = dmamem_va_to_iova(&g_upcie_rte.mem.dmem, buf);

	return 0;
}

/**
 * Register caller memory with a registry-backed dmamem
 *
 * Shared by the GPU backends, which differ only in which dmamem the memory
 * belongs to.
 *
 * @return 0 on success, negative errno on failure.
 */
int
xnvme_be_upcie_dmamem_map(struct dmamem *dmem, void *vaddr, size_t nbytes, uint64_t *phys)
{
	int err;

	err = dmamem_register(dmem, vaddr, nbytes);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_register(); err(%d)", err);
		return err;
	}

	if (phys) {
		*phys = dmamem_va_to_iova(dmem, vaddr);
		if (!*phys) {
			XNVME_DEBUG("FAILED: registered but unresolvable; vaddr(%p)", vaddr);
			err = dmamem_unregister(dmem, vaddr);
			if (err) {
				XNVME_DEBUG("FAILED: dmamem_unregister(); err(%d)", err);
			}
			return -EINVAL;
		}
	}

	return 0;
}

int
xnvme_be_upcie_dmamem_unmap(struct dmamem *dmem, void *vaddr)
{
	return dmamem_unregister(dmem, vaddr);
}

static int
_adopt_desc(struct dmamem *dmem, void *vaddr, const struct hostmem_shared_desc *desc,
	    uint32_t page_size)
{
	uint64_t *lut = NULL;
	size_t nlut;
	int shift = 0, err;

	while (((size_t)1 << shift) < page_size) {
		++shift;
	}

	if (desc->kind == HOSTMEM_SHARED_LUT) {
		return dmamem_registry_adopt(&dmem->registry, vaddr, desc->nbytes, desc->phys,
					     (int)desc->gran_shift, NULL);
	}
	if (desc->kind != HOSTMEM_SHARED_ARITHMETIC) {
		return -EPROTO;
	}

	nlut = (desc->nbytes + page_size - 1) >> shift;
	lut = calloc(nlut, sizeof(*lut));
	if (!lut) {
		return -ENOMEM;
	}
	for (size_t i = 0; i < nlut; ++i) {
		lut[i] = desc->base_addr + ((uint64_t)i << shift);
	}

	err = dmamem_registry_adopt(&dmem->registry, vaddr, desc->nbytes, lut, shift, NULL);
	free(lut);

	return err;
}

int
xnvme_be_upcie_dmem_from_desc(struct dmamem *dmem, void *base,
			      const struct hostmem_shared_desc *desc, enum dmamem_backing backing,
			      uint32_t page_size)
{
	int err;

	if (!dmem || !base || !desc || !xnvme_is_pow2(page_size)) {
		return -EINVAL;
	}
	if (desc->version != HOSTMEM_SHARED_DESC_VERSION) {
		XNVME_DEBUG("FAILED: description version(%u), expected(%u)", desc->version,
			    HOSTMEM_SHARED_DESC_VERSION);
		return -EPROTO;
	}

	memset(dmem, 0, sizeof(*dmem));

	err = dmamem_registry_init(&dmem->registry, page_size, xnvme_be_upcie_va_bits(), NULL,
				   NULL, NULL, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_registry_init(); err(%d)", err);
		return err;
	}

	err = _adopt_desc(dmem, base, desc, page_size);
	if (err) {
		XNVME_DEBUG("FAILED: adopting the heap description; err(%d)", err);
		dmamem_registry_term(&dmem->registry);
		return err;
	}

	dmem->fd = -1;
	dmem->cpu_va = (backing == DMAMEM_BACKING_HOSTMEM) ? base : NULL;
	dmem->base_va = base;
	dmem->size = desc->nbytes;
	dmem->backing = backing;
	dmem->translator = DMAMEM_XLATE_LUT;
	dmem->owned = 0;

	return 0;
}

static struct {
	struct {
		void *vaddr;
		struct xnvme_be_upcie_ctrlr *ctrlr;
		uint64_t offset;
	} *entries;
	int count;
	int capacity;
} g_served_regs;

static int
_served_regs_add(void *vaddr, struct xnvme_be_upcie_ctrlr *ctrlr, uint64_t offset)
{
	if (g_served_regs.count == g_served_regs.capacity) {
		int capacity = g_served_regs.capacity ? 2 * g_served_regs.capacity : 16;
		void *grown = realloc(g_served_regs.entries,
				      (size_t)capacity * sizeof(*g_served_regs.entries));

		if (!grown) {
			return -ENOMEM;
		}
		g_served_regs.entries = grown;
		g_served_regs.capacity = capacity;
	}

	g_served_regs.entries[g_served_regs.count].vaddr = vaddr;
	g_served_regs.entries[g_served_regs.count].ctrlr = ctrlr;
	g_served_regs.entries[g_served_regs.count].offset = offset;
	g_served_regs.count++;

	return 0;
}

static int
_served_regs_find(void *vaddr, struct xnvme_be_upcie_ctrlr *ctrlr)
{
	for (int i = 0; i < g_served_regs.count; ++i) {
		if ((g_served_regs.entries[i].vaddr == vaddr) &&
		    (g_served_regs.entries[i].ctrlr == ctrlr)) {
			return i;
		}
	}

	return -1;
}

int
xnvme_be_upcie_served_mem_map(struct dmamem *dmem, struct xnvme_be_upcie_ctrlr *ctrlr,
			      int dmabuf_fd, void *vaddr, size_t nbytes, uint32_t page_size)
{
	const struct hostmem_shared_desc *desc = NULL;
	uint64_t offset = 0;
	int err;

	if (!dmem || !ctrlr || (dmabuf_fd < 0) || !vaddr || !nbytes || !page_size) {
		return -EINVAL;
	}
	if (dmem->translator != DMAMEM_XLATE_LUT) {
		return -EOPNOTSUPP;
	}
	if (((uintptr_t)vaddr & (page_size - 1)) || (nbytes & (page_size - 1))) {
		XNVME_DEBUG("FAILED: vaddr(%p) nbytes(%zu) not aligned to page_size(%u)", vaddr,
			    nbytes, page_size);
		return -EINVAL;
	}

	err = xnvme_be_upcie_cplane_register_client_mem(ctrlr, dmabuf_fd, nbytes, page_size, &desc,
							&offset);
	if (err) {
		XNVME_DEBUG("FAILED: registering %zu bytes with the server; err(%d)", nbytes, err);
		return err;
	}

	err = _adopt_desc(dmem, vaddr, desc, page_size);
	if (!err) {
		err = _served_regs_add(vaddr, ctrlr, offset);
		if (err) {
			dmamem_registry_remove(&dmem->registry, vaddr);
		}
	}

	if (err) {
		XNVME_DEBUG("FAILED: adopting the registration; err(%d)", err);
		xnvme_be_upcie_cplane_unregister_client_mem(ctrlr, offset);
	}

	return err;
}

int
xnvme_be_upcie_served_mem_unmap(struct dmamem *dmem, struct xnvme_be_upcie_ctrlr *ctrlr,
				void *vaddr)
{
	uint64_t offset;
	int at, err;

	if (!dmem || !ctrlr || !vaddr) {
		return -EINVAL;
	}

	at = _served_regs_find(vaddr, ctrlr);
	if (at < 0) {
		return -EINVAL;
	}
	offset = g_served_regs.entries[at].offset;
	g_served_regs.entries[at] = g_served_regs.entries[--g_served_regs.count];
	dmamem_registry_remove(&dmem->registry, vaddr);

	err = xnvme_be_upcie_cplane_unregister_client_mem(ctrlr, offset);
	if (err) {
		XNVME_DEBUG("FAILED: the server kept the registration; err(%d)", err);
	}

	return err;
}

/* The heap sits at an IOVA equal to its address, so any memory mapped that way translates alike */
static int
_heap_at_va(void)
{
	const struct dmamem *dmem = &g_upcie_rte.mem.dmem;

	return g_upcie_rte.mem.dmem_alive && (dmem->translator == DMAMEM_XLATE_ARITHMETIC) &&
	       (dmem->base_iova == (uint64_t)dmem->base_va);
}

struct iommufd *
xnvme_be_upcie_mem_map_ioas(void)
{
	if (!_heap_at_va()) {
		return NULL;
	}
	if (g_upcie_rte.connection.alive) {
		return g_upcie_rte.connection.iommufd_alive ? &g_upcie_rte.connection.iommufd
							    : NULL;
	}

	return (g_upcie_rte.mode == XNVME_BE_UPCIE_MODE_VFIO_CDEV) ? &g_upcie_rte.cdev.iommufd
								   : NULL;
}

struct vfio_container *
xnvme_be_upcie_mem_map_container(void)
{
	if (!_heap_at_va()) {
		return NULL;
	}
	if (g_upcie_rte.connection.alive) {
		return g_upcie_rte.connection.container_alive ? &g_upcie_rte.connection.container
							      : NULL;
	}

	return ((g_upcie_rte.mode == XNVME_BE_UPCIE_MODE_VFIO_TYPE1) &&
		g_upcie_rte.type1.iommu_set)
		       ? &g_upcie_rte.type1.container
		       : NULL;
}

/**
 * How memory of this process's own is made reachable, if at all
 *
 * Through an IOMMU it is mapped at an IOVA equal to its address, in this
 * process's IOAS or container or the server's. Without one the device
 * takes physical addresses, which the heap's registry reads from pagemap.
 */
enum _map_via {
	_MAP_VIA_NONE,
	_MAP_VIA_IOMMUFD,
	_MAP_VIA_TYPE1,
	_MAP_VIA_REGISTRY,
};

static enum _map_via
_map_via(void)
{
	const struct dmamem *dmem = &g_upcie_rte.mem.dmem;

	if (xnvme_be_upcie_mem_map_ioas()) {
		return _MAP_VIA_IOMMUFD;
	}
	if (xnvme_be_upcie_mem_map_container()) {
		return _MAP_VIA_TYPE1;
	}
	if (g_upcie_rte.mem.dmem_alive && (dmem->translator == DMAMEM_XLATE_LUT) &&
	    dmem->registry.populate) {
		return _MAP_VIA_REGISTRY;
	}

	return _MAP_VIA_NONE;
}

static int
_iommu_map(enum _map_via via, uint64_t va, uint64_t nbytes)
{
	if (via == _MAP_VIA_TYPE1) {
		struct vfio_iommu_type1_dma_map map = {0};

		map.argsz = sizeof(map);
		map.flags = VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE;
		map.vaddr = va;
		map.iova = va;
		map.size = nbytes;

		return vfio_iommu_map_dma(xnvme_be_upcie_mem_map_container(), &map) ? -errno : 0;
	}

	return iommufd_ioas_map(xnvme_be_upcie_mem_map_ioas(), va, nbytes,
				IOMMU_IOAS_MAP_FIXED_IOVA | IOMMU_IOAS_MAP_READABLE |
					IOMMU_IOAS_MAP_WRITEABLE,
				&va);
}

static int
_iommu_unmap(enum _map_via via, uint64_t va, uint64_t nbytes)
{
	if (via == _MAP_VIA_TYPE1) {
		struct vfio_iommu_type1_dma_unmap unmap = {0};

		unmap.argsz = sizeof(unmap);
		unmap.iova = va;
		unmap.size = nbytes;

		if (vfio_iommu_unmap_dma(xnvme_be_upcie_mem_map_container(), &unmap)) {
			return -errno;
		}

		return 0;
	}

	return iommufd_ioas_unmap(xnvme_be_upcie_mem_map_ioas(), va, nbytes);
}

/* Mapped once however many devices map it, since they share the IOAS or container */
static struct {
	struct {
		void *vaddr;
		size_t nbytes;
		int refs;
	} *entries;
	int count;
	int capacity;
} g_maps;

static int
_maps_find(void *vaddr)
{
	for (int i = 0; i < g_maps.count; ++i) {
		if (g_maps.entries[i].vaddr == vaddr) {
			return i;
		}
	}

	return -1;
}

static int
_maps_grow(void)
{
	int capacity;
	void *grown;

	if (g_maps.count < g_maps.capacity) {
		return 0;
	}

	capacity = g_maps.capacity ? 2 * g_maps.capacity : 16;
	grown = realloc(g_maps.entries, (size_t)capacity * sizeof(*g_maps.entries));
	if (!grown) {
		return -ENOMEM;
	}
	g_maps.entries = grown;
	g_maps.capacity = capacity;

	return 0;
}

void
xnvme_be_upcie_mem_maps_term(void)
{
	const enum _map_via via = _map_via();

	/* A server's address space outlives this process, so what is left must go */
	if ((via == _MAP_VIA_IOMMUFD) || (via == _MAP_VIA_TYPE1)) {
		for (int i = 0; i < g_maps.count; ++i) {
			_iommu_unmap(via, (uint64_t)g_maps.entries[i].vaddr,
				     g_maps.entries[i].nbytes);
		}
	}
	free(g_maps.entries);
	memset(&g_maps, 0, sizeof(g_maps));
}

int
xnvme_be_upcie_mem_map(const struct xnvme_dev *XNVME_UNUSED(dev), void *vaddr, size_t nbytes,
		       uint64_t *phys)
{
	const uint64_t pagesize = (uint64_t)sysconf(_SC_PAGESIZE);
	const enum _map_via via = _map_via();
	int at, err;

	switch (via) {
	case _MAP_VIA_NONE:
		return -ENOSYS;

	case _MAP_VIA_REGISTRY:
		/* The registry counts registrations of the same memory itself */
		return xnvme_be_upcie_dmamem_map(&g_upcie_rte.mem.dmem, vaddr, nbytes, phys);

	case _MAP_VIA_IOMMUFD:
	case _MAP_VIA_TYPE1:
		break;
	}

	if (!vaddr || !nbytes || ((uint64_t)vaddr & (pagesize - 1)) || (nbytes & (pagesize - 1))) {
		XNVME_DEBUG("FAILED: vaddr(%p) nbytes(%zu) not aligned to the page size", vaddr,
			    nbytes);
		return -EINVAL;
	}

	at = _maps_find(vaddr);
	if (at >= 0) {
		if (g_maps.entries[at].nbytes != nbytes) {
			XNVME_DEBUG("FAILED: vaddr(%p) is mapped with nbytes(%zu)", vaddr,
				    g_maps.entries[at].nbytes);
			return -EEXIST;
		}
		g_maps.entries[at].refs += 1;
		goto mapped;
	}

	err = _maps_grow();
	if (err) {
		return err;
	}

	err = _iommu_map(via, (uint64_t)vaddr, nbytes);
	if (err) {
		XNVME_DEBUG("FAILED: mapping vaddr(%p) nbytes(%zu); err(%d)", vaddr, nbytes, err);
		return err;
	}

	g_maps.entries[g_maps.count].vaddr = vaddr;
	g_maps.entries[g_maps.count].nbytes = nbytes;
	g_maps.entries[g_maps.count].refs = 1;
	g_maps.count++;

mapped:
	if (phys) {
		*phys = dmamem_va_to_iova(&g_upcie_rte.mem.dmem, vaddr);
	}

	return 0;
}

int
xnvme_be_upcie_mem_unmap(const struct xnvme_dev *XNVME_UNUSED(dev), void *vaddr)
{
	const enum _map_via via = _map_via();
	size_t nbytes;
	int at, err;

	switch (via) {
	case _MAP_VIA_NONE:
		return -ENOSYS;

	case _MAP_VIA_REGISTRY:
		return xnvme_be_upcie_dmamem_unmap(&g_upcie_rte.mem.dmem, vaddr);

	case _MAP_VIA_IOMMUFD:
	case _MAP_VIA_TYPE1:
		break;
	}

	at = _maps_find(vaddr);
	if (at < 0) {
		return -EINVAL;
	}
	if (--g_maps.entries[at].refs) {
		return 0;
	}
	nbytes = g_maps.entries[at].nbytes;
	g_maps.entries[at] = g_maps.entries[--g_maps.count];

	err = _iommu_unmap(via, (uint64_t)vaddr, nbytes);
	if (err) {
		XNVME_DEBUG("FAILED: unmapping vaddr(%p); err(%d)", vaddr, err);
	}

	return err;
}

int
xnvme_be_upcie_mem_map_req(const struct xnvme_dev *XNVME_UNUSED(dev),
			   struct xnvme_mem_map_req *req)
{
	switch (_map_via()) {
	case _MAP_VIA_NONE:
		return -ENOSYS;

	case _MAP_VIA_REGISTRY:
		/* Whole hugepages, see dmamem_hostmem_pagemap_populate() */
		req->align = g_upcie_rte.mem.dmem.registry.gran_mask + 1;
		return 0;

	case _MAP_VIA_IOMMUFD:
	case _MAP_VIA_TYPE1:
		req->align = (uint64_t)sysconf(_SC_PAGESIZE);
		return 0;
	}

	return -ENOSYS;
}

#endif

struct xnvme_be_mem g_xnvme_be_upcie_mem = {
	.id = "upcie",
#ifdef XNVME_BE_UPCIE_ENABLED
	.buf_alloc = xnvme_be_upcie_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_upcie_buf_free,
	.buf_vtophys = xnvme_be_upcie_buf_vtophys,
	.mem_map = xnvme_be_upcie_mem_map,
	.mem_unmap = xnvme_be_upcie_mem_unmap,
	.mem_map_req = xnvme_be_upcie_mem_map_req,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#endif
};
