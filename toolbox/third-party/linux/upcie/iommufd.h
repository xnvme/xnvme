// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * iommufd helper for user-space
 * =============================
 *
 * iommufd is the modern Linux interface for managing I/O address spaces. It
 * replaces the IOMMU half of the legacy vfio container (see vfioctl.h), exposing
 * map and unmap of DMA address spaces through a single file-descriptor based API
 * on /dev/iommu.
 *
 * Most functions here are thin wrappers of the ioctls of the same name. The
 * essential data structures and their relation:
 *
 *  /dev/iommu (iommufd)
 *        |
 *     IOAS, I/O address space         <- IOMMU_IOAS_ALLOC
 *        |   map: user_va or fd -> IOVA  (IOMMU_IOAS_MAP / IOMMU_IOAS_MAP_FILE)
 *        |
 *   attached device                   <- VFIO_DEVICE_ATTACH_IOMMUFD_PT
 *        |
 *   vfio device cdev (/dev/vfio/devices/vfioN)
 *
 * A device is taken into the iommufd model by opening its vfio cdev, binding it
 * to an iommufd handle, allocating an IOAS, and attaching the device to that
 * IOAS. From there, ranges mapped into the IOAS become valid IOVAs for the
 * device to DMA against. A dma-buf fd (see dmabuf.h), e.g. exported from CUDA
 * device memory, can be mapped with iommufd_ioas_map_file to let the device DMA
 * straight into that memory through the IOMMU.
 *
 * Kernel docs: https://docs.kernel.org/userspace-api/iommufd.html
 *
 * @file iommufd.h
 * @version 0.5.1
 */
#include <linux/vfio.h>
#if defined(__has_include) && __has_include(<linux/iommufd.h>) && defined(VFIO_DEVICE_BIND_IOMMUFD)
#include <linux/iommufd.h>
#define UPCIE_HAVE_IOMMUFD 1
#endif

#ifndef UPCIE_HAVE_IOMMUFD
/* Fallback definitions so callers using these flags in dmamem_from_memfd,
 * dmamem_from_dmabuf, and dmamem_from_hostmem_iommufd compile against
 * distros that ship pre-iommufd kernel headers (Linux < 6.6). The
 * iommufd_* ioctl wrappers below stub to -ENOTSUP in the same case, so
 * runtime callers can detect and fall back. Values match the mainline
 * uapi. */
#define IOMMU_IOAS_MAP_FIXED_IOVA (1U << 0)
#define IOMMU_IOAS_MAP_WRITEABLE  (1U << 1)
#define IOMMU_IOAS_MAP_READABLE   (1U << 2)
#endif

struct iommufd {
	int fd; ///< Handle to /dev/iommu
};

struct iommufd_device {
	int fd;         ///< vfio device cdev file descriptor
	uint32_t devid; ///< Device id assigned by VFIO_DEVICE_BIND_IOMMUFD
};

/**
 * Open an iommufd handle
 *
 * @param ctx Assigns the file-descriptor to ctx.fd
 *
 * @return On success, 0 is returned and ctx.fd is setup as a handle to
 * /dev/iommu. On error, negative errno is returned to indicate the error.
 */
static inline int
iommufd_open(struct iommufd *ctx)
{
	ctx->fd = open("/dev/iommu", O_RDWR);
	if (ctx->fd == -1) {
		return -errno;
	}

	return 0;
}

