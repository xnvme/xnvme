// Copyright (C) Rishabh Shukla <rishabh.sh@samsung.com>
// Copyright (C) Pranjal Dave <pranjal.58@partner.samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_WINDOWS_ENABLED
#include <windows.h>
#include <xnvme_be_windows.h>
#include <libxnvme_file.h>

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
	wrtn += fprintf(stream, "WINDOWS;WINDOWS_VERSION_CODE-UAPI/%lu.%lu (%lu) ",
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

static struct xnvme_be_mixin g_xnvme_be_mixin_windows[] = {
	{
		.mtype = XNVME_BE_MEM,
		.name = "windows",
		.descr = "Use Windows memory allocator",
		.mem = &g_xnvme_be_windows_mem,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ASYNC,
		.name = "emu",
		.descr = "Use emulated asynchronous I/O",
		.async = &g_xnvme_be_posix_async_emu,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ASYNC,
		.name = "thrpool",
		.descr = "Use thread pool for Asynchronous I/O",
		.async = &g_xnvme_be_posix_async_thrpool,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ASYNC,
		.name = "iocp",
		.descr = "Use Windows readfile/writefile with overlapped(iocp) for Asynchronous "
			 "I/O",
		.async = &g_xnvme_be_windows_async_iocp,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ASYNC,
		.name = "iocp_th",
		.descr = "Use Windows readfile/writefile with thread based overlapped(iocp) for "
			 "Asynchronous I/O",
		.async = &g_xnvme_be_windows_async_iocp_th,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ASYNC,
		.name = "io_ring",
		.descr = "Use Windows io_ring for Asynchronous I/O",
		.async = &g_xnvme_be_windows_async_ioring,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_SYNC,
		.name = "nvme",
		.descr = "Use Windows NVMe Driver ioctl() for synchronous I/O",
		.sync = &g_xnvme_be_windows_sync_nvme,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ADMIN,
		.name = "nvme",
		.descr = "Use Windows NVMe Driver ioctl() for admin commands",
		.admin = &g_xnvme_be_windows_admin_nvme,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ADMIN,
		.name = "block",
		.descr = "Use Windows NVMe Driver ioctl() for admin commands",
		.admin = &g_xnvme_be_windows_admin_block,
		.check_support = xnvme_be_supported,
	},

#ifdef XNVME_BE_WINDOWS_FS_ENABLED
	{
		.mtype = XNVME_BE_SYNC,
		.name = "file",
		.descr = "Use Windows File System APIs and readfile()/writefile() for I/O",
		.sync = &g_xnvme_be_windows_sync_fs,
		.check_support = xnvme_be_supported,
	},
	{
		.mtype = XNVME_BE_ADMIN,
		.name = "file",
		.descr = "Use Windows File System APIs for admin commands",
		.admin = &g_xnvme_be_windows_admin_fs,
		.check_support = xnvme_be_supported,
	},
#endif

	{
		.mtype = XNVME_BE_DEV,
		.name = "windows",
		.descr = "Use Windows file/dev handles and enumerate NVMe devices",
		.dev = &g_xnvme_be_dev_windows,
		.check_support = xnvme_be_supported,
	},
};
#endif

struct xnvme_be xnvme_be_windows = {
	.mem = XNVME_BE_NOSYS_MEM,
	.admin = XNVME_BE_NOSYS_ADMIN,
	.sync = XNVME_BE_NOSYS_SYNC,
	.async = XNVME_BE_NOSYS_QUEUE,
	.dev = XNVME_BE_NOSYS_DEV,
	.attr =
		{
			.name = "windows",
#ifdef XNVME_BE_WINDOWS_ENABLED
			.enabled = 1,
#endif
		},
#ifdef XNVME_BE_WINDOWS_ENABLED
	.nobjs = sizeof g_xnvme_be_mixin_windows / sizeof *g_xnvme_be_mixin_windows,
	.objs = g_xnvme_be_mixin_windows,
#endif
};
