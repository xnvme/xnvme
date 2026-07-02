// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_platform.h>
#include <xnvme_be_registry.h>

#ifdef XNVME_PLATFORM_LINUX_ENABLED
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <upcie/pci.h>
#include <sys/stat.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

static uint32_t
xnvme_platform_linux_classify(const char *uri)
{
	struct stat st;

	if (!stat(uri, &st)) {
		if (S_ISREG(st.st_mode)) {
			return XNVME_BE_CAP_FILE;
		}
		if (S_ISCHR(st.st_mode)) {
			return XNVME_BE_CAP_NVME_CDEV;
		}
		if (S_ISBLK(st.st_mode)) {
			const char *base = strrchr(uri, '/');

			base = base ? base + 1 : uri;
			if (!strncmp(base, "nvme", 4)) {
				return XNVME_BE_CAP_NVME_BDEV;
			}
			return XNVME_BE_CAP_BDEV;
		}
		return 0;
	}

	{
		size_t len = strlen(uri);

		if (len >= 2 && !strcmp(uri + len - 2, "GB")) {
			return XNVME_BE_CAP_RAMDISK;
		}
	}

	/* PCI BDF pattern: DDDD:DD:DD.D */
	{
		unsigned domain, bus, dev, func;

		if (sscanf(uri, "%x:%x:%x.%u", &domain, &bus, &dev, &func) == 4) {
			return XNVME_BE_CAP_NVME_PCIE;
		}
	}

	/* Fabrics address:port */
	if (strchr(uri, ':')) {
		return XNVME_BE_CAP_NVME_TCP;
	}

	/* A path we could not stat() and which matched nothing else is assumed
	 * to be a not-yet-created file, e.g. the destination of a copy. */
	return XNVME_BE_CAP_FILE;
}

struct xnvme_linux_scan_args {
	xnvme_scan_cb cb_func;
	void *cb_args;
	int stopped;
};

/**
 * Read the kernel driver name bound to the given PCI BDF from sysfs.
 *
 * @param bdf PCI bus-device-function string (e.g. "0000:01:00.0")
 * @param driver Buffer to store the driver name, empty string if none is bound
 * @param driver_sz Size of the driver buffer
 */
static void
_scan_pci_read_driver(const char *bdf, char *driver, size_t driver_sz)
{
	char link_target[PATH_MAX] = {0};
	char link_path[PATH_MAX] = {0};
	ssize_t len;

	snprintf(link_path, sizeof(link_path), "/sys/bus/pci/devices/%s/driver", bdf);
	len = readlink(link_path, link_target, sizeof(link_target) - 1);
	if (len < 0) {
		XNVME_DEBUG("FAILED: readlink('%s'), errno: %d", link_path, errno);
		return;
	}
	if (!len) {
		XNVME_DEBUG("FAILED: readlink('%s') returned empty target", link_path);
		return;
	}
	if (len >= (ssize_t)sizeof(link_target) - 1) {
		XNVME_DEBUG("FAILED: readlink('%s') truncated", link_path);
		return;
	}

	link_target[len] = '\0';
	{
		char *name;

		name = strrchr(link_target, '/');
		name = name ? name + 1 : link_target;
		snprintf(driver, driver_sz, "%.*s", (int)(driver_sz - 1), name);
	}
}

/**
 * Report a PCI NVMe controller to the scan callback.
 *
 * @param args Scan arguments containing callback and state
 * @param bdf PCI bus-device-function string
 * @param driver Kernel driver name bound to the device
 *
 * @return Non-zero if the callback requested a stop, 0 otherwise
 */
static int
_scan_pci_report_ctrlr(struct xnvme_linux_scan_args *args, const char *bdf, const char *driver)
{
	struct xnvme_ident ident = {.dtype = XNVME_DEV_TYPE_NVME_CONTROLLER};

	snprintf(ident.uri, sizeof(ident.uri), "%s", bdf);
	snprintf(ident.kernel_driver, sizeof(ident.kernel_driver), "%s", driver);

	if (args->cb_func(&ident, args->cb_args)) {
		args->stopped = 1;
		return 1;
	}

	return 0;
}

/**
 * Read the NVMe namespace ID from sysfs.
 *
 * @param ns_dir Sysfs directory for the NVMe controller (e.g. ".../nvme/nvme0")
 * @param ns_name Namespace directory entry name (e.g. "nvme0n1")
 *
 * @return The namespace ID on success, 0 on failure
 */
