// SPDX-License-Identifier: BSD-3-Clause

/**
 * Representation of a dma-buf and its physical pages
 * ==================================================
 *
 * A generic interface compatible with any dma-buf. A dma-buf descriptor can be
 * obtained from host memory (memfd via udmabuf) or from device memory, e.g.
 * CUDA or ROCm.
 *
 * This header is dependency-free: it describes a dma-buf and segments its
 * pages, and needs nothing beyond libc. Resolving the DMA addresses behind a
 * dma-buf in the first place needs the out-of-tree dmabuf_import module and
 * lives in <upcie/experimental/dmabuf_import.h>.
 *
 * @file dmabuf.h
 * @version 0.9.0
 */

struct dmabuf_page {
	uint64_t addr;			///< Address of a page
	uint64_t len;			///< Length of the page (can span multiple phys pages)
};

struct dmabuf {
	int fd;				///< dma-buf file descriptor
	size_t npages;			///< Number of pages in the dma-buf
	struct dmabuf_page *pages;	///< Array of pages in the dma-buf
};

/**
 * Print information about the given dma-buf and each of it's pages
 */
static inline int
dmabuf_pp(struct dmabuf *dmabuf)
{
	int wrtn = 0;

	wrtn += printf("dmabuf:");

	if (!dmabuf) {
		wrtn += printf(" ~\n");
		return 0;
	}

	wrtn += printf("\n");
	wrtn += printf("  fd: %d\n", dmabuf->fd);
	wrtn += printf("  npages: %zu\n", dmabuf->npages);
	wrtn += printf("  pages:\n");
	for (size_t i = 0; i < dmabuf->npages; ++i) {
		struct dmabuf_page page = dmabuf->pages[i];
		wrtn += printf("  - addr: 0x%" PRIx64 ", len: %" PRIu64 "\n", page.addr, page.len);
	}

	return wrtn;
}

/**
 * Get LUT (lookup table) from dma-buf
 *
 * The pages in the dma-buf might span multiple physical pages.
 * This function creates a LUT segmented to fit the provided page_size.
 *
 * NOTE: Requires pre-allocated phys_lut
 */
static inline int
dmabuf_get_lut(struct dmabuf *dmabuf, size_t nphys, uint64_t *phys_lut, uint64_t page_size)
{
	size_t i = 0;

	for (uint32_t j = 0; j < dmabuf->npages; j++) {
		// handle a single address for multiple pages
		for (uint64_t k = 0; k < dmabuf->pages[j].len / page_size; k++) {
			if (i >= nphys) {
				UPCIE_DEBUG("FAILED: dmabuf (%zu) has more pages than expected (%zu)", i, nphys);
				return -EINVAL;
			}

			phys_lut[i] = dmabuf->pages[j].addr + k * page_size;
			i++;
		}
	}

	if (i != nphys) {
		UPCIE_DEBUG("FAILED: LUT is not full: actual < expected (%zu < %zu)", i, nphys);
		return -EINVAL;
	}

	return 0;
}

/**
 * Summarise a dma-buf into one address per granule
 *
 * Where dmabuf_get_lut() expands the scatter list per page, this collapses it
 * per `granule`, for a translator indexing by `va >> granule_shift`.
 *
 * Each granule is verified physically contiguous rather than assumed: an
 * exporter may split a contiguous run at its own page size, which is harmless,
 * but a genuine discontinuity would make `base + offset` resolve wrongly.
 *
 * And it is tolerant at both ends, because neither vendor's export lines up
 * with the allocation size it reports. An export describing more than `nlut`
 * granules is fine, the surplus is ignored; so is a final granule the export
 * only partially covers, which is what an allocation not ending on a granule
 * boundary produces. What is not fine is a granule the export does not reach
 * at all, since nothing would fill its entry.
 *
 * @param dmabuf  Attached dma-buf to read the scatter list from
 * @param lut     Destination, `nlut` entries, one per granule from the start
 * @param nlut    Number of granules to fill
 * @param granule Bytes per entry; a power of two
 *
 * @return 0 on success, -EINVAL on bad arguments or a granule the export does
 *         not reach, -EOPNOTSUPP when a granule is not contiguous.
 */
static inline int
dmabuf_get_granule_lut(struct dmabuf *dmabuf, uint64_t *lut, size_t nlut, uint64_t granule)
{
	uint64_t off = 0;
	size_t filled = 0;

	if (!dmabuf || !lut || !nlut || !granule || (granule & (granule - 1))) {
		return -EINVAL;
	}

	for (size_t j = 0; j < dmabuf->npages; ++j) {
		const uint64_t addr = dmabuf->pages[j].addr;
		const uint64_t len = dmabuf->pages[j].len;
		const size_t g = (size_t)(off / granule);

		if (g >= nlut) {
			break;
		}

		if (g == filled) {
			/* Unopened only on a boundary, so off is aligned. */
			lut[g] = addr - (off % granule);
			filled = g + 1;
		} else if (addr != lut[g] + (off % granule)) {
			UPCIE_DEBUG("FAILED: granule(%zu) not contiguous at off(0x%" PRIx64 ")", g,
				    off);
			return -EOPNOTSUPP;
		}

		for (size_t k = filled; k < nlut && (uint64_t)k * granule < off + len; ++k) {
			lut[k] = addr + ((uint64_t)k * granule - off);
			filled = k + 1;
		}

		off += len;
	}

	if (filled < nlut) {
		UPCIE_DEBUG("FAILED: export describes %zu granules, need %zu", filled, nlut);
		return -EINVAL;
	}

	return 0;
}
