// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <errno.h>
#include <xnvme_be_upcie.h>
#include <xnvme_dev.h>

/**
 * Allocate a buffer from the RTE's dmamem_heap.
 *
 * Offsets into the heap resolve to device addresses through the dmamem
 * translator, so the same allocation works whichever way the target is
 * attached; the caller hands the returned VA to the device as a PRP later.
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

	return xnvme_be_upcie_buf_alloc_on(state ? state->ctrlr : NULL, nbytes, phys);
}

void
xnvme_be_upcie_buf_free(const struct xnvme_dev *dev, void *buf)
{
	struct xnvme_be_upcie_state *state = dev ? (void *)dev->be.state : NULL;

	xnvme_be_upcie_buf_free_on(state ? state->ctrlr : NULL, buf);
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
