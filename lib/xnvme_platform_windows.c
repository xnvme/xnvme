// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_platform.h>
#include <xnvme_be_registry.h>

#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
#include <errno.h>
#include <stdio.h>
#include <tchar.h>
#include <initguid.h>
#include <windows.h>
#include <Setupapi.h>
#include <xnvme_dev.h>

static int
xnvme_platform_windows_scan(const char *sys_uri, struct xnvme_opts *XNVME_UNUSED(opts),
			    xnvme_scan_cb cb_func, void *cb_args)
{
	HDEVINFO disk_class_devices;
	GUID disk_class_device_interface_guid = GUID_DEVINTERFACE_DISK;
	SP_DEVICE_INTERFACE_DATA device_interface_data;
	DWORD device_index = 0;
	int num_devices = 0;

	if (sys_uri) {
		XNVME_DEBUG("FAILED: sys_uri: %s is not supported", sys_uri);
		return -ENOSYS;
	}

	disk_class_devices = SetupDiGetClassDevs(&disk_class_device_interface_guid, NULL, NULL,
						 DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (disk_class_devices == INVALID_HANDLE_VALUE) {
		XNVME_DEBUG("FAILED: SetupDiGetClassDevs: %lu", GetLastError());
		return 0;
	}

	memset(&device_interface_data, 0, sizeof(SP_DEVICE_INTERFACE_DATA));
	device_interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	while (SetupDiEnumDeviceInterfaces(disk_class_devices, NULL,
					   &disk_class_device_interface_guid, device_index,
					   &device_interface_data)) {
		++device_index;
	}
	SetupDiDestroyDeviceInfoList(disk_class_devices);
	num_devices = device_index;

	for (int i = 0; i < num_devices; i++) {
		HANDLE dev_handle;
		char str_device_name[256] = {0};
		STORAGE_PROPERTY_QUERY query = {0};
		STORAGE_ADAPTER_DESCRIPTOR result = {0};
		ULONG ret_len;
		int ret;

		_stprintf_s(str_device_name, 256, _T("\\\\.\\PhysicalDrive%d"), i);

		dev_handle = CreateFile((LPCSTR)str_device_name, GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL, NULL);
		if (dev_handle == INVALID_HANDLE_VALUE) {
			XNVME_DEBUG("FAILED: CreateFile(%s): %lu", str_device_name,
				    GetLastError());
			continue;
		}

		query.PropertyId = (STORAGE_PROPERTY_ID)StorageAdapterProperty;
		query.QueryType = PropertyStandardQuery;

		ret = DeviceIoControl(dev_handle, IOCTL_STORAGE_QUERY_PROPERTY, &query,
				      sizeof(query), &result, sizeof(result), &ret_len, NULL);
		if (!ret) {
			XNVME_DEBUG("FAILED: IOCTL_STORAGE_QUERY_PROPERTY(%s): %lu",
				    str_device_name, GetLastError());
			CloseHandle(dev_handle);
			continue;
		}

		CloseHandle(dev_handle);

		if (result.BusType == BusTypeNvme) {
			struct xnvme_ident ident = {0};

			snprintf(ident.uri, sizeof(ident.uri), "%s", str_device_name);
			ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
			ident.nsid = 1;

			if (cb_func(&ident, cb_args)) {
				return 0;
			}
		}
	}

	return 0;
}

struct xnvme_platform g_xnvme_platform_windows = {
	.name = "windows",
	.backends =
		(const struct xnvme_be_config *const[]){
			&g_xnvme_be_windows_emu_nvme,
			&g_xnvme_be_windows_thrpool_nvme,
			&g_xnvme_be_windows_iocp_nvme,
			&g_xnvme_be_windows_iocp_th_nvme,
			&g_xnvme_be_windows_ioring_nvme,
			&g_xnvme_be_windows_nil_nvme,
#ifdef XNVME_BE_WINDOWS_FS_ENABLED
			&g_xnvme_be_windows_iocp_fs,
			&g_xnvme_be_windows_thrpool_fs,
#endif
#ifdef XNVME_BE_RAMDISK_ENABLED
			&g_xnvme_be_ramdisk_nil,
			&g_xnvme_be_ramdisk_thrpool,
			&g_xnvme_be_ramdisk_emu,
#endif
			NULL,
		},
	.dev_open = xnvme_platform_dev_open,
	.scan = xnvme_platform_windows_scan,
	.enumerate = xnvme_platform_enumerate,
};
#endif