static uint32_t
_scan_pci_read_nsid(const char *ns_dir, const char *ns_name)
{
	char buf[32] = {0};
	char path[PATH_MAX] = {0};
	uint32_t nsid;
	ssize_t nread;
	int fd;

	snprintf(path, sizeof(path), "%s/%s/nsid", ns_dir, ns_name);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		XNVME_DEBUG("FAILED: open('%s'), errno: %d", path, errno);
		return 0;
	}

	nread = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (nread <= 0) {
		XNVME_DEBUG("FAILED: read('%s'), errno: %d", path, errno);
		return 0;
	}

	buf[nread] = '\0';
	nsid = strtoul(buf, NULL, 0);
	if (!nsid) {
		XNVME_DEBUG("FAILED: strtoul('%s') from '%s'", buf, path);
	}

	return nsid;
}

/**
 * Report a single namespace device to the scan callback.
 *
 * @param args Scan arguments containing callback and state
 * @param uri Device path (e.g. "/dev/nvme0n1" or "/dev/ng0n1")
 * @param driver Kernel driver name bound to the device
 * @param nsid NVMe namespace ID
 *
 * @return Non-zero if the callback requested a stop, 0 otherwise
 */
static int
_scan_pci_report_ns(struct xnvme_linux_scan_args *args, const char *uri, const char *driver,
		    uint32_t nsid)
{
	struct xnvme_ident ident = {.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE, .nsid = nsid};

	snprintf(ident.uri, sizeof(ident.uri), "%s", uri);
	snprintf(ident.kernel_driver, sizeof(ident.kernel_driver), "%s", driver);

	if (args->cb_func(&ident, args->cb_args)) {
		args->stopped = 1;
		return 1;
	}

	return 0;
}

/**
 * Walk sysfs for kernel NVMe namespaces under the given BDF and report both
 * block (/dev/nvmeXnY) and char (/dev/ngXnY) devices to the scan callback.
 *
 * @param args Scan arguments containing callback and state
 * @param bdf PCI bus-device-function string
 * @param driver Kernel driver name bound to the device
 *
 * @return Non-zero if the callback requested a stop, 0 otherwise
 */
static int
_scan_pci_report_namespaces(struct xnvme_linux_scan_args *args, const char *bdf,
			    const char *driver)
{
	char nvme_dir[PATH_MAX] = {0};
	struct dirent *entry;
	DIR *dir;

	snprintf(nvme_dir, sizeof(nvme_dir), "/sys/bus/pci/devices/%s/nvme", bdf);

	dir = opendir(nvme_dir);
	if (!dir) {
		XNVME_DEBUG("FAILED: opendir('%s'), errno: %d", nvme_dir, errno);
		return 0;
	}

	while ((entry = readdir(dir)) != NULL) {
		char ns_dir[PATH_MAX] = {0};
		struct dirent *nsent;
		DIR *nsdir;
		int ctrlr;

		if (sscanf(entry->d_name, "nvme%d", &ctrlr) != 1) {
			continue;
		}

		snprintf(ns_dir, sizeof(ns_dir), "%s/%s", nvme_dir, entry->d_name);
		nsdir = opendir(ns_dir);
		if (!nsdir) {
			XNVME_DEBUG("FAILED: opendir('%s'), errno: %d", ns_dir, errno);
			continue;
		}

		while ((nsent = readdir(nsdir)) != NULL) {
			char uri[PATH_MAX] = {0};
			int ctrlr_idx, ns_idx;
			uint32_t nsid;

			if (sscanf(nsent->d_name, "nvme%dn%d", &ctrlr_idx, &ns_idx) != 2) {
				continue;
			}

			nsid = _scan_pci_read_nsid(ns_dir, nsent->d_name);
			if (!nsid) {
				XNVME_DEBUG("INFO: skipping '%s'; failed to read nsid",
					    nsent->d_name);
				continue;
			}

			snprintf(uri, sizeof(uri), "/dev/%s", nsent->d_name);
			if (_scan_pci_report_ns(args, uri, driver, nsid)) {
				closedir(nsdir);
				closedir(dir);
				return 1;
			}

			snprintf(uri, sizeof(uri), "/dev/ng%dn%d", ctrlr_idx, ns_idx);
			if (_scan_pci_report_ns(args, uri, driver, nsid)) {
				closedir(nsdir);
				closedir(dir);
				return 1;
			}
		}
		closedir(nsdir);
	}
	closedir(dir);

	return 0;
}

