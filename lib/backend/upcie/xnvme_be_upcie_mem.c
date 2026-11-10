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

/**
 * The dma-bufs registered with xnvme_buf_register_dmabuf() on host memory
 *
 * Each has a dmamem of its own, as the heap's translates the heap alone, and a command finds it
 * by its payload's address (xnvme_be_upcie_dmem_of()); the count makes that lookup free in a
 * process that registers none. The library's registrations (xnvme_buf_reg) count references, and
 * each one's priv is its index here.
 */
#define DMABUF_REGS_MAX 64

static struct {
	struct dmamem dmem;
	struct xnvme_be_upcie_ctrlr *ctrlr; ///< Registered through this client's server, or NULL
	uint64_t offset;                    ///< Of the registration with the server
	int alive;
} g_dmabuf_regs[DMABUF_REGS_MAX];
static int g_dmabuf_regs_count;

struct dmamem *
xnvme_be_upcie_dmem_of(struct dmamem *dmem, void *buf)
{
	const char *at = buf;

	for (int i = 0; g_dmabuf_regs_count && (i < DMABUF_REGS_MAX); ++i) {
		const char *base = g_dmabuf_regs[i].dmem.base_va;

		if (g_dmabuf_regs[i].alive && (at >= base) &&
		    (at < base + g_dmabuf_regs[i].dmem.size)) {
			return &g_dmabuf_regs[i].dmem;
		}
	}

	return dmem;
}

/**
 * Map `fd` for the controllers: through the server of a client, else, with the controllers on
 * vfio-cdev, into their IOAS with IOMMU_IOAS_MAP_FILE, which upstream kernels allow for the
 * dma-bufs vfio-pci exports
 */
static int
_dmabuf_map(const struct xnvme_dev *dev, int fd, void *vaddr, size_t nbytes, int *at)
{
	const struct xnvme_be_upcie_state *state = &xnvme_be_state(dev)->upcie_state;
	struct dmamem *dmem = NULL;
	uint64_t iova = 0;
	int err;

	for (int i = 0; i < DMABUF_REGS_MAX; ++i) {
		if (!g_dmabuf_regs[i].alive) {
			dmem = &g_dmabuf_regs[i].dmem;
			*at = i;
			break;
		}
	}
	if (!dmem) {
		XNVME_DEBUG("FAILED: %d dma-bufs are registered already", DMABUF_REGS_MAX);
		return -ENOMEM;
	}

	if (g_upcie_rte.connection.alive) {
		const struct hostmem_shared_desc *desc = NULL;
		uint64_t offset = 0;

		err = xnvme_be_upcie_cplane_register_client_mem(state->ctrlr, fd, nbytes, 4096,
								&desc, &offset);
		if (err) {
			XNVME_DEBUG("FAILED: registering with the server; err(%d)", err);
			return err;
		}
		err = dmamem_from_shared(dmem, vaddr, desc, xnvme_be_upcie_va_bits(),
					 DMAMEM_BACKING_DMABUF);
		if (err) {
			XNVME_DEBUG("FAILED: dmamem_from_shared(); err(%d)", err);
			xnvme_be_upcie_cplane_unregister_client_mem(state->ctrlr, offset);
			return err;
		}
		g_dmabuf_regs[*at].ctrlr = state->ctrlr;
		g_dmabuf_regs[*at].offset = offset;
	} else {
		if ((XNVME_BE_UPCIE_MODE_VFIO_CDEV != g_upcie_rte.mode) ||
		    !g_upcie_rte.cdev.ioas_alive) {
			XNVME_DEBUG("FAILED: without a server, a dma-buf takes vfio-cdev");
			return -ENOSYS;
		}
		err = iommufd_ioas_map_file(&g_upcie_rte.cdev.iommufd, fd, 0, nbytes,
					    IOMMU_IOAS_MAP_READABLE | IOMMU_IOAS_MAP_WRITEABLE,
					    &iova);
		if (err) {
			XNVME_DEBUG("FAILED: iommufd_ioas_map_file(); err(%d)", err);
			return err;
		}
		memset(dmem, 0, sizeof(*dmem));
		dmem->fd = -1;
		dmem->base_va = vaddr;
		dmem->base_iova = iova;
		dmem->size = nbytes;
		dmem->backing = DMAMEM_BACKING_DMABUF;
		dmem->translator = DMAMEM_XLATE_ARITHMETIC;
		g_dmabuf_regs[*at].ctrlr = NULL;
	}

	g_dmabuf_regs[*at].alive = 1;
	g_dmabuf_regs_count++;

	return 0;
}

static int
_dmabuf_unmap(int at)
{
	struct dmamem *dmem = &g_dmabuf_regs[at].dmem;
	int err;

	if (g_dmabuf_regs[at].ctrlr) {
		err = xnvme_be_upcie_cplane_unregister_client_mem(g_dmabuf_regs[at].ctrlr,
								  g_dmabuf_regs[at].offset);
	} else {
		err = iommufd_ioas_unmap(&g_upcie_rte.cdev.iommufd, dmem->base_iova, dmem->size);
	}
	if (err) {
		XNVME_DEBUG("FAILED: unmapping the dma-buf; err(%d)", err);
		return err;
	}
	if (DMAMEM_XLATE_LUT == dmem->translator) {
		dmamem_registry_term(&dmem->registry);
	}

	memset(&g_dmabuf_regs[at], 0, sizeof(g_dmabuf_regs[at]));
	g_dmabuf_regs_count--;

	return 0;
}

static int
xnvme_be_upcie_buf_register_dmabuf(const struct xnvme_dev *dev, int fd, void *vaddr)
{
	struct xnvme_buf_reg *reg;
	size_t nbytes;
	off_t size;
	int at = -1, err;

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

	err = xnvme_buf_reg_find(&g_upcie_rte.mem.dmem, vaddr, nbytes, &reg);
	if (!err) {
		reg->refs += 1;
		return 0;
	}
	if (err != -ENOENT) {
		return err;
	}

	err = _dmabuf_map(dev, fd, vaddr, nbytes, &at);
	if (err) {
		return err;
	}
	err = xnvme_buf_reg_add(&g_upcie_rte.mem.dmem, vaddr, nbytes, (uint64_t)at);
	if (err) {
		_dmabuf_unmap(at);
	}

	return err;
}

static int
xnvme_be_upcie_buf_unregister(const struct xnvme_dev *XNVME_UNUSED(dev), void *vaddr)
{
	struct xnvme_buf_reg *reg;
	int err;

	err = xnvme_buf_reg_find(&g_upcie_rte.mem.dmem, vaddr, 1, &reg);
	if (err || (reg->vaddr != vaddr)) {
		XNVME_DEBUG("FAILED: vaddr(%p) is not a registered dma-buf", vaddr);
		return -EINVAL;
	}
	if (reg->refs > 1) {
		reg->refs -= 1;
		return 0;
	}

	err = _dmabuf_unmap((int)reg->priv);
	if (err) {
		return err;
	}
	xnvme_buf_reg_remove(reg);

	return 0;
}

static int
xnvme_be_upcie_buf_get_alignment(const struct xnvme_dev *XNVME_UNUSED(dev), size_t *align)
{
	*align = 4096;

	return 0;
}

#endif

struct xnvme_be_mem g_xnvme_be_upcie_mem = {
	.id = "upcie",
#ifdef XNVME_BE_UPCIE_ENABLED
	.buf_alloc = xnvme_be_upcie_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_upcie_buf_free,
	.buf_vtophys = xnvme_be_upcie_buf_vtophys,
	.buf_register = xnvme_be_nosys_buf_register,
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
