// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_buf.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_cplane_client.h>
#include <xnvme_be_upcie_cplane_serve_mem.h>
#include <xnvme_be_state.h>
#include <xnvme_dev.h>

/**
 * Allocate a buffer from the RTE's dmamem_heap.
 *
 * Offsets into the heap resolve to device addresses through the dmamem
 * translator, so the same allocation works whichever way the target is
 * attached; the caller hands the returned VA to the device as a PRP later.
 */
static void *
xnvme_be_upcie_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys)
{
	size_t offset = 0;
	void *buf;
	int err;

	if (g_upcie_rte.connection.alive) {
		struct xnvme_be_upcie_state *state =
			dev ? &xnvme_be_state(dev)->upcie_state : NULL;

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

static void
xnvme_be_upcie_buf_free(const struct xnvme_dev *dev, void *buf)
{
	size_t offset;

	if (!buf) {
		return;
	}

	if (g_upcie_rte.connection.alive) {
		struct xnvme_be_upcie_state *state =
			dev ? &xnvme_be_state(dev)->upcie_state : NULL;

		xnvme_be_upcie_cplane_buf_free(state ? state->ctrlr : NULL, buf);
		return;
	}

	offset = (size_t)((char *)buf - (char *)g_upcie_rte.mem.dmem.cpu_va);
	dmamem_heap_free(&g_upcie_rte.mem.heap, offset);
}

static int
xnvme_be_upcie_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf, uint64_t *phys)
{
	*phys = dmamem_va_to_iova(xnvme_be_upcie_dmem_of(&g_upcie_rte.mem.dmem, buf), buf);

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

/** Whether `desc` puts the first byte at the address the range adopted at `vaddr` has */
static int
_desc_matches(struct dmamem *dmem, void *vaddr, const struct hostmem_shared_desc *desc,
	      uint32_t page_size)
{
	uint64_t addr;

	return !hostmem_shared_desc_addr(desc, 0, page_size, &addr) &&
	       (addr == dmamem_va_to_iova(dmem, vaddr));
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
		struct dmamem *dmem;
		void *vaddr;
		struct xnvme_be_upcie_ctrlr *ctrlr;
		uint64_t offset;
	} *entries;
	int count;
	int capacity;
} g_served_regs;

static int
_served_regs_add(struct dmamem *dmem, void *vaddr, struct xnvme_be_upcie_ctrlr *ctrlr,
		 uint64_t offset)
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

	g_served_regs.entries[g_served_regs.count].dmem = dmem;
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

static int
_served_regs_find_dmem(const struct dmamem *dmem, const void *vaddr)
{
	for (int i = 0; i < g_served_regs.count; ++i) {
		if ((g_served_regs.entries[i].dmem == dmem) &&
		    (g_served_regs.entries[i].vaddr == vaddr)) {
			return i;
		}
	}

	return -1;
}

int
xnvme_be_upcie_served_mem_reaches(struct xnvme_be_upcie_ctrlr *ctrlr, void *vaddr)
{
	return _served_regs_find(vaddr, ctrlr) >= 0;
}

int
xnvme_be_upcie_served_mem_map(struct dmamem *dmem, struct xnvme_be_upcie_ctrlr *ctrlr,
			      int dmabuf_fd, void *vaddr, size_t nbytes, uint32_t page_size)
{
	const struct hostmem_shared_desc *desc = NULL;
	uint64_t offset = 0;
	int adopted, err;

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

	/* Through a further controller the server installs the same dma-buf at the IOVA it gave
	 * the first, so the table has it already. Another dma-buf of the same memory can be mapped
	 * elsewhere, and the table holds one translation per vaddr. */
	adopted = _served_regs_find_dmem(dmem, vaddr) >= 0;
	if (adopted) {
		err = _desc_matches(dmem, vaddr, desc, page_size) ? 0 : -EBUSY;
	} else {
		err = _adopt_desc(dmem, vaddr, desc, page_size);
	}
	if (!err) {
		err = _served_regs_add(dmem, vaddr, ctrlr, offset);
		if (err && !adopted) {
			dmamem_registry_remove(&dmem->registry, vaddr);
		}
	}

	if (err) {
		XNVME_DEBUG("FAILED: adopting the registration; err(%d)", err);
		xnvme_be_upcie_cplane_unregister_client_mem(ctrlr, offset);
	}

	return err;
}

static int
_served_regs_unmap_at(struct dmamem *dmem, int at)
{
	struct xnvme_be_upcie_ctrlr *ctrlr = g_served_regs.entries[at].ctrlr;
	void *vaddr = g_served_regs.entries[at].vaddr;
	uint64_t offset = g_served_regs.entries[at].offset;
	int err;

	g_served_regs.entries[at] = g_served_regs.entries[--g_served_regs.count];
	if (_served_regs_find_dmem(dmem, vaddr) < 0) {
		dmamem_registry_remove(&dmem->registry, vaddr);
	}

	err = xnvme_be_upcie_cplane_unregister_client_mem(ctrlr, offset);
	if (err) {
		XNVME_DEBUG("FAILED: the server kept the registration; err(%d)", err);
	}

	return err;
}

int
xnvme_be_upcie_served_mem_unmap(struct dmamem *dmem, void *vaddr)
{
	int found = 0, err = 0;

	if (!dmem || !vaddr) {
		return -EINVAL;
	}

	for (int at = g_served_regs.count - 1; at >= 0; --at) {
		int ret;

		if ((g_served_regs.entries[at].dmem != dmem) ||
		    (g_served_regs.entries[at].vaddr != vaddr)) {
			continue;
		}

		found = 1;
		ret = _served_regs_unmap_at(dmem, at);
		err = err ? err : ret;
	}

	return found ? err : -EINVAL;
}

void
xnvme_be_upcie_served_mem_unmap_all(struct dmamem *dmem, struct xnvme_be_upcie_ctrlr *ctrlr)
{
	for (int at = g_served_regs.count - 1; at >= 0; --at) {
		void *vaddr = g_served_regs.entries[at].vaddr;
		struct xnvme_buf_reg *reg;

		if ((g_served_regs.entries[at].dmem != dmem) ||
		    (ctrlr && (g_served_regs.entries[at].ctrlr != ctrlr))) {
			continue;
		}

		_served_regs_unmap_at(dmem, at);
		if ((_served_regs_find_dmem(dmem, vaddr) < 0) &&
		    !xnvme_buf_reg_find(dmem, vaddr, 1, &reg)) {
			xnvme_buf_reg_remove(reg);
		}
	}
}

static int
_in_heap(uint64_t heap, size_t heap_nbytes, void *vaddr, size_t nbytes)
{
	uint64_t va = (uint64_t)(uintptr_t)vaddr;

	return vaddr && (va >= heap) && (nbytes <= heap_nbytes) &&
	       ((va - heap) <= (heap_nbytes - nbytes));
}

int
xnvme_be_upcie_buf_reg_ref(struct dmamem *dmem, uint64_t heap, size_t heap_nbytes, void *vaddr,
			   size_t nbytes)
{
	struct xnvme_buf_reg *reg;
	int err;

	err = xnvme_buf_reg_find(dmem, vaddr, nbytes, &reg);
	if (!err) {
		reg->refs += 1;
		return 0;
	}
	if (err != -ENOENT) {
		return err;
	}

	if (_in_heap(heap, heap_nbytes, vaddr, nbytes)) {
		XNVME_DEBUG("INFO: vaddr(%p) is in the heap, as from xnvme_buf_alloc()", vaddr);
		return 0;
	}

	return -ENOENT;
}

int
xnvme_be_upcie_dmabuf_check_base(struct dmamem *dmem, void *vaddr, size_t nbytes)
{
	struct xnvme_buf_reg *reg;

	if (!xnvme_buf_reg_find(dmem, vaddr, nbytes, &reg) && (reg->vaddr != vaddr)) {
		XNVME_DEBUG("FAILED: vaddr(%p) is inside the range registered at %p", vaddr,
			    reg->vaddr);
		return -EBUSY;
	}

	return 0;
}

int
xnvme_be_upcie_buf_reg_unref(struct dmamem *dmem, uint64_t heap, size_t heap_nbytes, void *vaddr,
			     struct xnvme_buf_reg **last)
{
	struct xnvme_buf_reg *reg;
	int err;

	*last = NULL;

	err = xnvme_buf_reg_find(dmem, vaddr, 1, &reg);
	if ((err == -ENOENT) && _in_heap(heap, heap_nbytes, vaddr, 1)) {
		XNVME_DEBUG("INFO: vaddr(%p) was not registered, as from xnvme_buf_alloc()",
			    vaddr);
		return 0;
	}
	if (err) {
		XNVME_DEBUG("FAILED: vaddr(%p) is not registered", vaddr);
		return -EINVAL;
	}

	if (reg->refs > 1) {
		reg->refs -= 1;
		return 0;
	}
	*last = reg;

	return 0;
}

/* The heap sits at an IOVA equal to its address, so any memory mapped that way translates alike */
static int
_heap_at_va(void)
{
	const struct dmamem *dmem = &g_upcie_rte.mem.dmem;

	return g_upcie_rte.mem.dmem_alive && (dmem->translator == DMAMEM_XLATE_ARITHMETIC) &&
	       (dmem->base_iova == (uint64_t)dmem->base_va);
}

int
xnvme_be_upcie_mem_map_iommu(struct nvme_cplane_iommu_handle *iommu)
{
	if (!_heap_at_va()) {
		return 0;
	}
	if (g_upcie_rte.connection.alive) {
		*iommu = g_upcie_rte.connection.iommu;
		return iommu->kind != NVME_CPLANE_IOMMU_NONE;
	}

	switch (g_upcie_rte.mode) {
	case XNVME_BE_UPCIE_MODE_VFIO_CDEV:
		iommu->fd = g_upcie_rte.cdev.iommufd.fd;
		iommu->ioas_id = g_upcie_rte.cdev.iommufd.ioas_id;
		iommu->kind = NVME_CPLANE_IOMMU_IOMMUFD;
		return 1;

	case XNVME_BE_UPCIE_MODE_VFIO_TYPE1:
		if (!g_upcie_rte.type1.iommu_set) {
			return 0;
		}
		iommu->fd = g_upcie_rte.type1.container.fd;
		iommu->ioas_id = 0;
		iommu->kind = NVME_CPLANE_IOMMU_TYPE1;
		return 1;

	default:
		return 0;
	}
}

/**
 * How memory of this process's own is made reachable, if at all
 *
 * Through an IOMMU it is mapped at an IOVA equal to its address, in this
 * process's IOAS or container or the server's. Without one the device takes physical
 * addresses, which the heap's registry reads from pagemap.
 */
enum _map_via {
	_MAP_VIA_NONE,
	_MAP_VIA_IOMMU,
	_MAP_VIA_REGISTRY,
};

static enum _map_via
_map_via(struct nvme_cplane_iommu_handle *iommu)
{
	const struct dmamem *dmem = &g_upcie_rte.mem.dmem;

	if (xnvme_be_upcie_mem_map_iommu(iommu)) {
		return _MAP_VIA_IOMMU;
	}
	if (g_upcie_rte.mem.dmem_alive && (dmem->translator == DMAMEM_XLATE_LUT) &&
	    dmem->registry.populate) {
		return _MAP_VIA_REGISTRY;
	}

	return _MAP_VIA_NONE;
}

static int
_map(void *vaddr, size_t nbytes)
{
	struct nvme_cplane_iommu_handle iommu;

	switch (_map_via(&iommu)) {
	case _MAP_VIA_IOMMU:
		return nvme_cplane_iommu_map(&iommu, vaddr, nbytes, (uint64_t)vaddr);

	case _MAP_VIA_REGISTRY:
		return xnvme_be_upcie_dmamem_map(&g_upcie_rte.mem.dmem, vaddr, nbytes, NULL);

	case _MAP_VIA_NONE:
		break;
	}

	return -ENOSYS;
}

static int
_unmap(void *vaddr, size_t nbytes)
{
	struct nvme_cplane_iommu_handle iommu;

	switch (_map_via(&iommu)) {
	case _MAP_VIA_IOMMU:
		return nvme_cplane_iommu_unmap(&iommu, (uint64_t)vaddr, nbytes);

	case _MAP_VIA_REGISTRY:
		return xnvme_be_upcie_dmamem_unmap(&g_upcie_rte.mem.dmem, vaddr);

	case _MAP_VIA_NONE:
		break;
	}

	return -ENOSYS;
}

void
xnvme_be_upcie_buf_regs_term(void)
{
	struct xnvme_buf_reg *reg;

	while ((reg = xnvme_buf_reg_any(&g_upcie_rte.mem.dmem))) {
		/* A server's address space outlives this process, so what is left must go */
		if (g_upcie_rte.connection.alive) {
			_unmap(reg->vaddr, reg->nbytes);
		}
		xnvme_buf_reg_remove(reg);
	}
}

/* The smallest page the IOMMU maps, where larger than the CPU's */
static int
_iommu_pagesize(const struct nvme_cplane_iommu_handle *iommu, size_t *pagesize)
{
	uint64_t granule = 0;

	if (iommu->kind == NVME_CPLANE_IOMMU_TYPE1) {
		struct vfio_container container = {.fd = iommu->fd};
		struct vfio_iommu_type1_info info;

		if (vfio_iommu_get_info(&container, &info)) {
			XNVME_DEBUG("FAILED: vfio_iommu_get_info(); errno(%d)", errno);
			return -errno;
		}
		if (info.flags & VFIO_IOMMU_INFO_PGSIZES) {
			granule = info.iova_pgsizes & (~info.iova_pgsizes + 1);
		}
	} else {
		struct iommufd ctx = {.fd = iommu->fd, .ioas_id = iommu->ioas_id};
		struct iommu_iova_range ranges[16];
		uint32_t nranges;
		int err;

		err = iommufd_ioas_iova_ranges(&ctx, ranges, 16, &nranges, &granule);
		if (err) {
			XNVME_DEBUG("FAILED: iommufd_ioas_iova_ranges(); err(%d)", err);
			return err;
		}
	}

	*pagesize = (size_t)sysconf(_SC_PAGESIZE);
	if (granule > *pagesize) {
		*pagesize = (size_t)granule;
	}

	return 0;
}

static int
xnvme_be_upcie_buf_get_alignment(const struct xnvme_dev *XNVME_UNUSED(dev), size_t *align)
{
	struct nvme_cplane_iommu_handle iommu;

	switch (_map_via(&iommu)) {
	case _MAP_VIA_NONE:
		return -ENOSYS;

	case _MAP_VIA_REGISTRY:
		/* Whole hugepages, see dmamem_hostmem_pagemap_populate() */
		*align = g_upcie_rte.mem.dmem.registry.gran_mask + 1;
		return 0;

	case _MAP_VIA_IOMMU:
		break;
	}

	return _iommu_pagesize(&iommu, align);
}

/* Mapped once however many devices register it, since they share the IOAS or container */
static int
xnvme_be_upcie_buf_register(const struct xnvme_dev *dev, void *vaddr, size_t nbytes)
{
	struct dmamem *dmem = &g_upcie_rte.mem.dmem;
	struct nvme_cplane_iommu_handle iommu;
	const enum _map_via via = _map_via(&iommu);
	size_t align;
	int err;

	if (via == _MAP_VIA_NONE) {
		return -ENOSYS;
	}

	err = xnvme_be_upcie_buf_reg_ref(dmem, (uint64_t)dmem->base_va, dmem->size, vaddr, nbytes);
	if (err != -ENOENT) {
		return err;
	}

	err = xnvme_be_upcie_buf_get_alignment(dev, &align);
	if (err) {
		return err;
	}
	if (!vaddr || !nbytes || ((uintptr_t)vaddr & (align - 1)) || (nbytes & (align - 1))) {
		XNVME_DEBUG("FAILED: vaddr(%p) nbytes(%zu) not aligned to align(%zu)", vaddr,
			    nbytes, align);
		return -EINVAL;
	}
	if (via == _MAP_VIA_IOMMU) {
		err = xnvme_be_upcie_iova_window_clear((uint64_t)vaddr, nbytes);
		if (err) {
			return err;
		}
	}

	err = _map(vaddr, nbytes);
	if (err) {
		XNVME_DEBUG("FAILED: mapping vaddr(%p) nbytes(%zu); err(%d)", vaddr, nbytes, err);
		return err;
	}

	err = xnvme_buf_reg_add(dmem, vaddr, nbytes, 0);
	if (err) {
		_unmap(vaddr, nbytes);
	}

	return err;
}

static int
_host_unregister(const struct xnvme_dev *XNVME_UNUSED(dev), void *vaddr)
{
	struct dmamem *dmem = &g_upcie_rte.mem.dmem;
	struct xnvme_buf_reg *last;
	int err;

	err = xnvme_be_upcie_buf_reg_unref(dmem, (uint64_t)dmem->base_va, dmem->size, vaddr,
					   &last);
	if (err || !last) {
		return err;
	}

	err = _unmap(last->vaddr, last->nbytes);
	if (err) {
		XNVME_DEBUG("FAILED: unmapping vaddr(%p); err(%d)", last->vaddr, err);
		return err;
	}
	xnvme_buf_reg_remove(last);

	return 0;
}

/**
 * The dma-bufs registered on the upcie interface share one dmamem that translates by its registry,
 * since the heap's dmamem translates the heap alone. Without a server, the process maps them as a
 * server maps a client's, and g_dmabuf_local records what it mapped through each controller.
 */
static struct dmamem g_dmabuf_dmem;
static int g_dmabuf_dmem_alive;

static struct {
	struct {
		void *vaddr;
		struct xnvme_be_upcie_ctrlr *ctrlr;
		struct xnvme_be_upcie_cplane_registration reg;
		int alive;
	} *entries;
	int capacity;
} g_dmabuf_local;

/**
 * Reads the registry's table rather than walking its list of registrations, which costs a command
 * a lookup per registration and which xnvme_buf_unregister() on another device changes
 */
struct dmamem *
xnvme_be_upcie_dmem_of(struct dmamem *dmem, void *buf)
{
	const struct dmamem_registry *registry = &g_dmabuf_dmem.registry;
	uint64_t slot;

	if (!g_dmabuf_dmem_alive) {
		return dmem;
	}

	slot = (uint64_t)(uintptr_t)buf >> registry->gran_shift;
	if ((slot < registry->lut_capacity) && registry->lut_phys[slot]) {
		return &g_dmabuf_dmem;
	}

	return dmem;
}

struct dmamem *
xnvme_be_upcie_dmem_of_iov(struct dmamem *dmem, struct iovec *dvec, size_t dvec_cnt)
{
	struct dmamem *first = xnvme_be_upcie_dmem_of(dmem, dvec[0].iov_base);

	for (size_t i = 1; i < dvec_cnt; ++i) {
		if (xnvme_be_upcie_dmem_of(dmem, dvec[i].iov_base) != first) {
			return NULL;
		}
	}

	return first;
}

static int
_dmabuf_dmem_init(void)
{
	int err;

	if (g_dmabuf_dmem_alive) {
		return 0;
	}

	memset(&g_dmabuf_dmem, 0, sizeof(g_dmabuf_dmem));
	err = dmamem_registry_init(&g_dmabuf_dmem.registry, 4096, xnvme_be_upcie_va_bits(), NULL,
				   NULL, NULL, NULL);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_registry_init(); err(%d)", err);
		return err;
	}
	g_dmabuf_dmem.fd = -1;
	g_dmabuf_dmem.backing = DMAMEM_BACKING_DMABUF;
	g_dmabuf_dmem.translator = DMAMEM_XLATE_LUT;
	g_dmabuf_dmem_alive = 1;

	return 0;
}

