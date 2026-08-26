// SPDX-License-Identifier: BSD-3-Clause

/**
 * Resolve a dma-buf's DMA addresses via the dmabuf_import module
 * ==============================================================
 *
 * ==========================================================================
 * EXPERIMENTAL DEPENDENCY
 * Requires the out-of-tree dmabuf-import DKMS module, which installs the UAPI
 * as <linux/dmabuf_import.h> and serves /dev/dmabuf_import. Without the header
 * these compile to stubs returning -ENOTSUP, so uPCIe still builds;
 * UPCIE_HAVE_DMABUF_IMPORT tells you which case you got. The module must also
 * be loaded at runtime.
 * ==========================================================================
 *
 * @file dmabuf_import.h
 * @version 0.9.0
 */
#ifndef UPCIE_EXPERIMENTAL_DMABUF_IMPORT_H
#define UPCIE_EXPERIMENTAL_DMABUF_IMPORT_H

/* Include <upcie/dmabuf.h> first for struct dmabuf; the uPCIe headers have no
 * include guards, so it cannot be pulled in from here. <upcie/upcie.h> orders
 * them correctly. */

/* Optional: pull in the dmabuf_import UAPI if the DKMS package installed it.
 * Guarded with __has_include so upcie builds without it. */
#if defined(__has_include)
#  if __has_include(<linux/dmabuf_import.h>)
#    include <linux/dmabuf_import.h>
#  endif
#endif

#ifdef DMABUF_IMPORT_ATTACH
/* The module's UAPI is available, so the real import path is compiled in. */
#define UPCIE_HAVE_DMABUF_IMPORT 1
/**
 * Attach to dma-buf with given FD
 *
 * Populates the given dma-buf structure with information about the dma-buf.
 */
static inline int
dmabuf_import_attach(int dmabuf_fd, struct dmabuf *dmabuf)
{
	struct dmabuf_import_attach attach;
	struct dmabuf_import_get_map *map = NULL;
	int import_fd, err;
	size_t map_size, pages_size;

	import_fd = open(DMABUF_IMPORT_DEVPATH, O_RDWR);
	if (import_fd < 0) {
		err = -errno;
		UPCIE_DEBUG("FAILED: open(%s), errno: %d", DMABUF_IMPORT_DEVPATH, err);
		return err;
	}

	memset(&attach, 0, sizeof(attach));
	attach.fd = dmabuf_fd;

	err = ioctl(import_fd, DMABUF_IMPORT_ATTACH, &attach);
	if (err) {
		err = -errno;
		UPCIE_DEBUG("FAILED: ioctl(DMABUF_IMPORT_ATTACH), errno: %d", err);
		goto exit;
	}

	map_size = attach.count * sizeof(struct dmabuf_import_dma_map);
	map = malloc(sizeof(struct dmabuf_import_get_map) + map_size);
	if (!map) {
		err = -errno;
		UPCIE_DEBUG("FAILED: malloc(map), errno: %d", err);
		ioctl(import_fd, DMABUF_IMPORT_DETACH, &dmabuf_fd);
		goto exit;
	}

	memset(map, 0, sizeof(*map));
	map->fd = dmabuf_fd;
	map->count = attach.count;

	err = ioctl(import_fd, DMABUF_IMPORT_GET_MAP, map);
	if (err) {
		err = -errno;
		UPCIE_DEBUG("FAILED: ioctl(DMABUF_IMPORT_GET_MAP), errno: %d\n", err);
		ioctl(import_fd, DMABUF_IMPORT_DETACH, &dmabuf_fd);
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
		ioctl(import_fd, DMABUF_IMPORT_DETACH, &dmabuf_fd);
		goto exit;
	}

	memcpy(dmabuf->pages, map->dma_arr, pages_size);

exit:
	free(map);
	close(import_fd);
	return err;
}

/**
 * Detach from given dma-buf
 *
 * NOTE: This doesn't free the underlying memory
 */
static inline int
dmabuf_import_detach(struct dmabuf *dmabuf)
{
	int import_fd, err;

	if (!dmabuf || !dmabuf->pages) {
		return 0;
	}

	import_fd = open(DMABUF_IMPORT_DEVPATH, O_RDWR);
	if (import_fd < 0) {
		err = -errno;
		UPCIE_DEBUG("FAILED: open(%s), errno: %d", DMABUF_IMPORT_DEVPATH, err);
		return err;
	}

	err = ioctl(import_fd, DMABUF_IMPORT_DETACH, &dmabuf->fd);
	if (err) {
		err = -errno;
		UPCIE_DEBUG("FAILED: ioctl(DMABUF_IMPORT_DETACH), errno: %d\n", err);
		// fall-through
	}

	free(dmabuf->pages);
	dmabuf->pages = NULL;
	dmabuf->npages = 0;

	close(dmabuf->fd);
	dmabuf->fd = -1;

	close(import_fd);
	return err;
}
#else /* !DMABUF_IMPORT_ATTACH: the module UAPI is unavailable, stub it out */
/* The module's UAPI was not found, so the calls below fail with -ENOTSUP. */
#define UPCIE_HAVE_DMABUF_IMPORT 0

/**
 * Attach stub: the dmabuf_import UAPI (<linux/dmabuf_import.h>) is not
 * available. Install the dmabuf-import DKMS package to enable importing.
 */
static inline int
dmabuf_import_attach(int UPCIE_UNUSED(dmabuf_fd), struct dmabuf *UPCIE_UNUSED(dmabuf))
{
	UPCIE_DEBUG("FAILED: dmabuf_import unavailable; install dmabuf-import-dkms");
	return -ENOTSUP;
}

static inline int
dmabuf_import_detach(struct dmabuf *UPCIE_UNUSED(dmabuf))
{
	UPCIE_DEBUG("FAILED: dmabuf_import unavailable; install dmabuf-import-dkms");
	return -ENOTSUP;
}
#endif /* DMABUF_IMPORT_ATTACH */

#endif /* UPCIE_EXPERIMENTAL_DMABUF_IMPORT_H */
