// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_LINUX_ENABLED
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <libxnvme.h>
#include <xnvme_dev.h>
#include <xnvme_be_linux_dmabuf_iommu.h>
#ifdef XNVME_BE_LINUX_DMABUF_IOMMU_ENABLED
#include <fcntl.h>
#include <linux/dmabuf_import.h>
#include <linux/iommu_map_pa.h>
#endif

/** Room for "0000:00:00.0" */
#define XNVME_BE_LINUX_BDF_LEN 16

/**
 * Resolve the PCI address of the controller behind the given device
 *
 * The form the mapping ioctl names a target by.
 *
 * @return On success, 0 is returned. On error, negative errno
 */
static int
_bdf(const struct xnvme_dev *dev, char *bdf, size_t nbytes)
{
	char path[PATH_MAX], resolved[PATH_MAX], uri[PATH_MAX];
	unsigned int dom, bus, slot, func;
	char *name;

	snprintf(uri, sizeof(uri), "%s", dev->ident.uri);
	name = basename(uri);

	snprintf(path, sizeof(path), "/sys/class/block/%s/device", name);
	if (!realpath(path, resolved)) {
		XNVME_DEBUG("FAILED: realpath(%s), errno: %d", path, errno);
		return -errno;
	}

	for (char *at = resolved; strlen(at) > 1;) {
		char *base;

		snprintf(path, sizeof(path), "%s", at);
		base = basename(path);

		if (sscanf(base, "%x:%x:%x.%x", &dom, &bus, &slot, &func) == 4) {
			snprintf(bdf, nbytes, "%s", base);
			return 0;
		}

		base = strrchr(at, '/');
		if (!base) {
			break;
		}
		*base = '\0';
	}

	XNVME_DEBUG("FAILED: no PCI address above '%s'", dev->ident.uri);

	return -ENODEV;
}

int
xnvme_be_linux_dmabuf_iommu_translates(const struct xnvme_dev *dev)
{
	char bdf[XNVME_BE_LINUX_BDF_LEN], path[PATH_MAX], type[32] = {0};
	FILE *fh;
	int err;

	err = _bdf(dev, bdf, sizeof(bdf));
	if (err) {
		return err;
	}

	snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/iommu_group/type", bdf);

	fh = fopen(path, "r");
	if (!fh) {
		return 0; // No group, so nothing translates
	}
	if (!fgets(type, sizeof(type), fh)) {
		fclose(fh);
		XNVME_DEBUG("FAILED: fgets(%s)", path);
		return -EIO;
	}
	fclose(fh);

	// 'DMA' and 'DMA-FQ' translate; 'identity' and 'unmanaged' do not
	return strncmp(type, "DMA", 3) ? 0 : 1;
}

#ifdef XNVME_BE_LINUX_DMABUF_IOMMU_ENABLED
/**
 * Retrieve the descriptor the mappings of this process live on
 *
 * A mapping lasts until it is unmapped or the descriptor it was made on is
 * closed, so this one is never closed.
 *
 * @return On success, the descriptor. On error, negative errno
 */
static int
_iommu_fd(void)
{
	static int fd = -1;

	if (fd < 0) {
		fd = open(IOMMU_MAP_PA_DEVPATH, O_RDWR);
		if (fd < 0) {
			XNVME_DEBUG("FAILED: open(%s), errno: %d", IOMMU_MAP_PA_DEVPATH, errno);
			// Absent means the module is not loaded, the same answer as
			// a build without it
			return (errno == ENOENT) ? -ENOSYS : -errno;
		}
	}

	return fd;
}

/**
 * Read the physical ranges the exported dma-buf is made of
 *
 * @return On success, the number of ranges. On error, negative errno
 */
