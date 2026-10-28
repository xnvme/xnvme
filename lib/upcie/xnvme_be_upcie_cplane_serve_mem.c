// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

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

static int
_registration_lut(struct xnvme_be_upcie_cplane_registration *reg, uint64_t nbytes,
		  uint32_t page_size, uint32_t nphys, const uint64_t *phys)
{
	struct hostmem_shared_desc *desc;
	size_t desc_nbytes = hostmem_shared_desc_nbytes(nphys);
	uint64_t offset = 0;
	int err;

	err = xnvme_be_upcie_cplane_serve_buf_alloc(desc_nbytes, &offset);
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
	desc->gran_shift = (uint32_t)XNVME_ILOG2(page_size);
	memcpy(desc->phys, phys, sizeof(*phys) * nphys);

	reg->desc_offset = offset;

	return 0;
}

static struct xnvme_be_upcie_iova_range g_iova_range;
static int g_imp_refs;

static int
_imp_get(const char *bdf)
{
	int err;

	if (g_imp_refs) {
		err = xnvme_be_upcie_iova_range_attach(&g_iova_range, bdf);
		if (err) {
			XNVME_DEBUG("FAILED: xnvme_be_upcie_iova_range_attach(%s); err(%d)", bdf,
				    err);
			return err;
		}
		++g_imp_refs;

		return 0;
	}

	err = xnvme_be_upcie_iova_range_open(&g_iova_range, bdf);
	if (err) {
		return err;
	}

	g_imp_refs = 1;

	return 0;
}

static void
_imp_put(void)
{
	if (g_imp_refs && !--g_imp_refs) {
		xnvme_be_upcie_iova_range_close(&g_iova_range);
	}
}

static struct _cplane_region {
	uint64_t phys0;
	uint64_t nbytes;
	uint32_t page_size;

	uint64_t desc_offset;
	uint64_t map_handle;
	uint64_t iova_base;
	int map_fd;
	int refs;
} g_regions[XNVME_BE_UPCIE_CPLANE_REGIONS_MAX];

/* Not the handle's own allocator: the regions are installed with iommu_map_pa_add()
 * directly, so it would hand every client the same IOVA */
static uint64_t
_region_iova_alloc(uint64_t nbytes, uint64_t align)
{
	const struct dmamem_iommu_map_pa *imp = g_iova_range.imp;
	uint64_t cand;
	int moved = 1;

	if (!imp || !align) {
		return 0;
	}

	cand = (imp->window_base + align - 1) & ~(align - 1);

	while (moved) {
		moved = 0;
		for (int i = 0; i < XNVME_BE_UPCIE_CPLANE_REGIONS_MAX; ++i) {
			const struct _cplane_region *region = &g_regions[i];

			if (!region->refs || !region->iova_base) {
				continue;
			}
			if ((cand < (region->iova_base + region->nbytes)) &&
			    ((cand + nbytes) > region->iova_base)) {
				cand = (region->iova_base + region->nbytes + align - 1) &
				       ~(align - 1);
				moved = 1;
			}
		}
		for (const struct dmamem_iommu_map_pa_mapping *m = imp->maps; m; m = m->next) {
			if ((cand < (m->iova_base + m->nbytes)) &&
			    ((cand + nbytes) > m->iova_base)) {
				cand = (m->iova_base + m->nbytes + align - 1) & ~(align - 1);
				moved = 1;
			}
		}
	}

	if ((cand < imp->window_base) ||
	    ((cand + nbytes) > (imp->window_base + imp->window_size))) {
		return 0;
	}

	return cand;
}

static struct _cplane_region *
_region_slot(uint64_t phys0, uint64_t nbytes, uint32_t page_size)
{
	struct _cplane_region *free_slot = NULL;

	for (int i = 0; i < XNVME_BE_UPCIE_CPLANE_REGIONS_MAX; ++i) {
		struct _cplane_region *region = &g_regions[i];

		if (region->refs && (region->phys0 == phys0) && (region->nbytes == nbytes) &&
		    (region->page_size == page_size)) {
			return region;
		}
		if (!free_slot && !region->refs) {
			free_slot = region;
		}
	}

	return free_slot;
}

