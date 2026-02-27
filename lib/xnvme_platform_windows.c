// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_platform.h>
#include <xnvme_be_registry.h>

#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
#include <errno.h>

static int
xnvme_platform_nosys_scan(const char *XNVME_UNUSED(sys_uri), struct xnvme_opts *XNVME_UNUSED(opts),
			  xnvme_scan_cb XNVME_UNUSED(cb_func), void *XNVME_UNUSED(cb_args))
{
	return -ENOSYS;
}

struct xnvme_platform g_xnvme_platform_windows = {
	.name = "windows",
	.backends =
		(struct xnvme_be *[]){
			&xnvme_be_windows,
			&xnvme_be_ramdisk,
			NULL,
		},
	.dev_open = xnvme_platform_dev_open,
	.scan = xnvme_platform_nosys_scan,
	.enumerate = xnvme_platform_enumerate,
};
#endif
