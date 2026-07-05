// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * NVMe Controller Extension: dmamem over vfio-cdev + iommufd
 * ==========================================================
 *
 * Sibling of nvme_controller_vfio for the vfio-cdev + iommufd + dmamem
 * world. The caller owns the iommufd handle and the target IOAS; this
 * module owns the vfio device attached to that IOAS, the mmap'd BAR0,
 * and the admin queue buffers sub-allocated from a caller-provided
 * dmamem_heap.
 *
 * The submit/reap path is the existing nvme_qpair primitives; only the
 * SQ/CQ backing (and the DMA-address arithmetic) is different, so the
 * dmamem-backed admin queue reuses nvme_qpair_enqueue,
 * nvme_qpair_sqdb_update, and nvme_qpair_reap_cpl unchanged.
 *
 * @file nvme_controller_dmamem_vfio.h
 * @version 0.5.2
 */

/**
 * Context for a single NVMe controller reached via vfio-cdev + iommufd.
 *
 * The iommufd handle and IOAS are caller-owned. The vfio_device_path
 * (e.g. /dev/vfio/devices/vfio3) identifies the cdev; the caller
 * resolves it from BDF via sysfs, then hands it in.
 */
struct nvme_dmamem_vfio_ctx {
	struct iommufd *iommufd;   ///< Not owned; caller lifetime
	uint32_t ioas_id;          ///< Not owned; caller lifetime
	struct iommufd_device dev; ///< Owned; vfio cdev bound + attached
	void *bar0;                ///< mmap'd BAR0
	size_t bar0_size;          ///< BAR0 mapping length
	size_t aq_sq_offset;       ///< Admin SQ offset in the caller-provided heap
	size_t aq_cq_offset;       ///< Admin CQ offset in the caller-provided heap
	size_t aq_prp_offset;      ///< Admin PRP-scratch offset in the caller-provided heap
	int attached;              ///< Whether dev is attached to ioas_id
	int aq_alive;              ///< Whether aq_{sq,cq,prp}_offset hold live allocations
};

static inline void
nvme_dmamem_vfio_ctx_init(struct nvme_dmamem_vfio_ctx *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->dev.fd = -1;
}

/**
 * Release the vfio device + BAR0 mapping.
 *
 * The caller-owned iommufd handle and IOAS stay alive.
 */
static inline int
nvme_dmamem_vfio_ctx_close(struct nvme_dmamem_vfio_ctx *ctx)
{
	int first_err = 0;
	int err;

	if (ctx->bar0 && ctx->bar0_size) {
		munmap(ctx->bar0, ctx->bar0_size);
		ctx->bar0 = NULL;
		ctx->bar0_size = 0;
	}

	if (ctx->attached) {
		err = iommufd_device_detach(&ctx->dev);
		if (err && !first_err) {
			first_err = err;
		}
		ctx->attached = 0;
	}

	if (ctx->dev.fd >= 0) {
		err = iommufd_device_close(&ctx->dev);
		if (err && !first_err) {
			first_err = err;
		}
	}

	nvme_dmamem_vfio_ctx_init(ctx);

	return first_err;
}

/**
 * Allocate SQ, CQ, and a request pool with per-request PRP-list scratch
 * for a queue pair from a dmamem_heap, and populate the nvme_qpair
 * fields the submit/reap primitives read.
 *
 * The heap stays with the caller; qp->heap is left NULL to signal that
 * qp is not managed by hostmem_dma_free. Use nvme_qpair_dmamem_term to
 * free, passing back the same offsets returned here.
 *
 * @param qp             Queue pair to populate; fully memset before use.
 * @param qid            NVMe queue identifier (0 for the admin queue).
 * @param depth          Queue depth in entries.
 * @param bar0           mmap'd BAR0 base; used to derive SQ/CQ doorbell
 *                       addresses via CAP.DSTRD.
 * @param heap           dmamem_heap the SQ, CQ, and PRP scratch are
 *                       carved from.
 * @param sq_offset_out  On success, heap offset of the SQ allocation.
 * @param cq_offset_out  On success, heap offset of the CQ allocation.
 * @param prp_offset_out On success, heap offset of the per-request PRP
 *                       scratch region (needed by nvme_qpair_dmamem_term).
 * @param sq_iova_out    On success, IOVA of the SQ (for AQA/ASQ or
 *                       CREATE_IO_SQ).
 * @param cq_iova_out    On success, IOVA of the CQ (for AQA/ACQ or
 *                       CREATE_IO_CQ).
 *
 * @return 0 on success; negative errno on allocation failure.
 */
