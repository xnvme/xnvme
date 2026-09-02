// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Describing a runtime this process owns, so another can connect to it
 *
 * The backend knows things homi would otherwise have to reach in and take: the
 * heap's descriptor, the controller's BAR, and where in the heap a client's
 * description of both can live. This puts them behind one call, so what serves
 * clients stays about serving.
 *
 * Everything published here is written once, when the runtime comes up. What a
 * client does with it afterwards, and what it is allocated, is homi's business.
 */
#include <errno.h>
#include <string.h>

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_be_upcie.h>

/**
 * Place the record and the heap description in the heap, and report the rest
 *
 * @param dev A device this process opened
 * @param out Pre-allocated export to fill
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_export(struct xnvme_dev *dev, struct xnvme_be_upcie_cplane_export *out)
{
	struct xnvme_be_upcie_state *state;
	struct nvme_runtime_record *record;
	struct hostmem_shared_desc *desc;
	struct nvme_controller *ctrl;
	size_t record_offset, desc_offset, desc_nbytes;
	char *heap_base;
	int err;

	if (!dev || !out) {
		return -EINVAL;
	}

	state = (void *)dev->be.state;
	ctrl = state->ctrlr->ctrl;
	heap_base = g_upcie_rte.mem.dmem.base_va;

	if (!g_upcie_rte.mem.heap_alive || !heap_base) {
		XNVME_DEBUG("FAILED: no heap to describe");
		return -ENOTCONN;
	}

	/* Sized by what it has to carry: a base per granule where the device
	 * consumes physical addresses, and nothing beyond the header where it
	 * translates through an address space. */
	desc_nbytes = (g_upcie_rte.mode == XNVME_BE_UPCIE_MODE_UIO_LUT)
			      ? hostmem_shared_desc_nbytes(g_upcie_rte.mem.hp.nphys)
			      : sizeof(struct hostmem_shared_desc);

	err = dmamem_heap_alloc(&g_upcie_rte.mem.heap, desc_nbytes, &desc_offset);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_heap_alloc(desc); err(%d)", err);
		return err;
	}

	err = dmamem_heap_alloc(&g_upcie_rte.mem.heap, sizeof(*record), &record_offset);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_heap_alloc(record); err(%d)", err);
		dmamem_heap_free(&g_upcie_rte.mem.heap, desc_offset);
		return err;
	}

	desc = (struct hostmem_shared_desc *)(heap_base + desc_offset);
	record = (struct nvme_runtime_record *)(heap_base + record_offset);

	if (g_upcie_rte.mode == XNVME_BE_UPCIE_MODE_UIO_LUT) {
		err = hostmem_shared_desc_fill(desc, &g_upcie_rte.mem.hp);
	} else {
		err = hostmem_shared_desc_fill_arithmetic(desc, g_upcie_rte.mem.dmem.size,
							  g_upcie_rte.mem.dmem.base_iova);
	}
	if (err) {
		XNVME_DEBUG("FAILED: describing the heap; err(%d)", err);
		goto failed;
	}

	err = nvme_runtime_record_export(ctrl, g_upcie_rte.mem.dmem.size, record);
	if (err) {
		XNVME_DEBUG("FAILED: nvme_runtime_record_export(); err(%d)", err);
		goto failed;
	}
	record->desc_offset = desc_offset;

	/* The controller's own bdf is filled by the paths that open through
	 * sysfs and left empty by the others, so the identifier a client was
	 * given is what travels: it is the same one they will name. */
	snprintf(record->bdf, sizeof(record->bdf), "%s", dev->ident.uri);

	memset(out, 0, sizeof(*out));
	/* Whichever descriptor actually backs the heap: the dmamem owns one
	 * where it allocated the memory itself, and wraps a hugepage where the
	 * device consumes physical addresses. Publishing the wrong one hands a
	 * client a descriptor it cannot map. */
	out->heap_fd =
		(g_upcie_rte.mem.dmem.fd >= 0) ? g_upcie_rte.mem.dmem.fd : g_upcie_rte.mem.hp.fd;
	out->bar0_fd = ctrl->func.bars[0].fd;
	out->bar0_nbytes = ctrl->func.bars[0].size;
	out->heap_nbytes = g_upcie_rte.mem.dmem.size;
	out->record_offset = record_offset;
	out->desc_offset = desc_offset;
	snprintf(out->uri, sizeof(out->uri), "%s", dev->ident.uri);
	out->live = 1;

	return 0;

failed:
	dmamem_heap_free(&g_upcie_rte.mem.heap, record_offset);
	dmamem_heap_free(&g_upcie_rte.mem.heap, desc_offset);

	return err;
}