static int
_registration_arithmetic(struct xnvme_be_upcie_cplane_registration *reg, int dmabuf_fd,
			 uint64_t nbytes, uint32_t page_size, uint32_t nphys, const uint64_t *phys,
			 const char *bdf)
{
	struct hostmem_shared_desc *desc;
	uint64_t iova_base, offset = 0;
	int err;

	struct _cplane_region *region;

	err = _imp_get(bdf);
	if (err) {
		return err;
	}

	region = _region_slot(phys[0], nbytes, page_size);
	if (!region) {
		XNVME_DEBUG("FAILED: more than %d regions installed",
			    XNVME_BE_UPCIE_CPLANE_REGIONS_MAX);
		_imp_put();
		return -ENOSPC;
	}
	if (region->refs) {
		++region->refs;
		reg->desc_offset = region->desc_offset;
		reg->region = (int)(region - g_regions);

		return 0;
	}

	iova_base = _region_iova_alloc(nbytes, page_size);
	if (!iova_base) {
		XNVME_DEBUG("FAILED: no room in the range for %" PRIu64 " bytes", nbytes);
		_imp_put();
		return -ENOSPC;
	}

	err = iommu_map_pa_add(g_iova_range.imp->fd, bdf, dmabuf_fd, iova_base, page_size, nphys,
			       phys, IOMMU_MAP_PA_PROT_READ | IOMMU_MAP_PA_PROT_WRITE,
			       &region->map_handle);
	if (err) {
		XNVME_DEBUG("FAILED: iommu_map_pa_add(%s); err(%d)", bdf, err);
		_imp_put();
		return err;
	}
	region->map_fd = g_iova_range.imp->fd;

	err = xnvme_be_upcie_cplane_serve_buf_alloc(sizeof(*desc), &offset);
	if (err) {
		XNVME_DEBUG("FAILED: no room for a description; err(%d)", err);
		iommu_map_pa_del(region->map_fd, region->map_handle);
		memset(region, 0, sizeof(*region));
		_imp_put();
		return err;
	}

	desc = (void *)((char *)g_upcie_rte.mem.dmem.base_va + offset);
	hostmem_shared_desc_fill_arithmetic(desc, nbytes, iova_base);

	region->phys0 = phys[0];
	region->nbytes = nbytes;
	region->page_size = page_size;
	region->iova_base = iova_base;
	region->desc_offset = offset;
	region->refs = 1;

	reg->desc_offset = offset;
	reg->region = (int)(region - g_regions);

	return 0;
}

static int
_xnvme_be_upcie_cplane_serve_register_mem(int dmabuf_fd, uint64_t nbytes, uint32_t page_size,
					  const char *bdf,
					  struct xnvme_be_upcie_cplane_registration *out)
{
	uint64_t *phys = NULL;
	uint32_t nphys;
	int err;

	if ((dmabuf_fd < 0) || !nbytes || !page_size || !bdf || !out) {
		return -EINVAL;
	}
	if (!xnvme_is_pow2(page_size)) {
		XNVME_DEBUG("FAILED: page_size(%u) is not a power of two", page_size);
		return -EINVAL;
	}
	if (nbytes % page_size) {
		XNVME_DEBUG("FAILED: nbytes(%" PRIu64 ") is not a multiple of page_size(%u)",
			    nbytes, page_size);
		return -EINVAL;
	}
	if (nbytes / page_size > UINT32_MAX) {
		XNVME_DEBUG("FAILED: nbytes(%" PRIu64 ") is more pages than a table holds",
			    nbytes);
		return -EINVAL;
	}
	if (!g_upcie_rte.mem.heap_alive) {
		return -ENOTCONN;
	}

	memset(out, 0, sizeof(*out));
	out->map_fd = -1;
	out->region = -1;

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
	xnvme_be_upcie_cplane_serve_unregister_mem(out);

	return err;
}

int
xnvme_be_upcie_cplane_serve_register_mem(int dmabuf_fd, uint64_t nbytes, uint32_t page_size,
					 const char *bdf,
					 struct xnvme_be_upcie_cplane_registration *out)
{
	int err;

	xnvme_lock();
	err = _xnvme_be_upcie_cplane_serve_register_mem(dmabuf_fd, nbytes, page_size, bdf, out);
	xnvme_unlock();

	return err;
}

static void
_xnvme_be_upcie_cplane_serve_unregister_mem(struct xnvme_be_upcie_cplane_registration *reg)
{
	if (!reg) {
		return;
	}

	if (reg->region >= 0) {
		struct _cplane_region *region = &g_regions[reg->region];

		if (region->refs && !--region->refs) {
			iommu_map_pa_del(region->map_fd, region->map_handle);
			xnvme_be_upcie_cplane_serve_buf_free(region->desc_offset);
			memset(region, 0, sizeof(*region));
		}
		_imp_put();
	} else if (reg->desc_offset) {
		xnvme_be_upcie_cplane_serve_buf_free(reg->desc_offset);
	}
	if (reg->attached) {
		dmabuf_import_detach(&reg->dmabuf);
	}

	memset(reg, 0, sizeof(*reg));
	reg->map_fd = -1;
	reg->region = -1;
}

void
xnvme_be_upcie_cplane_serve_unregister_mem(struct xnvme_be_upcie_cplane_registration *reg)
{
	xnvme_lock();
	_xnvme_be_upcie_cplane_serve_unregister_mem(reg);
	xnvme_unlock();
}
#endif