static inline int
nvme_qpair_dmamem_init(struct nvme_qpair *qp, uint32_t qid, uint16_t depth, uint8_t *bar0,
		       struct dmamem_heap *heap, size_t *sq_offset_out, size_t *cq_offset_out,
		       size_t *prp_offset_out, uint64_t *sq_iova_out, uint64_t *cq_iova_out)
{
	int dstrd = nvme_reg_cap_get_dstrd(nvme_mmio_cap_read(bar0));
	size_t queue_bytes = 1024 * 64;
	size_t sq_offset = 0, cq_offset = 0, prp_offset = 0;
	int err;

	memset(qp, 0, sizeof(*qp));
	qp->qid = qid;
	qp->depth = depth;
	qp->phase = 1;
	qp->tail_last_written = UINT16_MAX;
	qp->sqdb = bar0 + 0x1000 + ((2 * qid) << (2 + dstrd));
	qp->cqdb = bar0 + 0x1000 + ((2 * qid + 1) << (2 + dstrd));

	err = dmamem_heap_alloc_aligned(heap, queue_bytes, 4096, &sq_offset);
	if (err) {
		UPCIE_DEBUG("FAILED: dmamem_heap_alloc_aligned(sq); err(%d)", err);
		return err;
	}

	err = dmamem_heap_alloc_aligned(heap, queue_bytes, 4096, &cq_offset);
	if (err) {
		UPCIE_DEBUG("FAILED: dmamem_heap_alloc_aligned(cq); err(%d)", err);
		dmamem_heap_free(heap, sq_offset);
		return err;
	}

	qp->rpool = calloc(1, sizeof(*qp->rpool));
	if (!qp->rpool) {
		UPCIE_DEBUG("FAILED: calloc(rpool); errno(%d)", errno);
		dmamem_heap_free(heap, cq_offset);
		dmamem_heap_free(heap, sq_offset);
		return -errno;
	}
	nvme_request_pool_init(qp->rpool);

	err = nvme_request_pool_init_prps_dmamem(qp->rpool, heap, &prp_offset);
	if (err) {
		UPCIE_DEBUG("FAILED: nvme_request_pool_init_prps_dmamem(); err(%d)", err);
		free(qp->rpool);
		qp->rpool = NULL;
		dmamem_heap_free(heap, cq_offset);
		dmamem_heap_free(heap, sq_offset);
		return err;
	}

	qp->sq = dmamem_heap_at_va(heap, sq_offset);
	qp->cq = dmamem_heap_at_va(heap, cq_offset);
	memset(qp->sq, 0, queue_bytes);
	memset(qp->cq, 0, queue_bytes);

	*sq_offset_out = sq_offset;
	*cq_offset_out = cq_offset;
	*prp_offset_out = prp_offset;
	*sq_iova_out = dmamem_heap_at_iova(heap, sq_offset);
	*cq_iova_out = dmamem_heap_at_iova(heap, cq_offset);

	return 0;
}

/**
 * Release the SQ, CQ, and request pool of a qp initialised by
 * nvme_qpair_dmamem_init.
 *
 * @param qp          Queue pair to release; memset to zero on return.
 * @param heap        The heap the SQ, CQ, and PRP scratch were carved from.
 * @param sq_offset   SQ offset returned by nvme_qpair_dmamem_init.
 * @param cq_offset   CQ offset returned by nvme_qpair_dmamem_init.
 * @param prp_offset  PRP scratch offset returned by nvme_qpair_dmamem_init.
 */
static inline void
nvme_qpair_dmamem_term(struct nvme_qpair *qp, struct dmamem_heap *heap, size_t sq_offset,
		       size_t cq_offset, size_t prp_offset)
{
	if (qp->rpool) {
		nvme_request_pool_term_prps_dmamem(qp->rpool, heap, prp_offset);
		free(qp->rpool);
		qp->rpool = NULL;
	}
	if (qp->sq) {
		dmamem_heap_free(heap, sq_offset);
	}
	if (qp->cq) {
		dmamem_heap_free(heap, cq_offset);
	}
	memset(qp, 0, sizeof(*qp));
}

/**
 * Open an NVMe controller through vfio-cdev + iommufd.
 *
 * @param ctrlr           Controller descriptor to populate.
 * @param ctx             Controller context (owned by caller until close).
 * @param iommufd         Open iommufd handle (caller-owned).
 * @param ioas_id         Target IOAS the device is attached to.
 * @param heap            dmamem_heap holding admin queue backing storage.
 *                        The heap must outlive the controller; its SQ/CQ
 *                        offsets are stored in ctx and freed by
 *                        nvme_controller_close_dmamem_vfio.
 * @param vfio_device_path Path to the vfio cdev, e.g. /dev/vfio/devices/vfio3.
 *
 * @return 0 on success, negative errno on error.
 */
