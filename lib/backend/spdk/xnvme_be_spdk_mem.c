// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_SPDK_ENABLED
#include <errno.h>
#include <stdbool.h>
#include <rte_eal.h>
#include <spdk/env.h>
#include <spdk/memory.h>
#include <spdk/nvme.h>
#include <xnvme_dev.h>
#include <xnvme_be_state.h>
#include <xnvme_buf.h>
#include <xnvme_host_buf.h>

static void *
xnvme_be_spdk_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys)
{
	const size_t alignment = dev->geo.nbytes ? dev->geo.nbytes : 4096;
	void *buf;

	buf = spdk_dma_malloc(nbytes, alignment, phys);
	if (!buf) {
		errno = ENOMEM;
		return NULL;
	}

	return buf;
}

static void *
xnvme_be_spdk_buf_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes, uint64_t *phys)
{
	const size_t alignment = dev->geo.nbytes ? dev->geo.nbytes : 4096;
	void *rebuf;

	rebuf = spdk_dma_realloc(buf, nbytes, alignment, phys);
	if (!rebuf) {
		errno = ENOMEM;
		return NULL;
	}

	return rebuf;
}

static void
xnvme_be_spdk_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	spdk_dma_free(buf);
}

static int
xnvme_be_spdk_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf, uint64_t *phys)
{
	*phys = spdk_vtophys(buf, NULL);
	if (SPDK_VTOPHYS_ERROR == *phys) {
		return -EIO;
	}
	return 0;
}

// TCP copies the payload of each command through a socket, so it needs nothing registered
static bool
_needs_registering(const struct xnvme_dev *dev)
{
	const struct xnvme_be_spdk_state *state = &xnvme_be_state(dev)->spdk_state;

	return spdk_nvme_ctrlr_get_transport_id(state->ctrlr)->trtype != SPDK_NVME_TRANSPORT_TCP;
}

// Only an IOMMU addressed by virtual address takes memory that is not physically contiguous
static bool
_needs_hugetlb(const struct xnvme_dev *dev)
{
	const struct xnvme_be_spdk_state *state = &xnvme_be_state(dev)->spdk_state;

	return (spdk_nvme_ctrlr_get_transport_id(state->ctrlr)->trtype ==
		SPDK_NVME_TRANSPORT_PCIE) &&
	       !(spdk_iommu_is_enabled() && (rte_eal_iova_mode() == RTE_IOVA_VA));
}

// Whether SPDK translates the range already, as it does memory from xnvme_buf_alloc()
static bool
_is_translated(void *vaddr, size_t nbytes)
{
	return (spdk_vtophys(vaddr, NULL) != SPDK_VTOPHYS_ERROR) &&
	       (spdk_vtophys((uint8_t *)vaddr + nbytes - 1, NULL) != SPDK_VTOPHYS_ERROR);
}

/*
 * SPDK registers memory for the process rather than for a device, so the registrations are the
 * process's, whichever device they come from
 */
static int
xnvme_be_spdk_buf_register(const struct xnvme_dev *dev, void *vaddr, size_t nbytes)
{
	struct xnvme_buf_reg *reg;
	int err;

	if (!_needs_registering(dev)) {
		return xnvme_buf_noreg_register(dev, vaddr, nbytes);
	}

	if (!vaddr || !nbytes || (((uintptr_t)vaddr | nbytes) & MASK_2MB)) {
		XNVME_DEBUG(
			"FAILED: vaddr(%p) and nbytes(%zu) must be non-zero multiples of 2 MiB",
			vaddr, nbytes);
		return -EINVAL;
	}

	err = xnvme_buf_reg_find(NULL, vaddr, nbytes, &reg);
	if (!err) {
		reg->refs += 1;
		return 0;
	}
	if (err != -ENOENT) {
		return err;
	}

	if (_is_translated(vaddr, nbytes)) {
		XNVME_DEBUG("INFO: vaddr(%p) is translated already, as from xnvme_buf_alloc()",
			    vaddr);
		return 0;
	}

	if (_needs_hugetlb(dev) && !xnvme_buf_is_hugetlb(vaddr, nbytes)) {
		XNVME_DEBUG("FAILED: no IOMMU with iova_mode=va and vaddr(%p) is not hugetlb",
			    vaddr);
		return -ENOTSUP;
	}

	err = spdk_mem_register(vaddr, nbytes);
	if (err) {
		XNVME_DEBUG("FAILED: spdk_mem_register(%p, %zu), err: %d", vaddr, nbytes, err);
		return err;
	}

	err = xnvme_buf_reg_add(NULL, vaddr, nbytes, 0);
	if (err) {
		spdk_mem_unregister(vaddr, nbytes);
	}

	return err;
}

static int
xnvme_be_spdk_buf_unregister(const struct xnvme_dev *dev, void *vaddr)
{
	struct xnvme_buf_reg *reg;
	int err;

	if (!_needs_registering(dev)) {
		return xnvme_buf_noreg_unregister(dev, vaddr);
	}

	err = xnvme_buf_reg_find(NULL, vaddr, 1, &reg);
	if (err == -ENOENT && vaddr && _is_translated(vaddr, 1)) {
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

	err = spdk_mem_unregister(reg->vaddr, reg->nbytes);
	if (err) {
		XNVME_DEBUG("FAILED: spdk_mem_unregister(%p, %zu), err: %d", reg->vaddr,
			    reg->nbytes, err);
		return err;
	}

	xnvme_buf_reg_remove(reg);

	return 0;
}

static int
xnvme_be_spdk_buf_get_alignment(const struct xnvme_dev *dev, size_t *align)
{
	if (!_needs_registering(dev)) {
		return xnvme_buf_noreg_get_alignment(dev, align);
	}

	*align = VALUE_2MB;

	return 0;
}
#endif

struct xnvme_be_mem g_xnvme_be_spdk_mem = {
	.id = "spdk",
#ifdef XNVME_BE_SPDK_ENABLED
	.buf_alloc = xnvme_be_spdk_buf_alloc,
	.buf_realloc = xnvme_be_spdk_buf_realloc,
	.buf_free = xnvme_be_spdk_buf_free,
	.buf_vtophys = xnvme_be_spdk_buf_vtophys,
	.buf_register = xnvme_be_spdk_buf_register,
	.buf_unregister = xnvme_be_spdk_buf_unregister,
	.buf_get_alignment = xnvme_be_spdk_buf_get_alignment,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.buf_register = xnvme_be_nosys_buf_register,
	.buf_unregister = xnvme_be_nosys_buf_unregister,
#endif
};