/** The entry of `vaddr` mapped through `ctrlr`, or through any controller where it is NULL */
static int
_local_find(void *vaddr, struct xnvme_be_upcie_ctrlr *ctrlr)
{
	for (int at = 0; at < g_dmabuf_local.capacity; ++at) {
		if (g_dmabuf_local.entries[at].alive &&
		    (g_dmabuf_local.entries[at].vaddr == vaddr) &&
		    (!ctrlr || (g_dmabuf_local.entries[at].ctrlr == ctrlr))) {
			return at;
		}
	}

	return -1;
}

static int
_local_map(const struct xnvme_dev *dev, int fd, void *vaddr, size_t nbytes)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;
	int adopted = _local_find(vaddr, NULL) >= 0;
	struct xnvme_be_upcie_cplane_registration *reg;
	const struct hostmem_shared_desc *desc;
	int at, err;

	for (at = 0; (at < g_dmabuf_local.capacity) && g_dmabuf_local.entries[at].alive; ++at) {
		;
	}
	if (at == g_dmabuf_local.capacity) {
		int capacity = g_dmabuf_local.capacity ? 2 * g_dmabuf_local.capacity : 16;
		void *grown = realloc(g_dmabuf_local.entries,
				      (size_t)capacity * sizeof(*g_dmabuf_local.entries));

		if (!grown) {
			return -ENOMEM;
		}
		g_dmabuf_local.entries = grown;
		memset(&g_dmabuf_local.entries[at], 0,
		       (size_t)(capacity - at) * sizeof(*g_dmabuf_local.entries));
		g_dmabuf_local.capacity = capacity;
	}
	reg = &g_dmabuf_local.entries[at].reg;

	err = xnvme_be_upcie_cplane_serve_register_mem(fd, nbytes, 4096, dev->ident.uri, reg);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_cplane_serve_register_mem(); err(%d)", err);
		return err;
	}
	desc = (const void *)((const char *)g_upcie_rte.mem.dmem.base_va + reg->desc_offset);

	/* Through a further controller the range keeps the addresses it was adopted with, which
	 * another dma-buf of the same memory need not be mapped at */
	if (adopted) {
		err = _desc_matches(&g_dmabuf_dmem, vaddr, desc, 4096) ? 0 : -EBUSY;
	} else {
		err = _adopt_desc(&g_dmabuf_dmem, vaddr, desc, 4096);
	}
	if (err) {
		XNVME_DEBUG("FAILED: adopting the registration; err(%d)", err);
		xnvme_be_upcie_cplane_serve_unregister_mem(reg);
		return err;
	}

	g_dmabuf_local.entries[at].vaddr = vaddr;
	g_dmabuf_local.entries[at].ctrlr = state->ctrlr;
	g_dmabuf_local.entries[at].alive = 1;

	return 0;
}