/**
 * Release what an export took from the heap
 *
 * The record and the heap's description are allocated per export, so a process
 * that serves more than once would otherwise leave one of each behind per call.
 */
void
xnvme_be_upcie_cplane_unexport(struct xnvme_be_upcie_cplane_export *exported)
{
	if (!exported) {
		return;
	}

	if (!exported->live) {
		return;
	}

	dmamem_heap_free(&g_upcie_rte.mem.heap, exported->record_offset);
	dmamem_heap_free(&g_upcie_rte.mem.heap, exported->desc_offset);

	memset(exported, 0, sizeof(*exported));
}

/**
 * Submit an admin command on a client's behalf, and wait for it
 *
 * The payload does not come through here: the command names an address the
 * device can already reach, from memory the client registered or was
 * allocated, so what lands where is the client's arrangement.
 *
 * Waiting is the point rather than a compromise: the caller is a thread that
 * serves one connection, and it holds the controller's admin lock while it
 * waits. So a command as long as a Format holds up whoever else wants that
 * queue, and nobody else.
 *
 * @param dev A device this process opened
 * @param cmd A struct nvme_command to submit
 * @param cpl A struct nvme_completion to fill
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_admin(struct xnvme_dev *dev, void *cmd, void *cpl)
{
	struct xnvme_be_upcie_state *state;
	struct nvme_controller *ctrl;
	int err;

	if (!dev || !cmd || !cpl) {
		return -EINVAL;
	}

	state = (void *)dev->be.state;
	ctrl = state->ctrlr->ctrl;

	err = nvme_qpair_submit_sync(&ctrl->aq, cmd, ctrl->timeout_ms, cpl);

	/* A command the controller refused did complete, and the completion is
	 * what says so. Reporting that as a failed request would leave the
	 * client with an errno and no status, unable to tell a command set it
	 * does not have from a drive that went away. So the status travels and
	 * only a command that never completed is an error here. */
	if ((err == -EIO) && (((struct nvme_completion *)cpl)->status & 0x1FE)) {
		return 0;
	}

	return err;
}

#define PAYLOAD_GRANULE (2ULL * 1024 * 1024)
#define PAYLOAD_GRANULES_MAX 64

/**
 * Hugepages carved for client payloads, and what still lives in each
 *
 * Payloads are kept off the hugepages holding queue memory. A drive fetching
 * SQEs and posting CQEs while writing payloads into the same 2 MiB page is
 * measurably slower at it: on a Samsung 9100 PRO, 512 B randread at qd128 fell
 * from 4.07M IOPS to 3.2M with the payload buffer a few pages below the SQ,
 * and recovered when only the buffer moved. First-fit put it there reliably,
 * since the identify payloads freed while connecting leave holes right under the
 * queues.
 */
static struct {
	size_t base; ///< Heap offset of the granule, aligned to PAYLOAD_GRANULE
	size_t used; ///< How much of it has been handed out
	int nallocs; ///< Live allocations in it; the granule goes back at zero
} g_payload[PAYLOAD_GRANULES_MAX];

/**
 * Allocate from the heap on a client's behalf
 *
 * A client cannot allocate here itself: the free list is a chain of this
 * process's addresses and has no lock. It asks and receives an offset, which
 * is what its own mapping resolves against.
 *
 * @param nbytes How much the client asked for
 * @param offset Set to where the memory begins in the heap
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_alloc_buf(size_t nbytes, uint64_t *offset)
{
	size_t at, want;
	int free_slot = -1;
	int err;

	if (!nbytes || !offset) {
		return -EINVAL;
	}
	if (!g_upcie_rte.mem.heap_alive) {
		return -ENOTCONN;
	}

	/* Anything from a granule upwards already gets a page to itself. */
	if (nbytes >= PAYLOAD_GRANULE) {
		err = dmamem_heap_alloc_aligned(&g_upcie_rte.mem.heap, nbytes, PAYLOAD_GRANULE,
						&at);
		if (err) {
			XNVME_DEBUG("FAILED: dmamem_heap_alloc_aligned(%zu); err(%d)", nbytes,
				    err);
			return err;
		}

		*offset = at;

		return 0;
	}

	want = (nbytes + 63) & ~(size_t)63;

	for (int i = 0; i < PAYLOAD_GRANULES_MAX; ++i) {
		if (!g_payload[i].nallocs) {
			free_slot = (free_slot < 0) ? i : free_slot;
			continue;
		}
		if ((PAYLOAD_GRANULE - g_payload[i].used) >= want) {
			*offset = g_payload[i].base + g_payload[i].used;
			g_payload[i].used += want;
			g_payload[i].nallocs++;

			return 0;
		}
	}

	if (free_slot < 0) {
		XNVME_DEBUG("FAILED: no room for another payload granule");
		return -ENOSPC;
	}

	err = dmamem_heap_alloc_aligned(&g_upcie_rte.mem.heap, PAYLOAD_GRANULE, PAYLOAD_GRANULE,
					&at);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_heap_alloc_aligned(granule); err(%d)", err);
		return err;
	}

	g_payload[free_slot].base = at;
	g_payload[free_slot].used = want;
	g_payload[free_slot].nallocs = 1;

	*offset = at;

	return 0;
}

