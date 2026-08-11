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
 * device to DMA against. A dma-buf fd (see dmabuf.h) can be mapped with
 * iommufd_ioas_map_file to let the device DMA straight into that memory through
 * the IOMMU. As of kernel 6.19 IOMMU_IOAS_MAP_FILE only accepts dma-bufs
 * exported by vfio-pci; dma-bufs exported from CUDA or HIP device memory are
 * rejected. Hopefully a future kernel lets those be mapped here too, so a device
 * can DMA straight into GPU VRAM through the IOMMU.
 *
 * Kernel docs: https://docs.kernel.org/userspace-api/iommufd.html
 *
 * @file iommufd.h
 * @version 0.6.0
 */
#include <linux/vfio.h>
#if defined(__has_include) && __has_include(<linux/iommufd.h>) && defined(VFIO_DEVICE_BIND_IOMMUFD)
#include <linux/iommufd.h>
#define UPCIE_HAVE_IOMMUFD 1
#endif

#ifndef UPCIE_HAVE_IOMMUFD
/* Fallback definitions so callers using these flags compile against
 * distros that ship pre-iommufd kernel headers (Linux < 6.6). The
 * iommufd_* ioctl wrappers below stub to -ENOTSUP in the same case, so
 * runtime callers can detect and fall back. Values match the mainline
 * uapi. */
#define IOMMU_IOAS_MAP_FIXED_IOVA (1U << 0)
#define IOMMU_IOAS_MAP_WRITEABLE  (1U << 1)
#define IOMMU_IOAS_MAP_READABLE   (1U << 2)
#endif

/**
 * An iommufd handle and the IOAS allocated on it.
 *
 * upcie models one IOAS per handle: iommufd_ioas_alloc fills `ioas_id`, and the
 * map/unmap/destroy helpers operate on that IOAS. Devices attached to the handle
 * share this single I/O address space, which is exactly what NVMe-to-GPU P2P
 * wants. For an isolated address space (e.g. a separate device group) open
 * another handle rather than juggling multiple IOAS on one.
 */
