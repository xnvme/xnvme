// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * How a controller is attached: driver probing, mode selection, vfio wiring
 *
 * Everything here answers "what is this BDF bound to, and how do we reach it",
 * keeping the runtime-environment and controller lifecycle in
 * xnvme_be_upcie_dev.c free of the sysfs and vfio detail.
 */
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_ENABLED
#include <dirent.h>
#include <limits.h>
#include <xnvme_dev.h>
#include <xnvme_be_upcie.h>

int
xnvme_be_upcie_get_driver_name(const char *bdf, char *driver_name, size_t driver_name_len)
{
	char path[PATH_MAX] = {0};
	char link[PATH_MAX] = {0};
	ssize_t nbytes;
	char *base;

	snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/driver", bdf);

	nbytes = readlink(path, link, sizeof(link) - 1);
	if (nbytes < 0) {
		return -errno;
	}

	base = strrchr(link, '/');
	if (!base || !base[1]) {
		return -EINVAL;
	}

	snprintf(driver_name, driver_name_len, "%s", base + 1);

	return 0;
}

int
xnvme_be_upcie_resolve_vfio_cdev(const char *bdf, char *cdev_path, size_t cdev_path_len)
{
	char sysfs_path[PATH_MAX] = {0};
	DIR *dir;
	struct dirent *ent;
	int found = 0;

	snprintf(sysfs_path, sizeof(sysfs_path), "/sys/bus/pci/devices/%s/vfio-dev", bdf);
	dir = opendir(sysfs_path);
	if (!dir) {
		return -errno;
	}
	while ((ent = readdir(dir))) {
		if (strncmp(ent->d_name, "vfio", 4) != 0) {
			continue;
		}
		snprintf(cdev_path, cdev_path_len, "/dev/vfio/devices/%s", ent->d_name);
		found = 1;
		break;
	}
	closedir(dir);
	return found ? 0 : -ENOENT;
}

/**
 * Return non-zero if the target BDF has a vfio-cdev entry
 */
static int
_bdf_has_vfio_cdev(const char *bdf)
{
	char cdev_path[PATH_MAX] = {0};

	return xnvme_be_upcie_resolve_vfio_cdev(bdf, cdev_path, sizeof(cdev_path)) == 0;
}

/**
 * Read the kernel driver bound to `bdf` and derive the attachment mode.
 *
 * vfio-pci        -> VFIO_CDEV if /dev/iommu and a vfio-cdev entry both
 *                    exist, otherwise VFIO_TYPE1 (legacy vfio container).
 *                    Env override: XNVME_UPCIE_VFIO_MODE = iommufd | type1.
 * uio_pci_generic -> UIO_LUT (pci_bar_map + hostmem hugepage + LUT).
 * anything else   -> -ENOTSUP.
 *
 * On VFIO_CDEV, `cdev_path` is filled with the device node to open.
 *
 * @return 0 on success, negative errno on error.
 */
int
xnvme_be_upcie_mode_from_driver(const char *bdf, const char *driver_name,
				enum xnvme_be_upcie_mode *mode, char *cdev_path,
				size_t cdev_path_len)
{
	int err;

	if (!strcmp(driver_name, "uio_pci_generic")) {
		*mode = XNVME_BE_UPCIE_MODE_UIO_LUT;
		return 0;
	}

	if (strcmp(driver_name, "vfio-pci")) {
		XNVME_DEBUG("FAILED: unsupported driver '%s'", driver_name);
		return -ENOTSUP;
	}

	{
		const char *override = getenv("XNVME_UPCIE_VFIO_MODE");

		if (override && !strcmp(override, "iommufd")) {
			*mode = XNVME_BE_UPCIE_MODE_VFIO_CDEV;
		} else if (override && !strcmp(override, "type1")) {
			*mode = XNVME_BE_UPCIE_MODE_VFIO_TYPE1;
		} else if (access("/dev/iommu", R_OK | W_OK) == 0 && _bdf_has_vfio_cdev(bdf)) {
			*mode = XNVME_BE_UPCIE_MODE_VFIO_CDEV;
		} else {
			*mode = XNVME_BE_UPCIE_MODE_VFIO_TYPE1;
		}
	}

	if (*mode != XNVME_BE_UPCIE_MODE_VFIO_CDEV) {
		return 0;
	}

	err = xnvme_be_upcie_resolve_vfio_cdev(bdf, cdev_path, cdev_path_len);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_upcie_resolve_vfio_cdev(%s); err(%d)", bdf, err);
		return err;
	}

	return 0;
}
/**
 * Attach a controller's group to the RTE's type1 container, and on the
 * first controller also set_iommu(TYPE1) + MAP_DMA the hostmem hugepage
 * + init the shared dmamem_heap.
 *
 * Rolls back the group open (and any RTE-side state it turned on) on
 * failure. The container itself and the hostmem hugepage remain owned
 * by the RTE.
 */
