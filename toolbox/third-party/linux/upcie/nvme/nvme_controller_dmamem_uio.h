// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * NVMe Controller Extension: dmamem over uio_pci_generic
 * ======================================================
 *
 * Sibling of nvme_controller_dmamem_vfio for the uio_pci_generic + LUT
 * world. Same submit/reap primitives, same dmamem_heap-backed admin
 * queue, same IO qpair helpers; only the way BAR0 is acquired differs.
 * uio_pci_generic exposes BAR0 through /sys/bus/pci/devices/<bdf>/resource0
 * rather than through a vfio device fd, and DMA addresses on the wire
 * are physical (iommu=pt or iommu=off) so the caller-provided
 * dmamem_heap must sit on a LUT-translator dmamem.
 *
 * The controller enable dance (CC.EN=0/1, CSTS.RDY, AQA/ASQ/ACQ) is shared
 * with the vfio-pci path via nvme_controller_reset_via_bar0 and
 * nvme_controller_enable_via_bar0.
 *
 * @file nvme_controller_dmamem_uio.h
 * @version 0.5.2
 */

/**
 * Context for a single NVMe controller reached via uio_pci_generic.
 *
 * BAR0 lives inside ctrlr->func.bars[0] once pci_bar_map has run. This ctx
 * only tracks the admin qpair's heap offsets so close can release them.
 */
struct nvme_dmamem_uio_ctx {
	size_t aq_sq_offset;  ///< Admin SQ offset in the caller-provided heap
	size_t aq_cq_offset;  ///< Admin CQ offset in the caller-provided heap
	size_t aq_prp_offset; ///< Admin PRP-scratch offset in the caller-provided heap
	int bar0_mapped;      ///< Whether ctrlr->func.bars[0] holds a live mmap
	int aq_alive;         ///< Whether aq_{sq,cq,prp}_offset hold live allocations
};

static inline void
nvme_dmamem_uio_ctx_init(struct nvme_dmamem_uio_ctx *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
}

/**
 * Open an NVMe controller through uio_pci_generic.
 *
 * The heap must sit on a LUT-translator dmamem (typically a hostmem
 * hugepage wrapped via dmamem_from_hostmem_lut) so dmamem_heap_at_iova
 * yields physical addresses that the device can DMA against with
 * iommu=pt/off.
 *
 * @param ctrlr Controller descriptor to populate.
 * @param ctx   Controller context (owned by caller until close).
 * @param heap  dmamem_heap holding admin queue backing storage. The
 *              heap must outlive the controller; its SQ/CQ offsets are
 *              stored in ctx and freed by
 *              nvme_controller_close_dmamem_uio.
 * @param bdf   PCIe address in "0000:bb:dd.f" form.
 *
 * @return 0 on success, negative errno on error.
 */
static inline int
nvme_controller_open_dmamem_uio(struct nvme_controller *ctrlr, struct nvme_dmamem_uio_ctx *ctx,
				struct dmamem_heap *heap, const char *bdf)
{
	uint64_t sq_iova = 0, cq_iova = 0;
	uint64_t cap;
	uint8_t *bar0;
	int err;

	if (!ctrlr || !ctx || !heap || !bdf) {
		return -EINVAL;
	}

	memset(ctrlr, 0, sizeof(*ctrlr));
	nvme_dmamem_uio_ctx_init(ctx);

	nvme_qid_bitmap_init(ctrlr->qids);

	err = pci_func_open(bdf, &ctrlr->func);
	if (err) {
		UPCIE_DEBUG("FAILED: pci_func_open('%s'); err(%d)", bdf, err);
		goto fail;
	}

	err = pci_bar_map(ctrlr->func.bdf, 0, &ctrlr->func.bars[0]);
	if (err) {
		UPCIE_DEBUG("FAILED: pci_bar_map(BAR0); err(%d)", err);
		goto fail;
	}
	ctx->bar0_mapped = 1;
	bar0 = ctrlr->func.bars[0].region;

	err = nvme_controller_reset_via_bar0(ctrlr, bar0, &cap);
	if (err) {
		goto fail;
	}

	err = nvme_qpair_dmamem_init(&ctrlr->aq, 0, 256, bar0, heap, &ctx->aq_sq_offset,
				     &ctx->aq_cq_offset, &ctx->aq_prp_offset, &sq_iova, &cq_iova);
	if (err) {
		UPCIE_DEBUG("FAILED: nvme_qpair_dmamem_init(aq); err(%d)", err);
		goto fail;
	}
	ctx->aq_alive = 1;

	nvme_mmio_aq_setup(bar0, sq_iova, cq_iova, ctrlr->aq.depth);

	err = nvme_controller_enable_via_bar0(ctrlr, bar0, cap);
	if (err) {
		goto fail;
	}

	return 0;

fail:
	if (ctx->aq_alive) {
		nvme_qpair_dmamem_term(&ctrlr->aq, heap, ctx->aq_sq_offset, ctx->aq_cq_offset,
				       ctx->aq_prp_offset);
		ctx->aq_alive = 0;
	}
	if (ctx->bar0_mapped) {
		pci_func_close(&ctrlr->func);
		ctx->bar0_mapped = 0;
	}
	memset(ctrlr, 0, sizeof(*ctrlr));
	return err;
}

/**
 * Close a controller opened with nvme_controller_open_dmamem_uio.
 *
 * Disables the controller, releases the admin queue's SQ/CQ back to
 * heap (using the offsets ctx recorded at open), and unmaps BAR0. The
 * dmamem_heap itself stays with the caller.
 *
 * @param ctrlr  Controller populated by nvme_controller_open_dmamem_uio;
 *               memset to zero on return.
 * @param ctx    Context populated by nvme_controller_open_dmamem_uio;
 *               reset to a fresh state on return.
 * @param heap   The same heap that was passed to open; must still be
 *               valid.
 *
 * @return 0 on success; first negative errno encountered during
 * teardown otherwise.
 */
static inline int
nvme_controller_close_dmamem_uio(struct nvme_controller *ctrlr, struct nvme_dmamem_uio_ctx *ctx,
				 struct dmamem_heap *heap)
{
	int first_err = 0;
	int err;

	if (ctx->bar0_mapped && ctrlr->func.bars[0].region) {
		nvme_mmio_cc_disable(ctrlr->func.bars[0].region);
		if (ctrlr->timeout_ms) {
			err = nvme_mmio_csts_wait_until_not_ready(ctrlr->func.bars[0].region,
								  ctrlr->timeout_ms);
			if (err) {
				first_err = err;
			}
		}
	}

	if (ctx->aq_alive) {
		nvme_qpair_dmamem_term(&ctrlr->aq, heap, ctx->aq_sq_offset, ctx->aq_cq_offset,
				       ctx->aq_prp_offset);
		ctx->aq_alive = 0;
	}

	if (ctx->bar0_mapped) {
		pci_func_close(&ctrlr->func);
		ctx->bar0_mapped = 0;
	}

	nvme_dmamem_uio_ctx_init(ctx);
	memset(ctrlr, 0, sizeof(*ctrlr));

	return first_err;
}

/*
 * IO qpair create/delete and the admin-sync helper are the vfio-cdev
 * variants from nvme_controller_dmamem.h; they touch only nvme_qpair
 * primitives and the caller-provided dmamem_heap, so the uio path
 * reuses them unchanged. Once open, the mode is invisible from here on.
 */