static int
_phys(int dmabuf_fd, uint64_t **phys, uint32_t *page_size)
{
	struct dmabuf_import_attach att = {.fd = dmabuf_fd};
	struct dmabuf_import_get_map *map = NULL;
	uint64_t *addrs = NULL;
	int fd, err;

	fd = open(DMABUF_IMPORT_DEVPATH, O_RDWR);
	if (fd < 0) {
		XNVME_DEBUG("FAILED: open(%s), errno: %d", DMABUF_IMPORT_DEVPATH, errno);
		return (errno == ENOENT) ? -ENOSYS : -errno;
	}

	if (ioctl(fd, DMABUF_IMPORT_ATTACH, &att)) {
		XNVME_DEBUG("FAILED: DMABUF_IMPORT_ATTACH, errno: %d", errno);
		err = -errno;
		goto exit;
	}

	map = calloc(1, sizeof(*map) + (size_t)att.count * sizeof(map->dma_arr[0]));
	addrs = calloc(att.count, sizeof(*addrs));
	if ((!map) || (!addrs)) {
		err = -ENOMEM;
		goto detach;
	}

	map->fd = dmabuf_fd;
	map->count = att.count;
	if (ioctl(fd, DMABUF_IMPORT_GET_MAP, map)) {
		XNVME_DEBUG("FAILED: DMABUF_IMPORT_GET_MAP, errno: %d", errno);
		err = -errno;
		goto detach;
	}

	// The IOVA is the physical address, so a gap cannot be closed by placing
	// the mapping elsewhere
	for (uint32_t i = 0; i < map->count; ++i) {
		addrs[i] = map->dma_arr[i].dma_addr;
		if (i && (addrs[i] != addrs[i - 1] + map->dma_arr[i - 1].dma_len)) {
			XNVME_DEBUG("FAILED: dma-buf is not one physical run");
			err = -ENOTSUP;
			goto detach;
		}
	}

	*page_size = (uint32_t)map->dma_arr[0].dma_len;
	*phys = addrs;
	addrs = NULL;
	err = (int)map->count;

detach:
	ioctl(fd, DMABUF_IMPORT_DETACH, &dmabuf_fd);
exit:
	free(addrs);
	free(map);
	close(fd);

	return err;
}

int
xnvme_be_linux_dmabuf_iommu_map(const struct xnvme_dev *dev, int dmabuf_fd, uint64_t *handle)
{
	struct iommu_map_pa_req req = {0};
	char bdf[XNVME_BE_LINUX_BDF_LEN];
	uint64_t *phys = NULL;
	uint32_t page_size = 0;
	int fd, nphys, err;

	err = _bdf(dev, bdf, sizeof(bdf));
	if (err) {
		return err;
	}

	nphys = _phys(dmabuf_fd, &phys, &page_size);
	if (nphys < 0) {
		return nphys;
	}

	fd = _iommu_fd();
	if (fd < 0) {
		free(phys);
		return fd;
	}

	snprintf(req.bdf, sizeof(req.bdf), "%s", bdf);
	req.dmabuf_fd = dmabuf_fd;
	req.page_size = page_size;
	req.nphys = (uint32_t)nphys;
	req.prot = IOMMU_MAP_PA_PROT_READ | IOMMU_MAP_PA_PROT_WRITE;
	req.iova_base = phys[0];
	req.user_phys_ptr = (uint64_t)(uintptr_t)phys;

	if (ioctl(fd, IOMMU_MAP_PA, &req)) {
		XNVME_DEBUG("FAILED: IOMMU_MAP_PA(%s), errno: %d", bdf, errno);
		err = -errno;
		free(phys);
		return err;
	}

	XNVME_DEBUG("INFO: mapped %u x %u at iova 0x%llx for %s", req.nphys, req.page_size,
		    (unsigned long long)req.iova_base, bdf);

	*handle = req.map_handle;

	free(phys);

	return 0;
}

void
xnvme_be_linux_dmabuf_iommu_unmap(uint64_t handle)
{
	struct iommu_unmap_pa_req req = {.map_handle = handle};
	int fd;

	fd = _iommu_fd();
	if (fd < 0) {
		return;
	}

	if (ioctl(fd, IOMMU_UNMAP_PA, &req)) {
		XNVME_DEBUG("FAILED: IOMMU_UNMAP_PA, errno: %d", errno);
	}
}
#else
int
xnvme_be_linux_dmabuf_iommu_map(const struct xnvme_dev *XNVME_UNUSED(dev),
				int XNVME_UNUSED(dmabuf_fd), uint64_t *XNVME_UNUSED(handle))
{
	XNVME_DEBUG("FAILED: built without the dma-buf IOMMU modules");
	return -ENOSYS;
}

void
xnvme_be_linux_dmabuf_iommu_unmap(uint64_t XNVME_UNUSED(handle))
{
}
#endif
#endif
