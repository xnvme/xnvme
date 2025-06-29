// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * VFIO helper for user-space
 * ==========================
 *
 * The intent is for this library to be vendored by who-ever needs it. Possibly also its companion
 * driver-script.
 *
 * The documentation for using vfio-pci is to my knowledge split between:
 *
 * - UAPI:
 * - Kernel docs: https://docs.kernel.org/driver-api/vfio.html
 *
 * Most of the functions here are simply wrappers of the IOCTLs of the same name. And a brief
 * introduction to the data structures:
 *
 *  VFIO Container (/dev/vfio/vfio)
 *              |
 *      ---------------------
 *      |                   |
 * IOMMU Group 5       IOMMU Group 6
 *  (/dev/vfio/5)       (/dev/vfio/6)
 *      |                   |
 *   ---------          -----------
 *   | Dev A |          | Dev C   |
 *   | Dev B |          |         |
 *   ---------          -----------
 *
 * The above is the essential modeling of the isolation-level of memory among devices.
 *
 * @file vfioctl.h
 * @version 0.3.2
 */
struct vfio_group {
	int fd;
	int id;
	struct vfio_group_status status;
};

struct vfio_container {
	int fd;
	struct vfio_iommu_type1_info info;
	struct vfio_iommu_type1_dma_map map;
};

struct vfio_device {
	int fd;
};

/**
 * Close a container
 *
 * @param container Pointer to a container handle (see vfio_container_open)
 * @return On success, 0 is returned. On error, negative errno is set to
 * indicate the error.
 */
static inline int
vfio_container_close(struct vfio_container *container)
{
	int err;

	err = close(container->fd);
	if (err == -1) {
		return -errno;
	}

	return 0;
}

/**
 * Retrieve a container handle
 *
 * @param container Assigns the file-descriptor to container.fd
 *
 * @return On success, 0 is returned and container.fd setup as a handle to a
 * vfio container. On error, negative errno is set to indicate the error.
 */
static inline int
vfio_container_open(struct vfio_container *container)
{

	container->fd = open("/dev/vfio/vfio", O_RDWR);
	if (container->fd == -1) {
		return -errno;
	}

	return 0;
}

static inline int
vfio_get_api_version(struct vfio_container *container, int *api_version)
{
	int err;

	err = ioctl(container->fd, VFIO_GET_API_VERSION);
	if (err < 0) {
		return -EIO;
	}

	*api_version = err;

	return 0;
}

static inline int
vfio_check_extension(struct vfio_container *container, int extension)
{
	return ioctl(container->fd, VFIO_CHECK_EXTENSION, extension);
}

static inline int
vfio_set_iommu(struct vfio_container *container, int iommu_type)
{
	return ioctl(container->fd, VFIO_SET_IOMMU, iommu_type);
}

static inline int
vfio_group_close(struct vfio_group *group)
{
	return close(group->fd);
}

/**
 * Open the group with the given 'id'
 *
 * @param id Numerical identifier of the given group
 */
static inline int
vfio_group_open(int id, struct vfio_group *group)
{
	char path[64] = {0};

	snprintf(path, sizeof(path), "/dev/vfio/%d", id);

	group->fd = open(path, O_RDWR);
	if (group->fd == -1) {
		return -EIO;
	}

	return 0;
}

static inline int
vfio_group_get_status(struct vfio_group *group, struct vfio_group_status *status)
{
	memset(&group->status, 0, sizeof(*status));
	group->status.argsz = sizeof(*status);

	return ioctl(group->fd, VFIO_GROUP_GET_STATUS, status);
}

static inline int
vfio_group_set_container(struct vfio_group *group, struct vfio_container *container)
{
	return ioctl(group->fd, VFIO_GROUP_SET_CONTAINER, &container->fd);
}

static inline int
vfio_group_get_device_fd(struct vfio_group *group, const char *device_name)
{
	return ioctl(group->fd, VFIO_GROUP_GET_DEVICE_FD, device_name);
}

static inline int
vfio_device_get_info(int device_fd, struct vfio_device_info *info)
{
	memset(info, 0, sizeof(*info));
	info->argsz = sizeof(*info);

	return ioctl(device_fd, VFIO_DEVICE_GET_INFO, info);
}

static inline int
vfio_device_get_region_info(int device_fd, struct vfio_region_info *region)
{
	memset(region, 0, sizeof(*region));
	region->argsz = sizeof(*region);

	return ioctl(device_fd, VFIO_DEVICE_GET_REGION_INFO, region);
}

static inline void *
vfio_map_region(int device_fd, size_t size, off_t offset)
{
	return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, device_fd, offset);
}

static inline int
vfio_iommu_get_info(struct vfio_container *container, struct vfio_iommu_type1_info *info)
{
	memset(info, 0, sizeof(*info));
	info->argsz = sizeof(*info);

	return ioctl(container->fd, VFIO_IOMMU_GET_INFO, info);
}

static inline int
vfio_iommu_map_dma(struct vfio_container *container, struct vfio_iommu_type1_dma_map *map)
{
	return ioctl(container->fd, VFIO_IOMMU_MAP_DMA, map);
}

static inline int
vfio_iommu_unmap_dma(struct vfio_container *container, struct vfio_iommu_type1_dma_unmap *unmap)
{
	return ioctl(container->fd, VFIO_IOMMU_UNMAP_DMA, unmap);
}

static inline int
vfio_device_get_irq_info(struct vfio_device *dev, struct vfio_irq_info *irq)
{
	memset(irq, 0, sizeof(*irq));
	irq->argsz = sizeof(*irq);
	return ioctl(dev->fd, VFIO_DEVICE_GET_IRQ_INFO, irq);
}

static inline int
vfio_device_set_irqs(struct vfio_device *dev, struct vfio_irq_set *irq_set)
{
	return ioctl(dev->fd, VFIO_DEVICE_SET_IRQS, irq_set);
}

static inline int
vfio_device_reset(struct vfio_device *dev)
{
	return ioctl(dev->fd, VFIO_DEVICE_RESET);
}

static inline int
vfio_device_get_pci_hot_reset_info(struct vfio_device *dev, struct vfio_pci_hot_reset_info *info)
{
	memset(info, 0, sizeof(*info));
	info->argsz = sizeof(*info);

	return ioctl(dev->fd, VFIO_DEVICE_GET_PCI_HOT_RESET_INFO, info);
}

static inline int
vfio_device_pci_hot_reset(struct vfio_device *dev, struct vfio_pci_hot_reset *reset)
{
	return ioctl(dev->fd, VFIO_DEVICE_PCI_HOT_RESET, reset);
}
