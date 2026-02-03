// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <stddef.h>
#include <xnvme_platform.h>
#include <xnvme_be_registry.h>

struct xnvme_platform g_xnvme_platform_windows = {
	.name = "windows",
	.backends =
		(struct xnvme_be *[]){
			&xnvme_be_windows,
			&xnvme_be_ramdisk,
			NULL,
		},
	.dev_open = xnvme_platform_dev_open,
	.enumerate = xnvme_platform_enumerate,
};