/**
 * Free memory allocated for a client
 *
 * @param offset An offset from xnvme_be_upcie_cplane_alloc_buf()
 *
 * @return 0 on success, negative errno on error
 */
int
xnvme_be_upcie_cplane_free_buf(uint64_t offset)
{
	if (!g_upcie_rte.mem.heap_alive) {
		return -ENOTCONN;
	}

	for (int i = 0; i < PAYLOAD_GRANULES_MAX; ++i) {
		if (!g_payload[i].nallocs || (offset < g_payload[i].base) ||
		    (offset >= (g_payload[i].base + PAYLOAD_GRANULE))) {
			continue;
		}

		/* Handed out by bumping, so the space comes back only once the
		 * last of them does, which is when the granule itself goes. */
		if (--g_payload[i].nallocs) {
			return 0;
		}

		dmamem_heap_free(&g_upcie_rte.mem.heap, g_payload[i].base);
		g_payload[i].base = 0;
		g_payload[i].used = 0;

		return 0;
	}

	dmamem_heap_free(&g_upcie_rte.mem.heap, offset);

	return 0;
}

static int
_registration_lut(struct xnvme_be_upcie_cplane_registration *reg, uint64_t nbytes,
		  uint32_t page_size, uint32_t nphys, const uint64_t *phys)
{
	struct hostmem_shared_desc *desc;
	size_t desc_nbytes = hostmem_shared_desc_nbytes(nphys);
	uint64_t offset = 0;
	int shift, err;

	for (shift = 0; (1U << shift) < page_size; ++shift) {
		;
	}

	err = xnvme_be_upcie_cplane_alloc_buf(desc_nbytes, &offset);
	if (err) {
		XNVME_DEBUG("FAILED: no room for a description of %u granules", nphys);
		return err;
	}

	desc = (void *)((char *)g_upcie_rte.mem.dmem.base_va + offset);
	memset(desc, 0, desc_nbytes);
	desc->version = HOSTMEM_SHARED_DESC_VERSION;
	desc->kind = HOSTMEM_SHARED_LUT;
	desc->nbytes = nbytes;
	desc->nphys = nphys;
	desc->gran_shift = (uint32_t)shift;
	memcpy(desc->phys, phys, sizeof(*phys) * nphys);

	reg->desc_offset = offset;

	return 0;
}

/**
 * The window every mapping this server installs is handed out from
 *
 * One per process, claimed from the IOAS before anything is placed in it, so
 * what goes here cannot collide with what iommufd allocates later. Opened with
 * the first region that needs it and closed with the last, since closing drops
 * every mapping made on it.
 */
static struct dmamem_iommu_map_pa g_imp;
static int g_imp_refs;

/**
 * Claim the window, or take another reference on it
 *
 * @param bdf The controller whose domain regions are mapped into
 *
 * @return 0 on success, negative errno on failure
 */
static int
_imp_get(const char *bdf)
{
	int err;

	if (g_imp_refs) {
		++g_imp_refs;
		return 0;
	}

	err = dmamem_iommu_map_pa_open(&g_imp, bdf, XNVME_BE_UPCIE_IMP_WINDOW_BASE,
				       XNVME_BE_UPCIE_IMP_WINDOW_SIZE);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_iommu_map_pa_open(%s); err(%d)", bdf, err);
		return err;
	}

	err = dmamem_iommu_map_pa_reserve_window(&g_imp, &g_upcie_rte.cdev.iommufd);
	if (err) {
		XNVME_DEBUG("FAILED: reserving the window; err(%d)", err);
		dmamem_iommu_map_pa_close(&g_imp);
		return err;
	}

	g_imp_refs = 1;

	return 0;
}

/**
 * Drop a reference, closing the window with the last one
 */
static void
_imp_put(void)
{
	if (g_imp_refs && !--g_imp_refs) {
		dmamem_iommu_map_pa_close(&g_imp);
	}
}

