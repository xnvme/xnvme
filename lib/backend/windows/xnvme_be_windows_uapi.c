// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
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
#endif