static void
_local_unmap_at(int at)
{
	void *vaddr = g_dmabuf_local.entries[at].vaddr;

	xnvme_be_upcie_cplane_serve_unregister_mem(&g_dmabuf_local.entries[at].reg);
	memset(&g_dmabuf_local.entries[at], 0, sizeof(g_dmabuf_local.entries[at]));
	if (_local_find(vaddr, NULL) < 0) {
		dmamem_registry_remove(&g_dmabuf_dmem.registry, vaddr);
	}
}

static int
_dmabuf_map(const struct xnvme_dev *dev, int fd, void *vaddr, size_t nbytes)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;

	if (g_upcie_rte.connection.alive) {
		return xnvme_be_upcie_served_mem_map(&g_dmabuf_dmem, state->ctrlr, fd, vaddr,
						     nbytes, 4096);
	}

	return _local_map(dev, fd, vaddr, nbytes);
}

static int
_dmabuf_reaches(const struct xnvme_dev *dev, void *vaddr)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;

	if (g_upcie_rte.connection.alive) {
		return xnvme_be_upcie_served_mem_reaches(state->ctrlr, vaddr);
	}

	return _local_find(vaddr, state->ctrlr) >= 0;
}

static int
_dmabuf_unmap(void *vaddr)
{
	int at;

	if (g_upcie_rte.connection.alive) {
		return xnvme_be_upcie_served_mem_unmap(&g_dmabuf_dmem, vaddr);
	}

	while ((at = _local_find(vaddr, NULL)) >= 0) {
		_local_unmap_at(at);
	}

	return 0;
}