int
xnvme_be_upcie_type1_attach(struct xnvme_be_upcie_ctrlr *ctrlr, const char *bdf)
{
	int api_version = 0;
	int group_id = -1;
	int err;

	err = vfio_device_get_iommu_group_id(bdf, &group_id);
	if (err) {
		XNVME_DEBUG("FAILED: vfio_device_get_iommu_group_id(%s); err(%d)", bdf, err);
		return err;
	}

	err = vfio_group_open(group_id, &ctrlr->attach.type1_group);
	if (err) {
		XNVME_DEBUG("FAILED: vfio_group_open(%d); err(%d)", group_id, err);
		return err;
	}

	err = vfio_group_get_status(&ctrlr->attach.type1_group);
	if (err < 0) {
		XNVME_DEBUG("FAILED: vfio_group_get_status(); errno(%d)", errno);
		err = -errno;
		goto fail_group;
	}
	if (!(ctrlr->attach.type1_group.status.flags & VFIO_GROUP_FLAGS_VIABLE)) {
		XNVME_DEBUG("FAILED: iommu group %d not viable", group_id);
		err = -EBUSY;
		goto fail_group;
	}

	err = vfio_group_set_container(&ctrlr->attach.type1_group, &g_upcie_rte.type1.container);
	if (err < 0) {
		XNVME_DEBUG("FAILED: vfio_group_set_container(); errno(%d)", errno);
		err = -errno;
		goto fail_group;
	}
	ctrlr->attach.type1_group_attached = 1;

	if (g_upcie_rte.type1.iommu_set) {
		/* Container already set_iommu'd + mapped by an earlier controller. */
		return 0;
	}

	err = vfio_get_api_version(&g_upcie_rte.type1.container, &api_version);
	if (err) {
		XNVME_DEBUG("FAILED: vfio_get_api_version(); err(%d)", err);
		goto fail_group;
	}
	if (api_version != VFIO_API_VERSION) {
		XNVME_DEBUG("FAILED: unexpected VFIO_API_VERSION(%d != %d)", api_version,
			    VFIO_API_VERSION);
		err = -EINVAL;
		goto fail_group;
	}

	if (!vfio_check_extension(&g_upcie_rte.type1.container, VFIO_TYPE1_IOMMU)) {
		XNVME_DEBUG("FAILED: VFIO_TYPE1_IOMMU extension not supported");
		err = -ENOTSUP;
		goto fail_group;
	}

	err = vfio_set_iommu(&g_upcie_rte.type1.container, VFIO_TYPE1_IOMMU);
	if (err < 0) {
		XNVME_DEBUG("FAILED: vfio_set_iommu(TYPE1); errno(%d)", errno);
		err = -errno;
		goto fail_group;
	}
	g_upcie_rte.type1.iommu_set = 1;

	/* MAP_DMA the whole hugepage at a caller-chosen base_iova; the
	 * dmamem sits on that mapping as ARITHMETIC (base_iova + offset). */
	err = dmamem_from_hostmem_type1(&g_upcie_rte.mem.dmem, &g_upcie_rte.type1.container,
					(uint64_t)0, &g_upcie_rte.mem.hp);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_from_hostmem_type1(); err(%d)", err);
		goto fail_group;
	}
	g_upcie_rte.mem.dmem_alive = 1;

	err = dmamem_heap_init(&g_upcie_rte.mem.heap, &g_upcie_rte.mem.dmem, 4096);
	if (err) {
		XNVME_DEBUG("FAILED: dmamem_heap_init(); err(%d)", err);
		dmamem_destroy(&g_upcie_rte.mem.dmem);
		g_upcie_rte.mem.dmem_alive = 0;
		goto fail_group;
	}
	g_upcie_rte.mem.heap_alive = 1;

	return 0;

fail_group:
	if (ctrlr->attach.type1_group.fd >= 0) {
		vfio_group_close(&ctrlr->attach.type1_group);
		ctrlr->attach.type1_group_attached = 0;
	}
	return err;
}

#endif