/**
 * Close an iommufd handle
 *
 * Closing the handle tears down every IOAS and mapping owned by it.
 *
 * @param ctx Pointer to an iommufd handle (see iommufd_open)
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
static inline int
iommufd_close(struct iommufd *ctx)
{
	if (close(ctx->fd) == -1) {
		return -errno;
	}

	return 0;
}

/**
 * Allocate an I/O address space (IOAS)
 *
 * @param ctx Pointer to an iommufd handle (see iommufd_open)
 * @param ioas_id Assigns the id of the allocated IOAS
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
#ifdef UPCIE_HAVE_IOMMUFD
static inline int
iommufd_ioas_alloc(struct iommufd *ctx, uint32_t *ioas_id)
{
	struct iommu_ioas_alloc args = {0};

	args.size = sizeof(args);

	if (ioctl(ctx->fd, IOMMU_IOAS_ALLOC, &args) == -1) {
		return -errno;
	}

	*ioas_id = args.out_ioas_id;

	return 0;
}
#else
// linux/iommufd.h landed in Linux 6.6; older distro kernel-headers packages
// don't ship it. Provide -ENOTSUP stubs so the tree compiles; callers that
// need iommufd should probe iommufd_open() at runtime and fall back.
static inline int
iommufd_ioas_alloc(struct iommufd *ctx, uint32_t *ioas_id)
{
	(void)ctx;
	(void)ioas_id;
	return -ENOTSUP;
}
#endif

/**
 * Destroy an iommufd object, e.g. an IOAS, by id
 *
 * @param ctx Pointer to an iommufd handle (see iommufd_open)
 * @param id Identifier of the object to destroy
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
#ifdef UPCIE_HAVE_IOMMUFD
static inline int
iommufd_destroy(struct iommufd *ctx, uint32_t id)
{
	struct iommu_destroy args = {0};

	args.size = sizeof(args);
	args.id = id;

	if (ioctl(ctx->fd, IOMMU_DESTROY, &args) == -1) {
		return -errno;
	}

	return 0;
}
#else
static inline int
iommufd_destroy(struct iommufd *ctx, uint32_t id)
{
	(void)ctx;
	(void)id;
	return -ENOTSUP;
}
#endif

/**
 * Map a host virtual address range into an IOAS
 *
 * @param ctx Pointer to an iommufd handle (see iommufd_open)
 * @param ioas_id Identifier of the IOAS to map into
 * @param user_va Host virtual address of the range to map
 * @param length Length of the range in bytes
 * @param flags Bitmask of IOMMU_IOAS_MAP_* flags
 * @param iova In/out. When IOMMU_IOAS_MAP_FIXED_IOVA is set in flags, the
 * caller provides the desired IOVA. On return it holds the IOVA the mapping was
 * placed at.
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
#ifdef UPCIE_HAVE_IOMMUFD
static inline int
iommufd_ioas_map(struct iommufd *ctx, uint32_t ioas_id, uint64_t user_va, uint64_t length,
		 uint32_t flags, uint64_t *iova)
{
	struct iommu_ioas_map args = {0};

	args.size = sizeof(args);
	args.flags = flags;
	args.ioas_id = ioas_id;
	args.user_va = user_va;
	args.length = length;
	args.iova = *iova; // Honoured only when IOMMU_IOAS_MAP_FIXED_IOVA is set in flags

	if (ioctl(ctx->fd, IOMMU_IOAS_MAP, &args) == -1) {
		return -errno;
	}

	*iova = args.iova;

	return 0;
}
#else
static inline int
iommufd_ioas_map(struct iommufd *ctx, uint32_t ioas_id, uint64_t user_va, uint64_t length,
		 uint32_t flags, uint64_t *iova)
{
	(void)ctx;
	(void)ioas_id;
	(void)user_va;
	(void)length;
	(void)flags;
	(void)iova;
	return -ENOTSUP;
}
#endif

/**
 * Map a range described by a file-descriptor into an IOAS
 *
 * Intended for memfd and dma-buf backed memory, where no host virtual mapping is
 * needed. A dma-buf fd exported from device memory, e.g. CUDA, is mapped this
 * way to expose it as an IOVA for device DMA.
 *
 * @param ctx Pointer to an iommufd handle (see iommufd_open)
 * @param ioas_id Identifier of the IOAS to map into
 * @param fd File-descriptor backing the range, e.g. a dma-buf fd
 * @param offset Offset into the file of the range to map
 * @param length Length of the range in bytes
 * @param flags Bitmask of IOMMU_IOAS_MAP_* flags
 * @param iova In/out. When IOMMU_IOAS_MAP_FIXED_IOVA is set in flags, the
 * caller provides the desired IOVA. On return it holds the IOVA the mapping was
 * placed at.
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
#ifdef IOMMU_IOAS_MAP_FILE
static inline int
iommufd_ioas_map_file(struct iommufd *ctx, uint32_t ioas_id, int fd, uint64_t offset,
		      uint64_t length, uint32_t flags, uint64_t *iova)
{
	struct iommu_ioas_map_file args = {0};

	args.size = sizeof(args);
	args.flags = flags;
	args.ioas_id = ioas_id;
	args.fd = fd;
	args.start = offset;
	args.length = length;
	args.iova = *iova; // Honoured only when IOMMU_IOAS_MAP_FIXED_IOVA is set in flags

	if (ioctl(ctx->fd, IOMMU_IOAS_MAP_FILE, &args) == -1) {
		return -errno;
	}

	*iova = args.iova;

	return 0;
}
#else
// IOMMU_IOAS_MAP_FILE was added in Linux 6.14; on older UAPI headers this maps
// to a stub so callers can fall back to iommufd_ioas_map() with a user_va.
static inline int
iommufd_ioas_map_file(struct iommufd *ctx, uint32_t ioas_id, int fd, uint64_t offset,
		      uint64_t length, uint32_t flags, uint64_t *iova)
{
	(void)ctx;
	(void)ioas_id;
	(void)fd;
	(void)offset;
	(void)length;
	(void)flags;
	(void)iova;

	return -ENOTSUP;
}
#endif

/**
 * Unmap a range from an IOAS
 *
 * @param ctx Pointer to an iommufd handle (see iommufd_open)
 * @param ioas_id Identifier of the IOAS to unmap from
 * @param iova IOVA of the range to unmap, as returned by iommufd_ioas_map
 * @param length Length of the range in bytes
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
#ifdef UPCIE_HAVE_IOMMUFD
static inline int
iommufd_ioas_unmap(struct iommufd *ctx, uint32_t ioas_id, uint64_t iova, uint64_t length)
{
	struct iommu_ioas_unmap args = {0};

	args.size = sizeof(args);
	args.ioas_id = ioas_id;
	args.iova = iova;
	args.length = length;

	if (ioctl(ctx->fd, IOMMU_IOAS_UNMAP, &args) == -1) {
		return -errno;
	}

	return 0;
}
#else
static inline int
iommufd_ioas_unmap(struct iommufd *ctx, uint32_t ioas_id, uint64_t iova, uint64_t length)
{
	(void)ctx;
	(void)ioas_id;
	(void)iova;
	(void)length;
	return -ENOTSUP;
}
#endif

/**
 * Open a vfio device cdev
 *
 * @param path Path to the device cdev, e.g. /dev/vfio/devices/vfio0
 * @param dev Assigns the file-descriptor to dev.fd
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
static inline int
iommufd_device_open(const char *path, struct iommufd_device *dev)
{
	dev->fd = open(path, O_RDWR);
	if (dev->fd == -1) {
		return -errno;
	}

	dev->devid = 0;

	return 0;
}

/**
 * Close a vfio device cdev
 *
 * Closing the device detaches it from any IOAS and unbinds it from iommufd.
 *
 * @param dev Pointer to a device handle (see iommufd_device_open)
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
static inline int
iommufd_device_close(struct iommufd_device *dev)
{
	if (close(dev->fd) == -1) {
		return -errno;
	}

	return 0;
}

/**
 * Bind a vfio device to an iommufd handle
 *
 * @param dev Pointer to a device handle (see iommufd_device_open). On success
 * dev.devid holds the device id assigned by the kernel.
 * @param ctx Pointer to an iommufd handle (see iommufd_open)
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
#ifdef UPCIE_HAVE_IOMMUFD
static inline int
iommufd_device_bind(struct iommufd_device *dev, struct iommufd *ctx)
{
	struct vfio_device_bind_iommufd args = {0};

	args.argsz = sizeof(args);
	args.iommufd = ctx->fd;

	if (ioctl(dev->fd, VFIO_DEVICE_BIND_IOMMUFD, &args) == -1) {
		return -errno;
	}

	dev->devid = args.out_devid;

	return 0;
}
#else
static inline int
iommufd_device_bind(struct iommufd_device *dev, struct iommufd *ctx)
{
	(void)dev;
	(void)ctx;
	return -ENOTSUP;
}
#endif

/**
 * Attach a bound vfio device to an IOAS
 *
 * @param dev Pointer to a bound device handle (see iommufd_device_bind)
 * @param pt_id Identifier of the IOAS to attach to (see iommufd_ioas_alloc)
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
#ifdef UPCIE_HAVE_IOMMUFD
static inline int
iommufd_device_attach(struct iommufd_device *dev, uint32_t pt_id)
{
	struct vfio_device_attach_iommufd_pt args = {0};

	args.argsz = sizeof(args);
	args.pt_id = pt_id;

	if (ioctl(dev->fd, VFIO_DEVICE_ATTACH_IOMMUFD_PT, &args) == -1) {
		return -errno;
	}

	return 0;
}
#else
static inline int
iommufd_device_attach(struct iommufd_device *dev, uint32_t pt_id)
{
	(void)dev;
	(void)pt_id;
	return -ENOTSUP;
}
#endif

/**
 * Detach a vfio device from its IOAS
 *
 * @param dev Pointer to an attached device handle (see iommufd_device_attach)
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
#ifdef UPCIE_HAVE_IOMMUFD
static inline int
iommufd_device_detach(struct iommufd_device *dev)
{
	struct vfio_device_detach_iommufd_pt args = {0};

	args.argsz = sizeof(args);

	if (ioctl(dev->fd, VFIO_DEVICE_DETACH_IOMMUFD_PT, &args) == -1) {
		return -errno;
	}

	return 0;
}
#else
static inline int
iommufd_device_detach(struct iommufd_device *dev)
{
	(void)dev;
	return -ENOTSUP;
}
#endif
