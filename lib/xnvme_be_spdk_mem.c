// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_SPDK_ENABLED
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <spdk/env.h>
#include <spdk/memory.h>
#include <xnvme_dev.h>

void *
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

void *
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

void
xnvme_be_spdk_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	spdk_dma_free(buf);
}

int
xnvme_be_spdk_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf, uint64_t *phys)
{
	*phys = spdk_vtophys(buf, NULL);
	if (SPDK_VTOPHYS_ERROR == *phys) {
		return -EIO;
	}
	return 0;
}

/**
 * A range registered with spdk_mem_register()
 *
 * SPDK registers memory for the process, not for a device, refuses to register a range twice, and
 * needs the length to unregister it. A range mapped with several devices is thus registered once
 * and counted, and unregistered when the last mapping of it is given back. The list is only
 * touched by xnvme_mem_map() and xnvme_mem_unmap(), which hold the control-plane lock.
 */
struct xnvme_be_spdk_mem_reg {
	void *vaddr;
	size_t nbytes;
	uint32_t refs;
	struct xnvme_be_spdk_mem_reg *next;
};

static struct xnvme_be_spdk_mem_reg *g_xnvme_be_spdk_mem_regs;

/**
 * Whether the range [lo, hi) lies in hugetlb pages of 2 MiB or more, which stay where they are
 *
 * Without an IOMMU, SPDK hands the device the physical address that /proc/self/pagemap gives at
 * registration. Ordinary pages can move after that, so only hugetlb pages are safe to register.
 */
static bool
_range_is_hugetlb(uintptr_t lo, uintptr_t hi)
{
#ifdef __linux__
	unsigned long start = 0, end = 0, kps;
	uintptr_t cursor = lo;
	char line[4096];
	FILE *fp;

	fp = fopen("/proc/self/smaps", "r");
	if (!fp) {
		XNVME_DEBUG("FAILED: fopen(/proc/self/smaps), errno: %d", errno);
		return false;
	}

	while (cursor < hi && fgets(line, sizeof(line), fp)) {
		if (sscanf(line, "%lx-%lx ", &start, &end) == 2) {
			continue;
		}
		if (sscanf(line, "KernelPageSize: %lu kB", &kps) != 1) {
			continue;
		}
		if (end <= cursor) {
			continue;
		}
		if (start > cursor || kps < (VALUE_2MB >> 10)) {
			break;
		}
		cursor = end;
	}

	fclose(fp);

	return cursor >= hi;
#else
	(void)lo;
	(void)hi;
	return false;
#endif
}

static int
xnvme_be_spdk_mem_map(const struct xnvme_dev *XNVME_UNUSED(dev), void *vaddr, size_t nbytes,
		      uint64_t *phys)
{
	struct xnvme_be_spdk_mem_reg *reg;
	int err;

	if (!vaddr || !nbytes || ((uintptr_t)vaddr | nbytes) % VALUE_2MB) {
		XNVME_DEBUG(
			"FAILED: vaddr(%p) and nbytes(%zu) must be non-zero multiples of 2 MiB",
			vaddr, nbytes);
		errno = EINVAL;
		return -EINVAL;
	}

	for (reg = g_xnvme_be_spdk_mem_regs; reg; reg = reg->next) {
		if (reg->vaddr == vaddr) {
			break;
		}
	}
	if (reg && reg->nbytes != nbytes) {
		XNVME_DEBUG("FAILED: vaddr(%p) is mapped with nbytes(%zu), not %zu", vaddr,
			    reg->nbytes, nbytes);
		errno = EBUSY;
		return -EBUSY;
	}

	if (!reg) {
		if (!spdk_iommu_is_enabled() &&
		    !_range_is_hugetlb((uintptr_t)vaddr, (uintptr_t)vaddr + nbytes)) {
			XNVME_DEBUG("FAILED: no IOMMU and vaddr(%p) is not in hugetlb pages",
				    vaddr);
			errno = ENOTSUP;
			return -ENOTSUP;
		}

		reg = calloc(1, sizeof(*reg));
		if (!reg) {
			errno = ENOMEM;
			return -ENOMEM;
		}

		err = spdk_mem_register(vaddr, nbytes);
		if (err) {
			XNVME_DEBUG("FAILED: spdk_mem_register(%p, %zu), err: %d", vaddr, nbytes,
				    err);
			free(reg);
			errno = -err;
			return err;
		}

		reg->vaddr = vaddr;
		reg->nbytes = nbytes;
		reg->next = g_xnvme_be_spdk_mem_regs;
		g_xnvme_be_spdk_mem_regs = reg;
	}
	reg->refs += 1;

	if (phys) {
		*phys = spdk_vtophys(vaddr, NULL);
	}

	return 0;
}

static int
xnvme_be_spdk_mem_unmap(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	struct xnvme_be_spdk_mem_reg **link, *reg;
	int err;

	for (link = &g_xnvme_be_spdk_mem_regs; *link; link = &(*link)->next) {
		if ((*link)->vaddr == buf) {
			break;
		}
	}
	reg = *link;
	if (!reg) {
		XNVME_DEBUG("FAILED: buf(%p) is not mapped", buf);
		errno = EINVAL;
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
		errno = -err;
		return err;
	}

	*link = reg->next;
	free(reg);

	return 0;
}

static int
xnvme_be_spdk_mem_map_req(const struct xnvme_dev *XNVME_UNUSED(dev), struct xnvme_mem_map_req *req)
{
	req->align = VALUE_2MB;

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
	.mem_map = xnvme_be_spdk_mem_map,
	.mem_unmap = xnvme_be_spdk_mem_unmap,
	.mem_map_req = xnvme_be_spdk_mem_map_req,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#endif
};
