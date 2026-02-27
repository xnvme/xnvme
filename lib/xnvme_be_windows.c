// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
#include <xnvme_be_cbi.h>
#include <windows.h>
#include <xnvme_be_windows.h>

#define STATUS_SUCCESS (0x00000000)

typedef NTSTATUS(WINAPI *RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

RTL_OSVERSIONINFOW
_be_windows_get_os_version(void)
{
	RTL_OSVERSIONINFOW rovi = {0};
	HMODULE module_handle = GetModuleHandleW(L"ntdll.dll");
	if (module_handle == NULL) {
		return rovi;
	}

	RtlGetVersionPtr fun_ptr =
		(RtlGetVersionPtr)GetProcAddress(module_handle, "RtlGetVersion");
	if (fun_ptr == NULL) {
		return rovi;
	}

	rovi.dwOSVersionInfoSize = sizeof(rovi);
	fun_ptr(&rovi);
	return rovi;
}

int
xnvme_be_windows_uapi_ver_fpr(FILE *stream, enum xnvme_pr opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	RTL_OSVERSIONINFOW info = _be_windows_get_os_version();
	wrtn += fprintf(stream,
			"WINDOWS;WINDOWS_VERSION_CODE-UAPI/%" PRIu64 ".%" PRIu64 " (%" PRIu64 ") ",
			info.dwMajorVersion, info.dwMinorVersion, info.dwBuildNumber);

	return wrtn;
}

int
xnvme_file_access_opts_to_windows(struct xnvme_opts *opts)
{
	int flags = 0;
	flags |= opts->rdonly ? GENERIC_READ : 0x0;
	flags |= opts->wronly ? GENERIC_WRITE : 0x0;
	flags |= opts->rdwr ? GENERIC_WRITE | GENERIC_READ : 0x0;
	return flags;
}

int
xnvme_file_creation_opts_to_windows(struct xnvme_opts *opts)
{
	int flags = 0;

	flags |= opts->create ? CREATE_ALWAYS : 0x0;
	flags |= opts->truncate ? TRUNCATE_EXISTING : 0x0;

	if (!flags) {
		flags = OPEN_EXISTING;
	}
	return flags;
}

/**
 * Convert the xnvme supplied flags to file attributes parameters
 *
 * @param oflags device/file oflag
 *
 * @return flag with dwCreationDisposition value is returned.
 */
int
xnvme_file_attributes_opts_to_windows(struct xnvme_opts *opts)
{
	int flags = 0;

	flags |= opts->direct ? FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH : 0x0;

	return flags;
}

const struct xnvme_be_config g_xnvme_be_windows_emu_nvme = {
	.async = &g_xnvme_be_cbi_async_emu,
	.sync = &g_xnvme_be_windows_sync_nvme,
	.admin = &g_xnvme_be_windows_admin_nvme,
	.dev = &g_xnvme_be_dev_windows,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "windows",
			.descr = "Emulated async with NVMe ioctl",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_windows_thrpool_nvme = {
	.async = &g_xnvme_be_cbi_async_thrpool,
	.sync = &g_xnvme_be_windows_sync_nvme,
	.admin = &g_xnvme_be_windows_admin_nvme,
	.dev = &g_xnvme_be_dev_windows,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "windows",
			.descr = "Thread pool with NVMe ioctl",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_windows_iocp_nvme = {
	.async = &g_xnvme_be_windows_async_iocp,
	.sync = &g_xnvme_be_windows_sync_nvme,
	.admin = &g_xnvme_be_windows_admin_nvme,
	.dev = &g_xnvme_be_dev_windows,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "windows",
			.descr = "IOCP with NVMe ioctl",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_windows_iocp_th_nvme = {
	.async = &g_xnvme_be_windows_async_iocp_th,
	.sync = &g_xnvme_be_windows_sync_nvme,
	.admin = &g_xnvme_be_windows_admin_nvme,
	.dev = &g_xnvme_be_dev_windows,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "windows",
			.descr = "IOCP threaded with NVMe ioctl",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_windows_ioring_nvme = {
	.async = &g_xnvme_be_windows_async_ioring,
	.sync = &g_xnvme_be_windows_sync_nvme,
	.admin = &g_xnvme_be_windows_admin_nvme,
	.dev = &g_xnvme_be_dev_windows,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "windows",
			.descr = "IoRing with NVMe ioctl",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

const struct xnvme_be_config g_xnvme_be_windows_nil_nvme = {
	.async = &g_xnvme_be_cbi_async_nil,
	.sync = &g_xnvme_be_windows_sync_nvme,
	.admin = &g_xnvme_be_windows_admin_nvme,
	.dev = &g_xnvme_be_dev_windows,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "windows",
			.descr = "Nil async with NVMe ioctl",
			.caps = XNVME_BE_CAP_BDEV,
		},
};

#ifdef XNVME_BE_WINDOWS_FS_ENABLED
const struct xnvme_be_config g_xnvme_be_windows_iocp_fs = {
	.async = &g_xnvme_be_windows_async_iocp,
	.sync = &g_xnvme_be_windows_sync_fs,
	.admin = &g_xnvme_be_windows_admin_fs,
	.dev = &g_xnvme_be_dev_windows,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "windows",
			.descr = "IOCP with file system APIs",
			.caps = XNVME_BE_CAP_FILE,
		},
};

const struct xnvme_be_config g_xnvme_be_windows_thrpool_fs = {
	.async = &g_xnvme_be_cbi_async_thrpool,
	.sync = &g_xnvme_be_windows_sync_fs,
	.admin = &g_xnvme_be_windows_admin_fs,
	.dev = &g_xnvme_be_dev_windows,
	.mem = &g_xnvme_be_windows_mem,
	.attr =
		{
			.name = "windows",
			.descr = "Thread pool with file system APIs",
			.caps = XNVME_BE_CAP_FILE,
		},
};
#endif

#endif
