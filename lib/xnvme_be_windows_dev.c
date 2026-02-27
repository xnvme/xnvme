// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
#include <tchar.h>
#include <errno.h>
#include <fcntl.h>
#include <initguid.h>
#include <windows.h>
#include <Setupapi.h>
#include <xnvme_be_windows.h>

int
xnvme_be_windows_get_device_type(HANDLE dev_handle)
{
	ULONG ret_len;
	STORAGE_PROPERTY_QUERY query = {0};
	STORAGE_ADAPTER_DESCRIPTOR result = {0};
	int ret;

	query.PropertyId = (STORAGE_PROPERTY_ID)StorageAdapterProperty;
	query.QueryType = PropertyStandardQuery;

	ret = DeviceIoControl(dev_handle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
			      &result, sizeof(result), &ret_len, NULL);

	if (!ret) {
		XNVME_DEBUG("Error retriving Storage Query property. Error: %d\n", GetLastError());
	}

	return result.BusType;
}

/**
 * Close a device handle open by CreateFile()
 *
 * @param state Device handle obtained with CreateFile()
 *
 */
static inline void
_be_windows_state_term(struct xnvme_be_windows_state *state)
{
	if (!state) {
		return;
	}

	CloseHandle(state->sync_handle);
	state->sync_handle = NULL;
	CloseHandle(state->async_handle);
	state->async_handle = NULL;
}

int
xnvme_file_opts_to_win(struct xnvme_opts *opts)
{
	int flags = 0;

	flags |= opts->create ? O_CREAT : 0x0;
	flags |= opts->rdonly ? O_RDONLY : 0x0;
	flags |= opts->wronly ? O_WRONLY : 0x0;
	flags |= opts->rdwr ? O_RDWR : 0x0;
	flags |= opts->truncate ? O_TRUNC : 0x0;

	return flags;
}

/**
 * Creates a device handle (::xnvme_dev) based on the given device-uri in Windows
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 *
 * @return On success, a handle to the device. On error, GetLastError() is returned.
 */