static int
_scan_pci_cb(struct pci_func *func, void *cb_arg)
{
	struct xnvme_linux_scan_args *args = cb_arg;
	uint32_t classcode = func->ident.classcode;
	char driver[32] = {0};

	// Filter for NVMe: base=0x01, sub=0x08, progif=0x02
	if ((classcode >> 8) != 0x0108 || (classcode & 0xFF) != 0x02) {
		return PCI_SCAN_ACTION_RELEASE_FUNC;
	}

	_scan_pci_read_driver(func->bdf, driver, sizeof(driver));

	if (_scan_pci_report_ctrlr(args, func->bdf, driver)) {
		return PCI_SCAN_ACTION_RELEASE_FUNC;
	}

	if (!strcmp(driver, "nvme")) {
		if (_scan_pci_report_namespaces(args, func->bdf, driver)) {
			return PCI_SCAN_ACTION_RELEASE_FUNC;
		}
	}

	return PCI_SCAN_ACTION_RELEASE_FUNC;
}

static int
xnvme_platform_linux_scan(const char *sys_uri, struct xnvme_opts *XNVME_UNUSED(opts),
			  xnvme_scan_cb cb_func, void *cb_args)
{
	struct xnvme_linux_scan_args args = {0};

	if (sys_uri) {
		XNVME_DEBUG("FAILED: sys_uri: %s is not supported", sys_uri);
		return -ENOSYS;
	}

	args.cb_func = cb_func;
	args.cb_args = cb_args;
	args.stopped = 0;

	pci_scan(_scan_pci_cb, &args);

	return 0;
}

struct xnvme_platform g_xnvme_platform_linux = {
	.name = "linux",
	.classify = xnvme_platform_linux_classify,
	.backends =
		(const struct xnvme_be_config *const[]){
#ifdef XNVME_BE_SPDK_ENABLED
			&g_xnvme_be_spdk,
#endif
#ifdef XNVME_BE_VFIO_ENABLED
			&g_xnvme_be_vfio,
#endif
#ifdef XNVME_BE_UPCIE_ENABLED
			&g_xnvme_be_upcie,
#endif
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
			&g_xnvme_be_upcie_cuda,
#endif
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
			&g_xnvme_be_upcie_hip,
			&g_xnvme_be_upcie_hip_gpuinit,
#endif
			&g_xnvme_be_linux_emu_nvme,
#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
			&g_xnvme_be_linux_emu_block,
#endif
#ifdef XNVME_BE_LINUX_LIBURING_ENABLED
			&g_xnvme_be_linux_ucmd_nvme,
			&g_xnvme_be_linux_iou_nvme,
#endif
#if defined(XNVME_BE_LINUX_LIBURING_ENABLED) && defined(XNVME_BE_LINUX_BLOCK_ENABLED)
			&g_xnvme_be_linux_iou_block,
#endif
#ifdef XNVME_BE_LINUX_LIBAIO_ENABLED
			&g_xnvme_be_linux_aio_nvme,
#endif
#if defined(XNVME_BE_LINUX_LIBAIO_ENABLED) && defined(XNVME_BE_LINUX_BLOCK_ENABLED)
			&g_xnvme_be_linux_aio_block,
#endif
#ifdef XNVME_BE_CBI_ASYNC_POSIX_ENABLED
			&g_xnvme_be_linux_posix_nvme,
#endif
			&g_xnvme_be_linux_thrpool_nvme,
#ifdef XNVME_BE_LINUX_BLOCK_ENABLED
			&g_xnvme_be_linux_thrpool_block,
#endif
			&g_xnvme_be_linux_nil_nvme,
			&g_xnvme_be_linux_emu_file,
			&g_xnvme_be_linux_thrpool_file,
#ifdef XNVME_BE_LINUX_LIBURING_ENABLED
			&g_xnvme_be_linux_iou_file,
#endif
#ifdef XNVME_BE_LINUX_LIBAIO_ENABLED
			&g_xnvme_be_linux_aio_file,
#endif
#ifdef XNVME_BE_RAMDISK_ENABLED
			&g_xnvme_be_ramdisk_nil,
			&g_xnvme_be_ramdisk_thrpool,
			&g_xnvme_be_ramdisk_emu,
#endif
			NULL,
		},
	.dev_open = xnvme_platform_dev_open,
	.scan = xnvme_platform_linux_scan,
	.enumerate = xnvme_platform_enumerate,
};
#endif
