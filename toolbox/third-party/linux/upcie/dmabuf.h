// SPDX-License-Identifier: BSD-3-Clause

/**
 * Get physical addresses from a dma-buf
 * =====================================
 *
 * This is a generic interface that is compatible with any dma-buf.
 * A dma-buf FD can be obtained either from host memory, memfd->udmabuf, or
 * device memory, e.g., CUDA or ROCm.
 *
 * NOTE: This depends on a patch for UDMABUF adding capabilites to import a
 * dma-buf and return the physical addresses to Userspace
 *
 */

#include <linux/dma-buf.h>
#include <linux/udmabuf.h>

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

#ifdef UDMABUF_ATTACH
/**
 * Attach to dma-buf with given FD
 * 
 * Populates the given dma-buf structure with information about the dma-buf. 
 */
static inline int
dmabuf_attach(int dmabuf_fd, struct dmabuf *dmabuf)
{
	struct udmabuf_attach attach;
	struct udmabuf_get_map *map = NULL;
	int udmabuf_fd, err;
	size_t map_size, pages_size;

	udmabuf_fd = open("/dev/udmabuf", O_RDWR);
	if (udmabuf_fd < 0) {
		err = -errno;
		UPCIE_DEBUG("FAILED: open(/dev/udmabuf), errno: %d", err);
		return err;
	}

	memset(&attach, 0, sizeof(attach));
	attach.fd = dmabuf_fd;

	err = ioctl(udmabuf_fd, UDMABUF_ATTACH, &attach);
	if (err) {
		err = -errno;
		UPCIE_DEBUG("FAILED: ioctl(UDMABUF_ATTACH), errno: %d", err);
		goto exit;
	}

	map_size = attach.count * sizeof(struct udmabuf_dma_map);
	map = malloc(sizeof(struct udmabuf_get_map) + map_size);
	if (!map) {
		err = -errno;
		UPCIE_DEBUG("FAILED: malloc(map), errno: %d", err);
		ioctl(udmabuf_fd, UDMABUF_DETACH, &dmabuf_fd);
		goto exit;
	}

	memset(map, 0, sizeof(*map));
	map->fd = dmabuf_fd;
	map->count = attach.count;

	err = ioctl(udmabuf_fd, UDMABUF_GET_MAP, map);
	if (err) {
		err = -errno;
		UPCIE_DEBUG("FAILED: ioctl(UDMABUF_GET_MAP), errno: %d\n", err);
		ioctl(udmabuf_fd, UDMABUF_DETACH, &dmabuf_fd);
		goto exit;
	}

	memset(dmabuf, 0, sizeof(*dmabuf));
	dmabuf->fd = dmabuf_fd;
	dmabuf->npages = map->count;
	pages_size = sizeof(struct dmabuf_page) * dmabuf->npages;
	dmabuf->pages = malloc(pages_size);
	if (!dmabuf->pages) {
		err = -errno;
		UPCIE_DEBUG("FAILED: malloc(dmabuf->pages), errno: %d", err);
		ioctl(udmabuf_fd, UDMABUF_DETACH, &dmabuf_fd);
		goto exit;
	}

	memcpy(dmabuf->pages, map->dma_arr, pages_size);

exit:
	free(map);
	close(udmabuf_fd);
	return err;
}
#else
static inline int
dmabuf_attach(int UPCIE_UNUSED(dmabuf_fd), struct dmabuf *UPCIE_UNUSED(dmabuf))
{
	UPCIE_DEBUG("FAILED: ioctl(UDMABUF_ATTACH) not supported by Linux Kernel");
	return -ENOTSUP;
}
#endif // UDMABUF_ATTACH


#ifdef UDMABUF_DETACH
/**
 * Detach from given dma-buf
 *
 * NOTE: This doesn't free the underlying memory
 */
static inline int
dmabuf_detach(struct dmabuf *dmabuf)
{
	int udmabuf_fd, err;

	free(dmabuf->pages);

	udmabuf_fd = open("/dev/udmabuf", O_RDWR);
	if (udmabuf_fd < 0) {
		err = -errno;
		UPCIE_DEBUG("FAILED: open(/dev/udmabuf), errno: %d", err);
		return err;
	}

	err = ioctl(udmabuf_fd, UDMABUF_DETACH, &dmabuf->fd);
	if (err) {
		err = -errno;
		UPCIE_DEBUG("FAILED: ioctl(UDMABUF_DETACH), errno: %d\n", err);
		// fall-through
	}

	close(dmabuf->fd);
	close(udmabuf_fd);
	return err;
}
#else
static inline int
dmabuf_detach(struct dmabuf *UPCIE_UNUSED(dmabuf))
{
	UPCIE_DEBUG("FAILED: ioctl(UDMABUF_DETACH) not supported by Linux Kernel");
	return -ENOTSUP;
}
#endif // UDMABUF_DETACH
