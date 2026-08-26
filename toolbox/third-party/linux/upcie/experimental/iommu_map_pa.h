// SPDX-License-Identifier: BSD-3-Clause

/**
 * Experimental uPCIe iommu-map helper interface
 * ==============================================
 *
 * Userspace wrappers for the helper kernel module that maps an array of
 * device-physical addresses (e.g. a CUDA/udmabuf-derived phys_lut) into the
 * IOMMU domain a VFIO-controlled NVMe already uses, returning an IOVA base that
 * userspace writes into NVMe PRPs.
 *
 * Mappings persist until userspace issues an explicit UNMAP or closes the file
 * descriptor.
 *
 * ==========================================================================
 * EXPERIMENTAL DEPENDENCY
 * Requires the out-of-tree iommu-map-pa DKMS module, which installs the UAPI
 * as <linux/iommu_map_pa.h> and serves /dev/iommu_map_pa. Without the header
 * these compile to stubs returning -ENOTSUP, so uPCIe still builds;
 * UPCIE_HAVE_IOMMU_MAP_PA tells you which case you got. The module must also
 * be loaded at runtime.
 * ==========================================================================
 *
 * @file iommu_map_pa.h
 * @version 0.8.0
 */
#ifndef UPCIE_EXPERIMENTAL_IOMMU_MAP_PA_H
#define UPCIE_EXPERIMENTAL_IOMMU_MAP_PA_H

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* Optional: pull in the iommu_map_pa UAPI if the DKMS package installed it.
 * Guarded with __has_include so upcie builds without it. */
#if defined(__has_include)
#  if __has_include(<linux/iommu_map_pa.h>)
#    include <linux/iommu_map_pa.h>
#  endif
#endif

#ifdef IOMMU_MAP_PA
/* The module's UAPI is available, so the real ioctl path is compiled in. */
#define UPCIE_HAVE_IOMMU_MAP_PA 1

static inline int
iommu_map_pa_open(void)
{
	int fd = open(IOMMU_MAP_PA_DEVPATH, O_RDWR);

	return fd < 0 ? -errno : fd;
}

static inline int
iommu_map_pa_close(int fd)
{
	if (fd < 0)
		return -EINVAL;

	return close(fd) ? -errno : 0;
}

/* Unmap a handle returned by iommu_map_pa_add(). */
static inline int
iommu_map_pa_del(int fd, uint64_t map_handle)
{
	struct iommu_unmap_pa_req req = {0};

	if (fd < 0 || !map_handle)
		return -EINVAL;

	req.map_handle = map_handle;
	return ioctl(fd, IOMMU_UNMAP_PA, &req) < 0 ? -errno : 0;
}

/*
 * Map an array of device-physical addresses (phys_lut) into the
 * IOMMU domain the target NVMe device currently uses. On success the IOMMU
 * translates 'iova_base + i * page_size' to 'phys[i]', so PRPs should be built
 * from iova_base, not from phys[].
 */
static inline int
iommu_map_pa_add(int fd, const char *bdf, int dmabuf_fd, uint64_t iova_base,
		       uint32_t page_size, uint32_t nphys, const uint64_t *phys, uint32_t prot,
		       uint64_t *map_handle_out)
{
	struct iommu_map_pa_req req = {0};

	if (fd < 0 || !bdf || !phys || !nphys)
		return -EINVAL;

	strncpy(req.bdf, bdf, sizeof(req.bdf) - 1);
	req.dmabuf_fd = dmabuf_fd;
	req.page_size = page_size;
	req.nphys = nphys;
	req.prot = prot;
	req.iova_base = iova_base;
	req.user_phys_ptr = (uint64_t)(uintptr_t)phys;

	if (ioctl(fd, IOMMU_MAP_PA, &req) < 0)
		return -errno;

	if (map_handle_out)
		*map_handle_out = req.map_handle;
	return 0;
}

#else /* !IOMMU_MAP_PA: iommu-map-pa UAPI unavailable, provide stubs */
#define UPCIE_HAVE_IOMMU_MAP_PA 0

/* The stubs take the same arguments as the real thing, so the constants those
 * arguments are built from have to exist here too; otherwise a caller cannot
 * compile without the package, which is what the stubs are for. Values mirror
 * the UAPI and are ABI, so they do not drift. */
#define IOMMU_MAP_PA_DEVPATH "/dev/iommu_map_pa"
#define IOMMU_MAP_PA_PROT_READ (1U << 0)
#define IOMMU_MAP_PA_PROT_WRITE (1U << 1)

static inline int
iommu_map_pa_open(void)
{
	return -ENOTSUP;
}

static inline int
iommu_map_pa_close(int fd)
{
	(void)fd;

	return -ENOTSUP;
}

static inline int
iommu_map_pa_del(int fd, uint64_t map_handle)
{
	(void)fd;
	(void)map_handle;

	return -ENOTSUP;
}

static inline int
iommu_map_pa_add(int fd, const char *bdf, int dmabuf_fd, uint64_t iova_base,
		       uint32_t page_size, uint32_t nphys, const uint64_t *phys, uint32_t prot,
		       uint64_t *map_handle_out)
{
	(void)fd;
	(void)bdf;
	(void)dmabuf_fd;
	(void)iova_base;
	(void)page_size;
	(void)nphys;
	(void)phys;
	(void)prot;
	(void)map_handle_out;

	return -ENOTSUP;
}
#endif /* IOMMU_MAP_PA */

#endif
