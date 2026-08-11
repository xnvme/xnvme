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
 * @version 0.6.0
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