int
xnvme_be_windows_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_windows_state *state = (void *)dev->be.state;
	struct xnvme_ident *ident = &dev->ident;
	struct xnvme_opts *opts = &dev->opts;
	int dw_creation_mode = 0;
	DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	DWORD access_mode = 0;
	DWORD file_attributes = FILE_ATTRIBUTE_NORMAL;
	int err = 0;
	int dev_type;

	int flags = xnvme_file_opts_to_win(opts);

	XNVME_DEBUG("INFO: open() : flags: 0x%x, opts->create_mode: 0x%x", flags,
		    opts->create_mode);

	state->fd = _open(ident->uri, flags, opts->create_mode);
	if (state->fd < 0) {
		XNVME_DEBUG("FAILED: open(uri: '%s') : state->fd: '%d', errno: %d", ident->uri,
			    state->fd, errno);

		state->fd = _open(ident->uri, O_RDONLY);
		if (state->fd < 0) {
			XNVME_DEBUG("FAILED: open(uri: '%s') : state->fd: '%d', errno: %d",
				    ident->uri, state->fd, errno);
			return -errno;
		}
	}

	struct stat dev_stat;
	err = fstat(state->fd, &dev_stat);
	if (err < 0) {
		XNVME_DEBUG("FAILED: open() : fstat() : err: %d, errno: %d", err, errno);
		if (EINVAL != errno) {
			_close(state->fd);
			return -errno;
		}
		if (strstr(ident->uri, "\\\\.\\PhysicalDrive")) {
			dev_stat.st_mode = 0x21B6;
		}
	}
	_close(state->fd);

	dw_creation_mode = xnvme_file_creation_opts_to_windows(opts);
	access_mode = xnvme_file_access_opts_to_windows(opts);
	file_attributes |= xnvme_file_attributes_opts_to_windows(opts);
	// Change {async,sync,admin} based on file unless one was explicitly requested
	switch (dev_stat.st_mode & S_IFMT) {
	case S_IFREG:
		state->sync_handle = CreateFile((LPCSTR)ident->uri, access_mode, share_mode, NULL,
						dw_creation_mode, file_attributes, NULL);
		if (state->sync_handle == INVALID_HANDLE_VALUE) {
			err = GetLastError();
			XNVME_DEBUG("FAILED: CreateFile(trgt: '%s') : state->sync_handle: '%p', "
				    "errno: %d",
				    ident->uri, state->sync_handle, err);
			return err;
		}

		state->async_handle =
			CreateFile((LPCSTR)ident->uri, access_mode, share_mode, NULL,
				   dw_creation_mode, FILE_FLAG_OVERLAPPED | file_attributes, NULL);
		if (state->async_handle == INVALID_HANDLE_VALUE) {
			err = GetLastError();
			XNVME_DEBUG("FAILED: CreateFile(trgt: '%s') : state->async_handle: '%p', "
				    "errno: %d",
				    ident->uri, state->async_handle, err);
			return err;
		}

		XNVME_DEBUG("INFO: open() : regular file");
		dev->ident.dtype = XNVME_DEV_TYPE_FS_FILE;
		dev->ident.csi = XNVME_SPEC_CSI_FS;
		dev->ident.nsid = 1;
		if (!opts->admin) {
			dev->be.admin = g_xnvme_be_windows_admin_fs;
		}
		if (!opts->sync) {
			dev->be.sync = g_xnvme_be_windows_sync_fs;
		}
		if (!opts->async) {
			dev->be.async = g_xnvme_be_windows_async_iocp;
		}
		break;

	case S_IFCHR:
		state->sync_handle = CreateFile((LPCSTR)ident->uri, access_mode, share_mode, NULL,
						OPEN_EXISTING, file_attributes, NULL);
		if (state->sync_handle == INVALID_HANDLE_VALUE) {
			err = GetLastError();
			XNVME_DEBUG("FAILED: CreateFile(trgt: '%s') : state->sync_handle: '%p', "
				    "errno: %d",
				    ident->uri, state->sync_handle, err);
			return err;
		}

		state->async_handle =
			CreateFile((LPCSTR)ident->uri, access_mode, share_mode, NULL,
				   OPEN_EXISTING, FILE_FLAG_OVERLAPPED | file_attributes, NULL);
		if (state->async_handle == INVALID_HANDLE_VALUE) {
			err = GetLastError();
			XNVME_DEBUG("FAILED: CreateFile(trgt: '%s') : state->async_handle: '%p', "
				    "errno: %d",
				    ident->uri, state->async_handle, err);
			return err;
		}

		XNVME_DEBUG("INFO: open() : block-device file (is a NVMe namespace)");
		dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
		dev->ident.csi = XNVME_SPEC_CSI_NVM;
		// Making the nsid 1 by default
		dev->ident.nsid = 1;
		dev_type = xnvme_be_windows_get_device_type(state->async_handle);
		if (!opts->admin) {
			if (dev_type == BusTypeNvme) {
				dev->be.admin = g_xnvme_be_windows_admin_nvme;
			} else if (dev_type == BusTypeScsi || dev_type == BusTypeSata) {
				dev->be.admin = g_xnvme_be_windows_admin_block;
			}
		}
		if (!opts->sync) {
			dev->be.sync = g_xnvme_be_windows_sync_nvme;
		}
		if (!opts->async) {
			dev->be.async = g_xnvme_be_windows_async_iocp;
		}
		break;

	default:
		XNVME_DEBUG("FAILED: open() : unsupported S_IFMT: %d", dev_stat.st_mode & S_IFMT);
		return -EINVAL;
	}

	XNVME_DEBUG("INFO: --- open() : OK ---");

	return 0;
}

/**
 * Destroy the given device handle in Windows
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 */
void
xnvme_be_windows_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	_be_windows_state_term((void *)dev->be.state);
	memset(&dev->be, 0, sizeof(dev->be));
}

#endif

struct xnvme_be_dev g_xnvme_be_dev_windows = {
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_windows_dev_open,
	.dev_close = xnvme_be_windows_dev_close,
	.id = "windows",
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
	.id = "nosys",
#endif
};