static int
_registration_arithmetic(struct xnvme_be_upcie_cplane_registration *reg, int dmabuf_fd,
			 uint64_t nbytes, uint32_t page_size, uint32_t nphys, const uint64_t *phys,
			 const char *bdf)
{
	struct hostmem_shared_desc *desc;
	uint64_t iova_base, offset = 0;
	int err;

	err = _imp_get(bdf);
	if (err) {
		return err;
	}

	iova_base = dmamem_iommu_map_pa_window_alloc(&g_imp, nbytes, page_size);
	if (!iova_base) {
		XNVME_DEBUG("FAILED: no room in the window for %" PRIu64 " bytes", nbytes);
		_imp_put();
		return -ENOSPC;
	}

	err = iommu_map_pa_add(g_imp.fd, bdf, dmabuf_fd, iova_base, page_size, nphys, phys,
			       IOMMU_MAP_PA_PROT_READ | IOMMU_MAP_PA_PROT_WRITE, &reg->map_handle);
	if (err) {
		XNVME_DEBUG("FAILED: iommu_map_pa_add(%s); err(%d)", bdf, err);
		_imp_put();
		return err;
	}
	reg->map_fd = g_imp.fd;

	err = xnvme_be_upcie_cplane_alloc_buf(sizeof(*desc), &offset);
	if (err) {
		XNVME_DEBUG("FAILED: no room for a description; err(%d)", err);
		iommu_map_pa_del(g_imp.fd, reg->map_handle);
		_imp_put();
		reg->map_fd = -1;
		return err;
	}

	desc = (void *)((char *)g_upcie_rte.mem.dmem.base_va + offset);
	hostmem_shared_desc_fill_arithmetic(desc, nbytes, iova_base);
	reg->desc_offset = offset;

	return 0;
}

int
xnvme_be_upcie_cplane_register_mem(int dmabuf_fd, uint64_t nbytes, uint32_t page_size,
				   const char *bdf, struct xnvme_be_upcie_cplane_registration *out)
{
	uint64_t *phys = NULL;
	uint32_t nphys;
	int err;

	if ((dmabuf_fd < 0) || !nbytes || !page_size || !bdf || !out) {
		return -EINVAL;
	}
	if (page_size & (page_size - 1)) {
		XNVME_DEBUG("FAILED: page_size(%u) is not a power of two", page_size);
		return -EINVAL;
	}
	if (nbytes % page_size) {
		XNVME_DEBUG("FAILED: nbytes(%" PRIu64 ") is not a multiple of page_size(%u)",
			    nbytes, page_size);
		return -EINVAL;
	}
	if (!g_upcie_rte.mem.heap_alive) {
		return -ENOTCONN;
	}

	memset(out, 0, sizeof(*out));
	out->map_fd = -1;

	err = dmabuf_import_attach(dmabuf_fd, &out->dmabuf);
	if (err) {
		XNVME_DEBUG("FAILED: dmabuf_import_attach(); err(%d)", err);
		return err;
	}
	out->attached = 1;

	nphys = (uint32_t)(nbytes / page_size);
	phys = calloc(nphys, sizeof(*phys));
	if (!phys) {
		err = -errno;
		goto failed;
	}

	err = dmabuf_get_lut(&out->dmabuf, nphys, phys, page_size);
	if (err) {
		XNVME_DEBUG("FAILED: dmabuf_get_lut(); err(%d)", err);
		goto failed;
	}

	/* What the controller consumes decides the shape of the answer: physical
	 * addresses where the IOMMU is out of the way, and a single base where
	 * it is, since the mapping installs the granules behind it. */
	if (g_upcie_rte.mode == XNVME_BE_UPCIE_MODE_UIO_LUT) {
		err = _registration_lut(out, nbytes, page_size, nphys, phys);
	} else {
		err = _registration_arithmetic(out, dmabuf_fd, nbytes, page_size, nphys, phys,
					       bdf);
	}
	if (err) {
		goto failed;
	}

	free(phys);

	return 0;

failed:
	free(phys);
	xnvme_be_upcie_cplane_unregister_mem(out);

	return err;
}

void
xnvme_be_upcie_cplane_unregister_mem(struct xnvme_be_upcie_cplane_registration *reg)
{
	if (!reg) {
		return;
	}

	if (reg->map_fd >= 0) {
		if (reg->map_handle) {
			iommu_map_pa_del(reg->map_fd, reg->map_handle);
		}
		_imp_put();
	}
	if (reg->desc_offset) {
		xnvme_be_upcie_cplane_free_buf(reg->desc_offset);
	}
	if (reg->attached) {
		dmabuf_import_detach(&reg->dmabuf);
	}

	memset(reg, 0, sizeof(*reg));
	reg->map_fd = -1;
}

#endif
