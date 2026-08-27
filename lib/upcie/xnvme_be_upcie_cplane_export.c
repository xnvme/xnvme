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

	if (!dev || !cmd || !cpl) {
		return -EINVAL;
	}

	state = (void *)dev->be.state;
	ctrl = state->ctrlr->ctrl;

	return nvme_qpair_submit_sync(&ctrl->aq, cmd, ctrl->timeout_ms, cpl);
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
#endif
