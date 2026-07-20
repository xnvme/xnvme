// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * NVMe Controller Extension: dmamem over vfio-pci + type1 container
 * =================================================================
 *
 * Sibling of nvme_controller_dmamem_vfio for the legacy vfio type1
 * container world. The caller owns the container, the group (already
 * viable and set_container'd), and the dmamem_heap (backed by a
 * dmamem_from_hostmem_type1 wrap so the container's TYPE1 IOMMU has
 * the mapping in place); this module owns the device fd, the mmap'd
 * BAR0, and the admin queue buffers sub-allocated from the heap.
 *
 * The submit/reap path is the existing nvme_qpair primitives; only
 * the SQ/CQ backing (and the DMA-address arithmetic) is different
 * from the base upcie hostmem_heap path, so the dmamem-backed admin
 * queue reuses nvme_qpair_enqueue, nvme_qpair_sqdb_update, and
 * nvme_qpair_reap_cpl unchanged.
 *
 * @file nvme_controller_dmamem_type1.h
 * @version 0.5.2
 */

/**
 * Context for a single NVMe controller reached via vfio type1 + a
 * group cdev.
 *
 * The container and group are caller-owned; container may be shared
 * across multiple controllers (multiple groups attached to it), while
 * each controller owns its group binding to its device fd.
 */
struct nvme_dmamem_type1_ctx {
	struct vfio_container *container; ///< Not owned; caller lifetime
	struct vfio_group *group;         ///< Not owned; caller lifetime
	int device_fd;                    ///< Owned; obtained from group + BDF
	void *bar0;                       ///< Owned; mmap'd BAR0
	size_t bar0_size;                 ///< BAR0 mapping length
	size_t aq_sq_offset;              ///< Admin SQ offset in the caller-provided heap
	size_t aq_cq_offset;              ///< Admin CQ offset in the caller-provided heap
	size_t aq_prp_offset;             ///< Admin PRP-scratch offset in the caller-provided heap
	int aq_alive;                     ///< Whether aq_{sq,cq,prp}_offset hold live allocations
};

static inline void
nvme_dmamem_type1_ctx_init(struct nvme_dmamem_type1_ctx *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->device_fd = -1;
}

/**
 * Release the type1 device fd + BAR0 mapping.
 *
 * The caller-owned container and group stay alive.
 */
static inline int
nvme_dmamem_type1_ctx_close(struct nvme_dmamem_type1_ctx *ctx)
{
	if (ctx->bar0 && ctx->bar0_size) {
		munmap(ctx->bar0, ctx->bar0_size);
		ctx->bar0 = NULL;
		ctx->bar0_size = 0;
	}

	if (ctx->device_fd >= 0) {
		close(ctx->device_fd);
	}

	nvme_dmamem_type1_ctx_init(ctx);

	return 0;
}

/**
 * Open an NVMe controller through a legacy vfio type1 container.
 *
 * Preconditions: container is open, group is open + viable +
 * set_container'd, container has VFIO_SET_IOMMU(TYPE1) applied, and
 * heap sits on a dmamem_from_hostmem_type1 wrap that already
 * VFIO_IOMMU_MAP_DMA'd the underlying hugepage into the container's
 * IOMMU. Nothing in this function installs a DMA mapping; it walks
 * the device open + admin queue setup end.
 *
 * @param ctrlr     Controller descriptor to populate.
 * @param ctx       Controller context (owned by caller until close).
 * @param container The type1 container the group is attached to.
 * @param group     Viable group already attached to container.
 * @param heap      dmamem_heap holding admin queue backing storage.
 *                  The heap must outlive the controller; its SQ/CQ
 *                  offsets are stored in ctx and freed by
 *                  nvme_controller_close_dmamem_type1.
 * @param bdf       PCIe address in "0000:bb:dd.f" form.
 *
 * @return 0 on success, negative errno on error.
 */
static inline int
nvme_controller_open_dmamem_type1(struct nvme_controller *ctrlr, struct nvme_dmamem_type1_ctx *ctx,
				  struct vfio_container *container, struct vfio_group *group,
				  struct dmamem_heap *heap, const char *bdf)
{
	uint64_t sq_iova = 0, cq_iova = 0;
	uint64_t cap;
	uint8_t *bar0;
	int err;

	if (!ctrlr || !ctx || !container || !group || !heap || !bdf) {
		return -EINVAL;
	}

	memset(ctrlr, 0, sizeof(*ctrlr));
	nvme_dmamem_type1_ctx_init(ctx);
	ctx->container = container;
	ctx->group = group;

	nvme_qid_bitmap_init(ctrlr->qids);

	ctx->device_fd = vfio_group_get_device_fd(group, bdf);
	if (ctx->device_fd < 0) {
		err = -errno;
		UPCIE_DEBUG("FAILED: vfio_group_get_device_fd('%s'); errno(%d)", bdf, errno);
		goto fail;
	}

	err = nvme_vfio_pci_acquire_bar0(ctx->device_fd, ctrlr, &ctx->bar0, &ctx->bar0_size);
	if (err) {
		goto fail;
	}
	bar0 = ctx->bar0;

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
	nvme_dmamem_type1_ctx_close(ctx);
	memset(ctrlr, 0, sizeof(*ctrlr));
	return err;
}

/**
 * Close a controller opened with nvme_controller_open_dmamem_type1.
 *
 * Disables the controller, releases the admin queue's SQ/CQ back to
 * heap, closes the device fd, and munmaps BAR0. The caller-owned
 * container, group, and dmamem_heap stay alive.
 *
 * @param ctrlr  Controller populated by nvme_controller_open_dmamem_type1;
 *               memset to zero on return.
 * @param ctx    Context populated by nvme_controller_open_dmamem_type1;
 *               reset to a fresh state on return.
 * @param heap   The same heap that was passed to open.
 *
 * @return 0 on success; first negative errno encountered during
 * teardown otherwise.
 */
static inline int
nvme_controller_close_dmamem_type1(struct nvme_controller *ctrlr, struct nvme_dmamem_type1_ctx *ctx,
				   struct dmamem_heap *heap)
{
	int first_err = 0;
	int err;

	if (ctx->bar0) {
		nvme_mmio_cc_disable(ctx->bar0);
		if (ctrlr->timeout_ms) {
			err = nvme_mmio_csts_wait_until_not_ready(ctx->bar0, ctrlr->timeout_ms);
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

	err = nvme_dmamem_type1_ctx_close(ctx);
	if (err && !first_err) {
		first_err = err;
	}

	memset(ctrlr, 0, sizeof(*ctrlr));

	return first_err;
}

/*
 * IO qpair create/delete and the admin-sync helper are the vfio-cdev
 * variants from nvme_controller_dmamem_vfio.h; they touch only
 * nvme_qpair primitives and the caller-provided dmamem_heap, so the
 * type1 path reuses them unchanged. Once open, the mode is invisible
 * from here on.
 */
