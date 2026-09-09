// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <xnvme_be_upcie.h>
#include <xnvme_dev.h>

/**
 * Allocate a buffer from the RTE's dmamem_heap.
 *
 * Offsets into the heap resolve to device addresses through the dmamem
 * translator, so the same allocation works whichever way the target is
 * attached; the caller hands the returned VA to the device as a PRP later.
 */
/*
 * Takes no lock: the served branch hands the allocation to the server, and
 * the local branch is reached from xnvme_be_upcie_buf_alloc(), which holds
 * the heap lock, or from a controller's open and close, which hold it too.
 */
void *
xnvme_be_upcie_buf_alloc_on(struct xnvme_be_upcie_ctrlr *ctrlr, size_t nbytes, uint64_t *phys)
{
	size_t offset = 0;
	void *buf;
	int err;

	if (g_upcie_rte.connection.alive) {
		/* The allocator belongs to whoever owns the heap, so this asks
		 * for an offset rather than taking one. */
		struct nvme_cplane_msg msg = {0};

		msg.op = NVME_CPLANE_OP_ALLOC_BUF;
		msg.u.mem.nbytes = nbytes;

		if (!ctrlr) {
			errno = EINVAL;
			return NULL;
		}

		err = xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL);
		if (err) {
			errno = -err;
			return NULL;
		}

		buf = (char *)g_upcie_rte.connection.heap_base + msg.u.mem.offset;
		if (phys) {
			*phys = dmamem_va_to_iova(&g_upcie_rte.mem.dmem, buf);
		}

		return buf;
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
xnvme_be_upcie_buf_free_on(struct xnvme_be_upcie_ctrlr *ctrlr, void *buf)
{
	size_t offset;

	if (!buf) {
		return;
	}
	offset = (size_t)((char *)buf - (char *)g_upcie_rte.mem.dmem.cpu_va);

	if (g_upcie_rte.connection.alive) {
		struct nvme_cplane_msg msg = {0};

		msg.op = NVME_CPLANE_OP_FREE_BUF;
		msg.u.mem.offset = offset;

		if (!ctrlr || xnvme_be_upcie_cplane_ask_ctrlr(ctrlr, &msg, NULL, NULL)) {
			XNVME_DEBUG("FAILED: giving back offset(0x%zx)", offset);
		}

		return;
	}

	dmamem_heap_free(&g_upcie_rte.mem.heap, offset);
}

/**
 * Allocate from the heap, on behalf of the device's controller
 *
 * Attached, the allocator belongs to the server this controller is served by,
 * so which controller it is decides which socket to ask on.
 */
void *
xnvme_be_upcie_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys)
{
	struct xnvme_be_upcie_state *state = dev ? (void *)dev->be.state : NULL;
	void *buf;

	xnvme_be_upcie_heap_lock();
	buf = xnvme_be_upcie_buf_alloc_on(state ? state->ctrlr : NULL, nbytes, phys);
	xnvme_be_upcie_heap_unlock();
	return buf;
}

void
xnvme_be_upcie_buf_free(const struct xnvme_dev *dev, void *buf)
{
	struct xnvme_be_upcie_state *state = dev ? (void *)dev->be.state : NULL;

	xnvme_be_upcie_heap_lock();
	xnvme_be_upcie_buf_free_on(state ? state->ctrlr : NULL, buf);
	xnvme_be_upcie_heap_unlock();
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

/**
 * Adopt a described region into a table-translating dmamem
 *
 * A LUT description is adopted as it is. An arithmetic one names a single
 * base, so it is unrolled into one entry per granule first.
 */
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

	if (!dmem || !base || !desc || !page_size || (page_size & (page_size - 1))) {
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

/**
 * What a served client has registered, so an unmap can name it to the server
 *
 * Keyed by region and controller, since the server keeps one registration per
 * controller and answers each with an offset of its own. Cold, and small: a
 * process registers its buffers once, not its commands.
 */
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

	xnvme_be_upcie_heap_lock();
	err = _adopt_desc(dmem, vaddr, desc, page_size);
	if (!err) {
		err = _served_regs_add(vaddr, ctrlr, offset);
		if (err) {
			dmamem_registry_remove(&dmem->registry, vaddr);
		}
	}
	xnvme_be_upcie_heap_unlock();

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

	xnvme_be_upcie_heap_lock();
	at = _served_regs_find(vaddr, ctrlr);
	if (at < 0) {
		xnvme_be_upcie_heap_unlock();
		return -EINVAL;
	}
	offset = g_served_regs.entries[at].offset;
	g_served_regs.entries[at] = g_served_regs.entries[--g_served_regs.count];
	dmamem_registry_remove(&dmem->registry, vaddr);
	xnvme_be_upcie_heap_unlock();

	err = xnvme_be_upcie_cplane_unregister_client_mem(ctrlr, offset);
	if (err) {
		XNVME_DEBUG("FAILED: the server kept the registration; err(%d)", err);
	}

	return err;
}

#endif

struct xnvme_be_mem g_xnvme_be_upcie_mem = {
	.id = "upcie",
#ifdef XNVME_BE_UPCIE_ENABLED
	.buf_alloc = xnvme_be_upcie_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_upcie_buf_free,
	.buf_vtophys = xnvme_be_upcie_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#endif
};
