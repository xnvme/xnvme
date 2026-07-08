// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_platform.h>
#include <xnvme_be_registry.h>

#ifdef XNVME_PLATFORM_FREEBSD_ENABLED
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/pciio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

static uint32_t
xnvme_platform_freebsd_classify(const char *uri)
{
	struct stat st;

	if (!stat(uri, &st)) {
		if (S_ISREG(st.st_mode)) {
			return XNVME_BE_CAP_FILE;
		}
		if (S_ISCHR(st.st_mode)) {
			const char *base = strrchr(uri, '/');

			base = base ? base + 1 : uri;
			if (!strncmp(base, "nvme", 4)) {
				return XNVME_BE_CAP_NVME_CDEV;
			}
			return 0;
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

struct xnvme_freebsd_scan_args {
	xnvme_scan_cb cb_func;
	void *cb_args;
	int stopped;
};

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
_scan_freebsd_report_ctrlr(struct xnvme_freebsd_scan_args *args, const char *bdf,
			   const char *driver)
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
 * Report a single namespace device to the scan callback.
 *
 * @param args Scan arguments containing callback and state
 * @param uri Device path (e.g. "/dev/nvme0ns1")
 * @param driver Kernel driver name bound to the device
 * @param nsid NVMe namespace ID
 *
 * @return Non-zero if the callback requested a stop, 0 otherwise
 */
static int
_scan_freebsd_report_ns(struct xnvme_freebsd_scan_args *args, const char *uri, const char *driver,
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
 * Scan /dev for kernel NVMe namespaces belonging to the given controller unit
 * and report them to the scan callback.
 *
 * @param args Scan arguments containing callback and state
 * @param unit Controller unit number (e.g. 0 for nvme0)
 * @param driver Kernel driver name bound to the device
 *
 * @return Non-zero if the callback requested a stop, 0 otherwise
 */
static int
_scan_freebsd_report_namespaces(struct xnvme_freebsd_scan_args *args, int unit, const char *driver)
{
	char nvme_dir[PATH_MAX] = {0};
	struct dirent *entry;
	DIR *dir;

	snprintf(nvme_dir, sizeof(nvme_dir), "/dev");

	dir = opendir(nvme_dir);
	if (!dir) {
		XNVME_DEBUG("FAILED: opendir('%s'), errno: %d", nvme_dir, errno);
		return 0;
	}

	while ((entry = readdir(dir)) != NULL) {
		char uri[PATH_MAX] = {0};
		int ctrlr, nsid;

		// Match nvmeXnsY pattern
		if (sscanf(entry->d_name, "nvme%dns%d", &ctrlr, &nsid) != 2) {
			continue;
		}
		if (ctrlr != unit) {
			continue;
		}

		snprintf(uri, sizeof(uri), "/dev/%s", entry->d_name);
		if (_scan_freebsd_report_ns(args, uri, driver, nsid)) {
			closedir(dir);
			return 1;
		}
	}
	closedir(dir);

	return 0;
}

static int
xnvme_platform_freebsd_scan(const char *sys_uri, struct xnvme_opts *XNVME_UNUSED(opts),
			    xnvme_scan_cb cb_func, void *cb_args)
{
	struct xnvme_freebsd_scan_args args = {0};
	struct pci_conf_io pc = {0};
	struct pci_conf conf[32];
	int pci_fd;
	int err = 0;

	if (sys_uri) {
		XNVME_DEBUG("FAILED: sys_uri: %s is not supported", sys_uri);
		return -ENOSYS;
	}

	args.cb_func = cb_func;
	args.cb_args = cb_args;
	args.stopped = 0;

	pci_fd = open("/dev/pci", O_RDONLY);
	if (pci_fd < 0) {
		XNVME_DEBUG("FAILED: open(/dev/pci): %d", errno);
		return -errno;
	}

	pc.match_buf_len = sizeof(conf);
	pc.matches = conf;

	do {
		if (ioctl(pci_fd, PCIOCGETCONF, &pc) < 0) {
			XNVME_DEBUG("FAILED: ioctl(PCIOCGETCONF): %d", errno);
			err = -errno;
			goto out;
		}

		if (pc.status == PCI_GETCONF_ERROR) {
			XNVME_DEBUG("FAILED: PCIOCGETCONF returned error");
			err = -EIO;
			goto out;
		}

		for (uint32_t i = 0; i < pc.num_matches; i++) {
			struct pci_conf *p = &conf[i];
			uint32_t classcode;
			char bdf[32] = {0};
			char driver[32] = {0};

			// Build classcode from base/sub/progif
			classcode = (p->pc_class << 16) | (p->pc_subclass << 8) | p->pc_progif;

			// Filter for NVMe: base=0x01, sub=0x08, progif=0x02
			if ((classcode >> 8) != 0x0108 || (classcode & 0xFF) != 0x02) {
				continue;
			}

			// Get bound driver name from pd_name
			if (p->pd_name[0]) {
				snprintf(driver, sizeof(driver), "%s", p->pd_name);
			}

			// Format BDF string
			snprintf(bdf, sizeof(bdf), "%04x:%02x:%02x.%x", p->pc_sel.pc_domain,
				 p->pc_sel.pc_bus, p->pc_sel.pc_dev, p->pc_sel.pc_func);

			if (_scan_freebsd_report_ctrlr(&args, bdf, driver)) {
				goto out;
			}

			// When bound to the kernel "nvme" driver, discover namespaces
			if (!strcmp(driver, "nvme")) {
				if (_scan_freebsd_report_namespaces(&args, p->pd_unit, driver)) {
					goto out;
				}
			}
		}
	} while (pc.status == PCI_GETCONF_MORE_DEVS);

out:
	close(pci_fd);
	return err;
}

struct xnvme_platform g_xnvme_platform_freebsd = {
	.name = "freebsd",
	.classify = xnvme_platform_freebsd_classify,
	.backends =
		(const struct xnvme_be_config *const[]){
#ifdef XNVME_BE_SPDK_ENABLED
			&g_xnvme_be_spdk,
#endif
			&g_xnvme_be_freebsd_kqueue_nvme,
			&g_xnvme_be_freebsd_emu_file,
			&g_xnvme_be_freebsd_kqueue_psync,
			&g_xnvme_be_freebsd_posix_nvme,
			&g_xnvme_be_freebsd_thrpool_nvme,
			&g_xnvme_be_freebsd_emu_nvme,
			&g_xnvme_be_freebsd_nil_nvme,
#ifdef XNVME_BE_RAMDISK_ENABLED
			&g_xnvme_be_ramdisk_nil,
			&g_xnvme_be_ramdisk_thrpool,
			&g_xnvme_be_ramdisk_emu,
#endif
			NULL,
		},
	.dev_open = xnvme_platform_dev_open,
	.scan = xnvme_platform_freebsd_scan,
	.enumerate = xnvme_platform_enumerate,
};
#endif
