// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/*
 * What a server hands a client of a controller: the descriptors and the description of
 * the heap the client maps to reach the controller.
 */
#include <errno.h>
#include <string.h>

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_cplane_share.h>
#include <xnvme_lock.h>

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

#endif