void
xnvme_be_upcie_dmabufs_term(struct xnvme_be_upcie_ctrlr *ctrlr)
{
	struct xnvme_buf_reg *reg;

	if (!g_dmabuf_dmem_alive) {
		return;
	}

	if (g_upcie_rte.connection.alive) {
		xnvme_be_upcie_served_mem_unmap_all(&g_dmabuf_dmem, ctrlr);
	}
	for (int at = 0; at < g_dmabuf_local.capacity; ++at) {
		void *vaddr = g_dmabuf_local.entries[at].vaddr;

		if (!g_dmabuf_local.entries[at].alive ||
		    (ctrlr && (g_dmabuf_local.entries[at].ctrlr != ctrlr))) {
			continue;
		}

		_local_unmap_at(at);
		if ((_local_find(vaddr, NULL) < 0) &&
		    !xnvme_buf_reg_find(&g_dmabuf_dmem, vaddr, 1, &reg)) {
			xnvme_buf_reg_remove(reg);
		}
	}
	if (ctrlr) {
		return;
	}

	while ((reg = xnvme_buf_reg_any(&g_dmabuf_dmem))) {
		xnvme_buf_reg_remove(reg);
	}
	free(g_dmabuf_local.entries);
	memset(&g_dmabuf_local, 0, sizeof(g_dmabuf_local));
	dmamem_registry_term(&g_dmabuf_dmem.registry);
	g_dmabuf_dmem_alive = 0;
}