static inline int
nvme_controller_open_dmamem_vfio(struct nvme_controller *ctrlr, struct nvme_dmamem_vfio_ctx *ctx,
			    struct iommufd *iommufd, uint32_t ioas_id, struct dmamem_heap *heap,
			    const char *vfio_device_path)
{
	uint64_t sq_iova = 0, cq_iova = 0;
	uint64_t cap;
	void *bar0;
	int err;

	if (!ctrlr || !ctx || !iommufd || !heap || !vfio_device_path) {
		return -EINVAL;
	}

	memset(ctrlr, 0, sizeof(*ctrlr));
	nvme_dmamem_vfio_ctx_init(ctx);
	ctx->iommufd = iommufd;
	ctx->ioas_id = ioas_id;

	nvme_qid_bitmap_init(ctrlr->qids);

	err = iommufd_device_open(vfio_device_path, &ctx->dev);
	if (err) {
		UPCIE_DEBUG("FAILED: iommufd_device_open('%s'); err(%d)", vfio_device_path, err);
		goto fail;
	}

	err = iommufd_device_bind(&ctx->dev, iommufd);
	if (err) {
		UPCIE_DEBUG("FAILED: iommufd_device_bind(); err(%d)", err);
		goto fail;
	}

	err = iommufd_device_attach(&ctx->dev, ioas_id);
	if (err) {
		UPCIE_DEBUG("FAILED: iommufd_device_attach(); err(%d)", err);
		goto fail;
	}
	ctx->attached = 1;

	err = nvme_vfio_pci_acquire_bar0(ctx->dev.fd, ctrlr, &ctx->bar0, &ctx->bar0_size);
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
	nvme_dmamem_vfio_ctx_close(ctx);
	memset(ctrlr, 0, sizeof(*ctrlr));
	return err;
}

/**
 * Close a controller opened with nvme_controller_open_dmamem_vfio.
 *
 * Disables the controller, releases the admin queue's SQ/CQ back to
 * heap (using the offsets ctx recorded at open), releases the vfio
 * device attachment, and munmaps BAR0. The dmamem_heap itself stays
 * with the caller.
 *
 * @param ctrlr  Controller populated by nvme_controller_open_dmamem_vfio;
 *               memset to zero on return.
 * @param ctx    Context populated by nvme_controller_open_dmamem_vfio;
 *               reset to a fresh state on return.
 * @param heap   The same heap that was passed to open; must still be
 *               valid.
 *
 * @return 0 on success; first negative errno encountered during
 * teardown otherwise.
 */
static inline int
nvme_controller_close_dmamem_vfio(struct nvme_controller *ctrlr, struct nvme_dmamem_vfio_ctx *ctx,
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

	err = nvme_dmamem_vfio_ctx_close(ctx);
	if (err && !first_err) {
		first_err = err;
	}

	memset(ctrlr, 0, sizeof(*ctrlr));

	return first_err;
}

/**
 * Synchronous admin-command helper for the dmamem admin queue.
 *
 * The dmamem admin queue does not carry a request pool, so submit_sync
 * cannot be used. This helper does a manual enqueue + doorbell + reap
 * with a caller-supplied CID.
 */
static inline int
nvme_admin_sync_dmamem(struct nvme_controller *ctrlr, struct nvme_command *cmd, uint16_t cid,
		       struct nvme_completion *cpl)
{
	int err;

	cmd->cid = cid;

	err = nvme_qpair_enqueue(&ctrlr->aq, cmd);
	if (err) {
		return err;
	}
	nvme_qpair_sqdb_update(&ctrlr->aq);

	err = nvme_qpair_reap_cpl(&ctrlr->aq, ctrlr->timeout_ms, cpl);
	if (err) {
		return err;
	}
	if ((cpl->status >> 1) & 0x7FF) {
		UPCIE_DEBUG("FAILED: admin CQE status=0x%x", cpl->status);
		return -EIO;
	}
	return 0;
}

/**
 * Create an I/O queue pair on the dmamem path.
 *
 * Allocates SQ/CQ from the caller's dmamem_heap, then programs the
 * controller via admin CREATE_IO_CQ + CREATE_IO_SQ so the controller
 * knows about the new qpair. The resulting nvme_qpair is compatible
 * with the heap-agnostic submit/reap primitives (nvme_qpair_enqueue,
 * nvme_qpair_sqdb_update, nvme_qpair_reap_cpl).
 *
 * The qid is allocated from the controller's bitmap; the caller must
 * hold on to the returned sq_offset/cq_offset until
 * nvme_controller_delete_io_qpair_dmamem is called.
 */
