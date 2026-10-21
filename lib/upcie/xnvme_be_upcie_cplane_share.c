// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/*
 * The server side of the control plane that needs the runtime: what a client of a
 * controller is handed, and the heap and admin queue operations done on its behalf.
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_be_upcie.h>
#include <xnvme_lock.h>

/* Client payloads are carved from granules of their own, apart from queue memory */
#define PAYLOAD_GRANULE (2ULL * 1024 * 1024)

static int
_xnvme_be_upcie_cplane_share_get(struct xnvme_dev *dev, struct xnvme_be_upcie_cplane_share *out)
{
	struct xnvme_be_upcie_state *state;
	struct hostmem_shared_desc *desc;
	struct nvme_controller *ctrl;
	size_t desc_offset, desc_nbytes;
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

	/* A base per granule for physical addresses; the header alone behind an IOMMU */
	desc_nbytes = (g_upcie_rte.mode == XNVME_BE_UPCIE_MODE_UIO_LUT)
			      ? hostmem_shared_desc_nbytes(g_upcie_rte.mem.hp.nphys)
			      : sizeof(struct hostmem_shared_desc);

	err = dmamem_heap_alloc(&g_upcie_rte.mem.heap, desc_nbytes, &desc_offset);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_heap_alloc(desc); err(%d)", err);
		return err;
	}

	desc = (struct hostmem_shared_desc *)(heap_base + desc_offset);

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

	memset(out, 0, sizeof(*out));
	/* The dmamem's own descriptor, or the hugepage's where the dmamem wraps one */
	out->heap_fd =
		(g_upcie_rte.mem.dmem.fd >= 0) ? g_upcie_rte.mem.dmem.fd : g_upcie_rte.mem.hp.fd;
	out->bar0_fd = ctrl->func.bars[0].fd;
	out->bar0_nbytes = ctrl->func.bars[0].size;
	out->heap_nbytes = g_upcie_rte.mem.dmem.size;
	out->base = ctrl->base;
	out->desc_offset = desc_offset;
	snprintf(out->uri, sizeof(out->uri), "%s", dev->ident.uri);
	out->live = 1;

	return 0;

failed:
	dmamem_heap_free(&g_upcie_rte.mem.heap, desc_offset);

	return err;
}

int
xnvme_be_upcie_cplane_share_get(struct xnvme_dev *dev, struct xnvme_be_upcie_cplane_share *out)
{
	int err;

	xnvme_lock();
	err = _xnvme_be_upcie_cplane_share_get(dev, out);
	xnvme_unlock();

	return err;
}

static void
_xnvme_be_upcie_cplane_share_put(struct xnvme_be_upcie_cplane_share *share)
{
	if (!share) {
		return;
	}

	if (!share->live) {
		return;
	}

	dmamem_heap_free(&g_upcie_rte.mem.heap, share->desc_offset);

	memset(share, 0, sizeof(*share));
}

void
xnvme_be_upcie_cplane_share_put(struct xnvme_be_upcie_cplane_share *share)
{
	xnvme_lock();
	_xnvme_be_upcie_cplane_share_put(share);
	xnvme_unlock();
}

int
xnvme_be_upcie_cplane_serve_admin_cmd(struct xnvme_dev *dev, void *cmd, void *cpl)
{
	struct xnvme_be_upcie_state *state;
	struct nvme_controller *ctrl;
	int err;

	if (!dev || !cmd || !cpl) {
		return -EINVAL;
	}

	state = (void *)dev->be.state;
	ctrl = state->ctrlr->ctrl;

	/* As xnvme_be_upcie_sync_cmd_admin() does, since queue setup shares the queue */
	xnvme_lock();
	err = nvme_qpair_submit_sync(&ctrl->aq, cmd, ctrl->base.admin_timeout_ms, cpl);
	xnvme_unlock();

	return err;
}

/*
 * Granules carved for client payloads. A drive writing payloads into the 2 MiB page that
 * holds its queues is measurably slower, and first-fit put them there, into the holes
 * the identify payloads leave while a client connects.
 */
static struct payload_granule {
	size_t base; /* Heap offset of the granule, aligned to PAYLOAD_GRANULE */
	size_t used; /* How much of it has been handed out */
	int nallocs; /* Live allocations in it; the granule goes back at zero */
} *g_payload;
static int g_payload_nslots;

static int
_xnvme_be_upcie_cplane_serve_buf_alloc(size_t nbytes, uint64_t *offset)
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

	/* A granule or more already gets pages of its own */
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

	for (int i = 0; i < g_payload_nslots; ++i) {
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
		int nslots = g_payload_nslots ? g_payload_nslots * 2 : 64;
		struct payload_granule *grown;

		grown = realloc(g_payload, nslots * sizeof(*grown));
		if (!grown) {
			XNVME_DEBUG("FAILED: realloc(payload granules: %d)", nslots);
			return -ENOMEM;
		}
		memset(&grown[g_payload_nslots], 0, (nslots - g_payload_nslots) * sizeof(*grown));

		free_slot = g_payload_nslots;
		g_payload = grown;
		g_payload_nslots = nslots;
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

int
xnvme_be_upcie_cplane_serve_buf_alloc(size_t nbytes, uint64_t *offset)
{
	int err;

	xnvme_lock();
	err = _xnvme_be_upcie_cplane_serve_buf_alloc(nbytes, offset);
	xnvme_unlock();

	return err;
}

static int
_xnvme_be_upcie_cplane_serve_buf_free(uint64_t offset)
{
	if (!g_upcie_rte.mem.heap_alive) {
		return -ENOTCONN;
	}

	for (int i = 0; i < g_payload_nslots; ++i) {
		if (!g_payload[i].nallocs || (offset < g_payload[i].base) ||
		    (offset >= (g_payload[i].base + PAYLOAD_GRANULE))) {
			continue;
		}

		/* Bump-allocated, so the granule goes back with its last allocation */
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

int
xnvme_be_upcie_cplane_serve_buf_free(uint64_t offset)
{
	int err;

	xnvme_lock();
	err = _xnvme_be_upcie_cplane_serve_buf_free(offset);
	xnvme_unlock();

	return err;
}
#endif