static int
xnvme_be_upcie_buf_register_dmabuf(const struct xnvme_dev *dev, int fd, void *vaddr)
{
	struct xnvme_buf_reg *reg;
	size_t nbytes;
	off_t size;
	int err;

	size = lseek(fd, 0, SEEK_END);
	if (size <= 0) {
		XNVME_DEBUG("FAILED: lseek(dma-buf), errno: %d", errno);
		return size ? -errno : -EINVAL;
	}
	nbytes = (size_t)size;
	if (((uintptr_t)vaddr | nbytes) & 4095) {
		XNVME_DEBUG("FAILED: vaddr(%p) nbytes(%zu) not aligned to 4096", vaddr, nbytes);
		return -EINVAL;
	}

	err = _dmabuf_dmem_init();
	if (err) {
		return err;
	}

	err = xnvme_be_upcie_dmabuf_check_base(&g_dmabuf_dmem, vaddr, nbytes);
	err = err ? err : xnvme_be_upcie_buf_reg_ref(&g_dmabuf_dmem, 0, 0, vaddr, nbytes);
	if (err != -ENOENT) {
		if (err || _dmabuf_reaches(dev, vaddr)) {
			return err;
		}
		err = _dmabuf_map(dev, fd, vaddr, nbytes);
		if (err && !xnvme_buf_reg_find(&g_dmabuf_dmem, vaddr, nbytes, &reg)) {
			reg->refs -= 1;
		}
		return err;
	}

	err = _dmabuf_map(dev, fd, vaddr, nbytes);
	if (err) {
		return err;
	}
	err = xnvme_buf_reg_add(&g_dmabuf_dmem, vaddr, nbytes, 0);
	if (err) {
		_dmabuf_unmap(vaddr);
	}

	return err;
}

