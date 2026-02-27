// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifdef XNVME_BE_VFIO_ENABLED
#include <xnvme_be.h>
#include <xnvme_be_vfio.h>

const struct xnvme_be_config g_xnvme_be_vfio = {
	.async = &g_xnvme_be_vfio_async,
	.sync = &g_xnvme_be_vfio_sync,
	.admin = &g_xnvme_be_vfio_admin,
	.dev = &g_xnvme_be_vfio_dev,
	.mem = &g_xnvme_be_vfio_mem,
	.attr =
		{
			.name = "libvfn",
			.descr = "libvfn/VFIO userspace NVMe driver",
			.caps = XNVME_BE_CAP_NVME_PCIE,
		},
};

#endif
