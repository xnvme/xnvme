// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_lock.h>
#include <xnvme_platform.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#ifndef XNVME_PLATFORM_WINDOWS_ENABLED
#include <unistd.h>
#endif

const char *
xnvme_driver_default(void)
{
	return g_xnvme_platform->driver_default ? g_xnvme_platform->driver_default() : NULL;
}

const char *
xnvme_driver_for_be(const char *be)
{
	if (!g_xnvme_platform->driver_attach) {
		errno = ENOSYS;
		return NULL;
	}
	if (!be) {
		errno = EINVAL;
		return NULL;
	}
	if (!strcmp(be, g_xnvme_platform->name)) {
		return "nvme";
	}

	for (int i = 0; g_xnvme_platform->backends[i]; ++i) {
		const struct xnvme_be_attr *attr = &g_xnvme_platform->backends[i]->attr;

		if (strcmp(be, attr->name)) {
			continue;
		}
		if (attr->caps & XNVME_BE_CAP_NVME_PCIE) {
			// libvfn reaches the device through vfio alone; the others take either
			return strcmp(attr->name, "libvfn") ? xnvme_driver_default() : "vfio-pci";
		}
		if (attr->caps &
		    (XNVME_BE_CAP_NVME_CDEV | XNVME_BE_CAP_NVME_BDEV | XNVME_BE_CAP_BDEV)) {
			return "nvme";
		}
		errno = EINVAL;
		return NULL;
	}

	errno = ENOENT;
	return NULL;
}

int
xnvme_driver_attach(const char *const *bdfs, int nbdfs, const char *driver, int flags,
		    struct xnvme_driver_result *results)
{
	int err;

	if (!g_xnvme_platform->driver_attach) {
		return -ENOSYS;
	}
	if ((nbdfs && (!bdfs || !results)) || nbdfs < 0) {
		return -EINVAL;
	}

	xnvme_lock();
	err = g_xnvme_platform->driver_attach(bdfs, nbdfs, driver, flags, results);
	xnvme_unlock();

	return err;
}

int
xnvme_driver_detach(const char *const *bdfs, int nbdfs, const char *driver, int flags,
		    struct xnvme_driver_result *results)
{
	int err;

	if (!g_xnvme_platform->driver_detach) {
		return -ENOSYS;
	}
	if ((nbdfs && (!bdfs || !results)) || nbdfs < 0) {
		return -EINVAL;
	}

	xnvme_lock();
	err = g_xnvme_platform->driver_detach(bdfs, nbdfs, driver, flags, results);
	xnvme_unlock();

	return err;
}

#ifndef XNVME_PLATFORM_WINDOWS_ENABLED
static uint32_t
le32(const uint8_t *buf)
{
	return buf[0] | buf[1] << 8 | buf[2] << 16 | (uint32_t)buf[3] << 24;
}

const char *
xnvme_driver_signature(const char *path)
{
	static uint8_t buf[65600 + 8];
	ssize_t nbytes;
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		return NULL;
	}
	memset(buf, 0, sizeof(buf));
	nbytes = pread(fd, buf, sizeof(buf), 0);
	close(fd);
	if (nbytes < 4096) {
		return NULL;
	}

	if (!memcmp(buf + 512, "EFI PART", 8)) {
		return "a GPT partition table";
	}
	if (!memcmp(buf + 512, "LABELONE", 8)) {
		return "an LVM physical volume";
	}
	if (!memcmp(buf, "XFSB", 4)) {
		return "an XFS filesystem";
	}
	if (buf[1080] == 0x53 && buf[1081] == 0xEF) {
		return "an ext2, ext3 or ext4 filesystem";
	}
	if (nbytes >= 65608 && !memcmp(buf + 65600, "_BHRfS_M", 8)) {
		return "a btrfs filesystem";
	}
	if (!memcmp(buf + 4086, "SWAPSPACE2", 10)) {
		return "a swap area";
	}
	if (le32(buf + 4096) == 0xa92b4efc) {
		return "an md RAID member";
	}
	if (buf[510] == 0x55 && buf[511] == 0xAA) {
		return "an MBR partition table or a FAT filesystem";
	}

	return NULL;
}
#endif