static int
_dmabuf_unregister(const struct xnvme_dev *XNVME_UNUSED(dev), void *vaddr)
{
	struct xnvme_buf_reg *last;
	int err;

	err = xnvme_be_upcie_buf_reg_unref(&g_dmabuf_dmem, 0, 0, vaddr, &last);
	if (err || !last) {
		return err;
	}

	err = _dmabuf_unmap(last->vaddr);
	/* Served, the range is forgotten even when the server keeps it */
	if (err && !g_upcie_rte.connection.alive) {
		return err;
	}
	xnvme_buf_reg_remove(last);

	return err;
}

static int
xnvme_be_upcie_buf_unregister(const struct xnvme_dev *dev, void *vaddr)
{
	struct xnvme_buf_reg *reg;

	if (g_dmabuf_dmem_alive && !xnvme_buf_reg_find(&g_dmabuf_dmem, vaddr, 1, &reg)) {
		return _dmabuf_unregister(dev, vaddr);
	}

	return _host_unregister(dev, vaddr);
}

#endif

struct xnvme_be_mem g_xnvme_be_upcie_mem = {
	.id = "upcie",
#ifdef XNVME_BE_UPCIE_ENABLED
	.buf_alloc = xnvme_be_upcie_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_upcie_buf_free,
	.buf_vtophys = xnvme_be_upcie_buf_vtophys,
	.buf_register = xnvme_be_upcie_buf_register,
	.buf_register_dmabuf = xnvme_be_upcie_buf_register_dmabuf,
	.buf_unregister = xnvme_be_upcie_buf_unregister,
	.buf_get_alignment = xnvme_be_upcie_buf_get_alignment,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.buf_register = xnvme_be_nosys_buf_register,
	.buf_unregister = xnvme_be_nosys_buf_unregister,
#endif
};
