// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_platform.h>
#include <xnvme_be_registry.h>

#ifdef XNVME_PLATFORM_MACOS_ENABLED
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <IOKit/IOTypes.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/nvme/NVMeSMARTLibExternal.h>
#include <CoreFoundation/CoreFoundation.h>
#include <sys/stat.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

static uint32_t
xnvme_platform_macos_classify(const char *uri)
{
	struct stat st;

	if (!stat(uri, &st)) {
		if (S_ISREG(st.st_mode)) {
			return XNVME_BE_CAP_FILE;
		}
		if (S_ISBLK(st.st_mode)) {
			return XNVME_BE_CAP_BDEV;
		}
		if (S_ISCHR(st.st_mode)) {
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

	/* MacVFN DriverKit service names */
	if (!strncmp(uri, "MacVFN-", 7)) {
		return XNVME_BE_CAP_NVME_PCIE;
	}

	return 0;
}

static int
xnvme_platform_macos_scan(const char *sys_uri, struct xnvme_opts *XNVME_UNUSED(opts),
			  xnvme_scan_cb cb_func, void *cb_args)
{
	if (sys_uri) {
		XNVME_DEBUG("FAILED: sys_uri: %s is not supported", sys_uri);
		return -ENOSYS;
	}

	for (int nid = 0; nid < 256; nid++) {
		char path[128] = {0};
		char disk_id[32] = {0};
		CFMutableDictionaryRef matching_dict;
		io_object_t ioservice_device;
		CFBooleanRef is_nvme;
		int nsid = 0;

		snprintf(disk_id, sizeof(disk_id), "disk%d", nid);
		snprintf(path, sizeof(path), "/dev/%s", disk_id);

		if (access(path, F_OK)) {
			continue;
		}

		matching_dict = IOBSDNameMatching(kIOMasterPortDefault, 0, disk_id);
		if (!matching_dict) {
			continue;
		}

		ioservice_device =
			IOServiceGetMatchingService(kIOMasterPortDefault, matching_dict);
		if (!ioservice_device) {
			continue;
		}

		is_nvme = IORegistryEntrySearchCFProperty(
			ioservice_device, kIOServicePlane, CFSTR(kIOPropertyNVMeSMARTCapableKey),
			kCFAllocatorDefault,
			kIORegistryIterateRecursively | kIORegistryIterateParents);
		if (!is_nvme) {
			IOObjectRelease(ioservice_device);
			continue;
		}
		CFRelease(is_nvme);

		{
			CFTypeRef cf_nsid = IORegistryEntrySearchCFProperty(
				ioservice_device, kIOServicePlane, CFSTR("NamespaceID"),
				kCFAllocatorDefault,
				kIORegistryIterateRecursively | kIORegistryIterateParents);
			if (cf_nsid) {
				CFNumberGetValue(cf_nsid, kCFNumberIntType, &nsid);
				CFRelease(cf_nsid);
			}
		}

		IOObjectRelease(ioservice_device);

		{
			struct xnvme_ident ident = {0};

			snprintf(ident.uri, sizeof(ident.uri), "%s", path);
			ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
			ident.nsid = nsid ? nsid : 1;

			if (cb_func(&ident, cb_args)) {
				return 0;
			}
		}
	}

	/** Scan for MacVFN DriverKit devices */
	{
		const char *service_classes[] = {"IOUserBlockStorageDevice", "IOUserService"};

		for (int i = 0; i < 2; ++i) {
			CFMutableDictionaryRef matching_dict;
			io_iterator_t iterator;
			io_service_t service;
			kern_return_t ret;

			matching_dict = IOServiceMatching(service_classes[i]);
			ret = IOServiceGetMatchingServices(kIOMasterPortDefault, matching_dict,
							   &iterator);
			if (ret != kIOReturnSuccess || !iterator) {
				continue;
			}

			while ((service = IOIteratorNext(iterator))) {
				struct xnvme_ident ident = {0};
				char name[XNVME_IDENT_URI_LEN] = {0};

				IORegistryEntryGetName(service, name);
				IOObjectRelease(service);

				if (strncmp("MacVFN-", name, 7)) {
					continue;
				}

				snprintf(ident.uri, sizeof(ident.uri), "%s", name);
				ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
				ident.nsid = 1;

				if (cb_func(&ident, cb_args)) {
					IOObjectRelease(iterator);
					return 0;
				}
			}

			IOObjectRelease(iterator);
		}
	}

	return 0;
}

struct xnvme_platform g_xnvme_platform_macos = {
	.name = "macos",
	.classify = xnvme_platform_macos_classify,
	.backends =
		(const struct xnvme_be_config *const[]){
			&g_xnvme_be_macos_emu,
			&g_xnvme_be_macos_thrpool,
			&g_xnvme_be_macos_posix,
			&g_xnvme_be_driverkit_native,
			&g_xnvme_be_driverkit_emu,
#ifdef XNVME_BE_RAMDISK_ENABLED
			&g_xnvme_be_ramdisk_nil,
			&g_xnvme_be_ramdisk_thrpool,
			&g_xnvme_be_ramdisk_emu,
#endif
			NULL,
		},
	.dev_open = xnvme_platform_dev_open,
	.scan = xnvme_platform_macos_scan,
	.enumerate = xnvme_platform_enumerate,
};
#endif
