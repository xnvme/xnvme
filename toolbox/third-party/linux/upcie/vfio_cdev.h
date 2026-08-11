// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * vfio device cdev helper for user-space
 * ======================================
 *
 * The modern vfio device model exposes each device as its own character device
 * under /dev/vfio/devices/vfioN, opened directly without the legacy container
 * and group dance (see vfioctl.h for that). A cdev is opened here, then bound
 * and attached to an iommufd IOAS via the iommufd_bind/iommufd_attach helpers
 * in iommufd.h.
 *
 * struct vfio_cdev is a plain handle: the open cdev fd plus the device id the
 * kernel assigns at bind time. Region and mmap helpers that take a raw device
 * fd (vfio_device_get_region_info, vfio_map_region) live in vfioctl.h and work
 * on this fd too.
 *
 * @file vfio_cdev.h
 * @version 0.6.0
 */

struct vfio_cdev {
	int fd;         ///< vfio device cdev file descriptor
	uint32_t devid; ///< Device id assigned by iommufd_bind (VFIO_DEVICE_BIND_IOMMUFD)
};

/**
 * Open a vfio device cdev
 *
 * @param path Path to the device cdev, e.g. /dev/vfio/devices/vfio0
 * @param cdev Assigns the file-descriptor to cdev.fd
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
static inline int
vfio_cdev_open(const char *path, struct vfio_cdev *cdev)
{
	cdev->fd = open(path, O_RDWR);
	if (cdev->fd == -1) {
		return -errno;
	}

	cdev->devid = 0;

	return 0;
}

/**
 * Close a vfio device cdev
 *
 * Closing the device detaches it from any IOAS and unbinds it from iommufd.
 *
 * @param cdev Pointer to a cdev handle (see vfio_cdev_open)
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
static inline int
vfio_cdev_close(struct vfio_cdev *cdev)
{
	if (close(cdev->fd) == -1) {
		return -errno;
	}

	return 0;
}
