// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/*
 * Heap memory a server allocates on a client's behalf.
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_cplane_serve_mem.h>
#include <xnvme_lock.h>

/* Client payloads are carved from granules of their own, apart from queue memory */
#define PAYLOAD_GRANULE (2ULL * 1024 * 1024)

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