static inline int
nvme_controller_create_io_qpair_dmamem(struct nvme_controller *ctrlr, struct nvme_qpair *qp,
				       uint16_t depth, struct dmamem_heap *heap,
				       size_t *sq_offset_out, size_t *cq_offset_out,
				       size_t *prp_offset_out)
{
	struct nvme_command cmd = {0};
	struct nvme_completion cpl = {0};
	uint64_t sq_iova = 0, cq_iova = 0;
	uint16_t qid;
	int err;

	err = nvme_qid_find_free(ctrlr->qids);
	if (err < 1) {
		return -ENOMEM;
	}
	qid = err;

	err = nvme_qid_alloc(ctrlr->qids, qid);
	if (err) {
		UPCIE_DEBUG("FAILED: nvme_qid_alloc; err(%d)", err);
		return err;
	}

	err = nvme_qpair_dmamem_init(qp, qid, depth, ctrlr->func.bars[0].region, heap, sq_offset_out,
				     cq_offset_out, prp_offset_out, &sq_iova, &cq_iova);
	if (err) {
		UPCIE_DEBUG("FAILED: nvme_qpair_dmamem_init(io); err(%d)", err);
		nvme_qid_free(ctrlr->qids, qid);
		return err;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.opc = 0x5; /* Create I/O Completion Queue */
	cmd.prp1 = cq_iova;
	cmd.cdw10 = ((uint32_t)(depth - 1) << 16) | qid;
	cmd.cdw11 = 0x1; /* Physically contiguous, no interrupts */
	err = nvme_admin_sync_dmamem(ctrlr, &cmd, 2, &cpl);
	if (err) {
		UPCIE_DEBUG("FAILED: CREATE_IO_CQ(qid=%u); err(%d)", qid, err);
		goto rollback_qpair;
	}

	memset(&cmd, 0, sizeof(cmd));
	memset(&cpl, 0, sizeof(cpl));
	cmd.opc = 0x1; /* Create I/O Submission Queue */
	cmd.prp1 = sq_iova;
	cmd.cdw10 = ((uint32_t)(depth - 1) << 16) | qid;
	cmd.cdw11 = ((uint32_t)qid << 16) | 0x1; /* CQID | physically contiguous */
	err = nvme_admin_sync_dmamem(ctrlr, &cmd, 3, &cpl);
	if (err) {
		struct nvme_command drop = {0};
		struct nvme_completion drop_cpl = {0};

		UPCIE_DEBUG("FAILED: CREATE_IO_SQ(qid=%u); err(%d)", qid, err);
		drop.opc = 0x4; /* Delete I/O Completion Queue */
		drop.cdw10 = qid;
		(void)nvme_admin_sync_dmamem(ctrlr, &drop, 4, &drop_cpl);
		goto rollback_qpair;
	}

	return 0;

rollback_qpair:
	nvme_qpair_dmamem_term(qp, heap, *sq_offset_out, *cq_offset_out, *prp_offset_out);
	nvme_qid_free(ctrlr->qids, qid);
	return err;
}

/**
 * Tear down an I/O queue pair created with the dmamem variant.
 */
static inline int
nvme_controller_delete_io_qpair_dmamem(struct nvme_controller *ctrlr, struct nvme_qpair *qp,
				       struct dmamem_heap *heap, size_t sq_offset, size_t cq_offset,
				       size_t prp_offset)
{
	struct nvme_command cmd = {0};
	struct nvme_completion cpl = {0};
	uint16_t qid = qp->qid;
	int first_err = 0;
	int err;

	cmd.opc = 0x0; /* Delete I/O Submission Queue */
	cmd.cdw10 = qid;
	err = nvme_admin_sync_dmamem(ctrlr, &cmd, 5, &cpl);
	if (err && !first_err) {
		first_err = err;
	}

	memset(&cmd, 0, sizeof(cmd));
	memset(&cpl, 0, sizeof(cpl));
	cmd.opc = 0x4; /* Delete I/O Completion Queue */
	cmd.cdw10 = qid;
	err = nvme_admin_sync_dmamem(ctrlr, &cmd, 6, &cpl);
	if (err && !first_err) {
		first_err = err;
	}

	nvme_qpair_dmamem_term(qp, heap, sq_offset, cq_offset, prp_offset);
	nvme_qid_free(ctrlr->qids, qid);
	return first_err;
}