struct iommufd {
	int fd;           ///< Handle to /dev/iommu
	uint32_t ioas_id; ///< IOAS allocated on this handle (see iommufd_ioas_alloc)
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
 * On success the allocated IOAS id is stored in ctx->ioas_id and used by the
 * map/unmap helpers on this handle.
 *
 * @param ctx Pointer to an iommufd handle (see iommufd_open)
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
#ifdef UPCIE_HAVE_IOMMUFD
static inline int
iommufd_ioas_alloc(struct iommufd *ctx)
{
	struct iommu_ioas_alloc args = {0};

	args.size = sizeof(args);

	if (ioctl(ctx->fd, IOMMU_IOAS_ALLOC, &args) == -1) {
		return -errno;
	}

	ctx->ioas_id = args.out_ioas_id;

	return 0;
}
#else
// linux/iommufd.h landed in Linux 6.6; older distro kernel-headers packages
// don't ship it. Provide -ENOTSUP stubs so the tree compiles; callers that
// need iommufd should probe iommufd_open() at runtime and fall back.
static inline int
iommufd_ioas_alloc(struct iommufd *ctx)
{
	(void)ctx;
	return -ENOTSUP;
}
#endif

/**
 * Destroy an iommufd object by id (wraps IOMMU_DESTROY)
 *
 * This is the generic object-destroy ioctl, not IOAS-specific: the kernel has
 * no dedicated IOMMU_IOAS_DESTROY. Pass ctx->ioas_id to tear down the handle's
 * IOAS, i.e. as the counterpart to iommufd_ioas_alloc.
 *
 * @param ctx Pointer to an iommufd handle (see iommufd_open)
 * @param id Identifier of the object to destroy, e.g. an IOAS id
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
 * Map a host virtual address range into the handle's IOAS
 *
 * @param ctx Pointer to an iommufd handle with an allocated IOAS (see
 * iommufd_ioas_alloc)
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
iommufd_ioas_map(struct iommufd *ctx, uint64_t user_va, uint64_t length, uint32_t flags,
		 uint64_t *iova)
{
	struct iommu_ioas_map args = {0};

	args.size = sizeof(args);
	args.flags = flags;
	args.ioas_id = ctx->ioas_id;
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
iommufd_ioas_map(struct iommufd *ctx, uint64_t user_va, uint64_t length, uint32_t flags,
		 uint64_t *iova)
{
	(void)ctx;
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
 * needed. As of kernel 6.19 IOMMU_IOAS_MAP_FILE only accepts dma-bufs exported
 * by vfio-pci; dma-bufs exported from CUDA or HIP device memory are rejected.
 * Hopefully a future kernel lets those be mapped here too, so a device can DMA
 * straight into GPU VRAM through the IOMMU.
 *
 * @param ctx Pointer to an iommufd handle with an allocated IOAS (see
 * iommufd_ioas_alloc)
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
iommufd_ioas_map_file(struct iommufd *ctx, int fd, uint64_t offset, uint64_t length,
		      uint32_t flags, uint64_t *iova)
{
	struct iommu_ioas_map_file args = {0};

	args.size = sizeof(args);
	args.flags = flags;
	args.ioas_id = ctx->ioas_id;
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
iommufd_ioas_map_file(struct iommufd *ctx, int fd, uint64_t offset, uint64_t length,
		      uint32_t flags, uint64_t *iova)
{
	(void)ctx;
	(void)fd;
	(void)offset;
	(void)length;
	(void)flags;
	(void)iova;

	return -ENOTSUP;
}
#endif

/**
 * Unmap a range from the handle's IOAS
 *
 * @param ctx Pointer to an iommufd handle with an allocated IOAS (see
 * iommufd_ioas_alloc)
 * @param iova IOVA of the range to unmap, as returned by iommufd_ioas_map
 * @param length Length of the range in bytes
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
#ifdef UPCIE_HAVE_IOMMUFD
static inline int
iommufd_ioas_unmap(struct iommufd *ctx, uint64_t iova, uint64_t length)
{
	struct iommu_ioas_unmap args = {0};

	args.size = sizeof(args);
	args.ioas_id = ctx->ioas_id;
	args.iova = iova;
	args.length = length;

	if (ioctl(ctx->fd, IOMMU_IOAS_UNMAP, &args) == -1) {
		return -errno;
	}

	return 0;
}
#else
static inline int
iommufd_ioas_unmap(struct iommufd *ctx, uint64_t iova, uint64_t length)
{
	(void)ctx;
	(void)iova;
	(void)length;
	return -ENOTSUP;
}
#endif

/**
 * Bind a vfio device cdev to this iommufd handle
 *
 * @param ctx Pointer to an iommufd handle (see iommufd_open)
 * @param cdev Pointer to an open cdev (see vfio_cdev_open). On success
 * cdev.devid holds the device id assigned by the kernel.
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
#ifdef UPCIE_HAVE_IOMMUFD
static inline int
iommufd_bind(struct iommufd *ctx, struct vfio_cdev *cdev)
{
	struct vfio_device_bind_iommufd args = {0};

	args.argsz = sizeof(args);
	args.iommufd = ctx->fd;

	if (ioctl(cdev->fd, VFIO_DEVICE_BIND_IOMMUFD, &args) == -1) {
		return -errno;
	}

	cdev->devid = args.out_devid;

	return 0;
}
#else
static inline int
iommufd_bind(struct iommufd *ctx, struct vfio_cdev *cdev)
{
	(void)ctx;
	(void)cdev;
	return -ENOTSUP;
}
#endif

/**
 * Attach a bound vfio device cdev to this handle's IOAS
 *
 * @param ctx Pointer to an iommufd handle with an allocated IOAS (see
 * iommufd_ioas_alloc)
 * @param cdev Pointer to a bound cdev (see iommufd_bind)
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
#ifdef UPCIE_HAVE_IOMMUFD
static inline int
iommufd_attach(struct iommufd *ctx, struct vfio_cdev *cdev)
{
	struct vfio_device_attach_iommufd_pt args = {0};

	args.argsz = sizeof(args);
	args.pt_id = ctx->ioas_id;

	if (ioctl(cdev->fd, VFIO_DEVICE_ATTACH_IOMMUFD_PT, &args) == -1) {
		return -errno;
	}

	return 0;
}
#else
static inline int
iommufd_attach(struct iommufd *ctx, struct vfio_cdev *cdev)
{
	(void)ctx;
	(void)cdev;
	return -ENOTSUP;
}
#endif

/**
 * Detach a vfio device cdev from its IOAS
 *
 * @param cdev Pointer to an attached cdev (see iommufd_attach)
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
#ifdef UPCIE_HAVE_IOMMUFD
static inline int
iommufd_detach(struct vfio_cdev *cdev)
{
	struct vfio_device_detach_iommufd_pt args = {0};

	args.argsz = sizeof(args);

	if (ioctl(cdev->fd, VFIO_DEVICE_DETACH_IOMMUFD_PT, &args) == -1) {
		return -errno;
	}

	return 0;
}
#else
static inline int
iommufd_detach(struct vfio_cdev *cdev)
{
	(void)cdev;
	return -ENOTSUP;
}
#endif
