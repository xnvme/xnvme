// SPDX-FileCopyrightText: Simon A. F. Lund
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#ifdef XNVME_BE_DMAMEM_ENABLED
#include <xnvme_be_dmamem.h>

struct xnvme_be_dmamem_rte g_dmamem_rte = {0};
#endif

const struct xnvme_be_config g_xnvme_be_dmamem = {
#ifdef XNVME_BE_DMAMEM_ENABLED
	.async = &g_xnvme_be_dmamem_async,
	.sync = &g_xnvme_be_dmamem_sync,
	.admin = &g_xnvme_be_dmamem_admin,
	.dev = &g_xnvme_be_dmamem_dev,
	.mem = &g_xnvme_be_dmamem_mem,
#endif
	.attr =
		{
			.name = "dmamem",
			.descr = "uPCIe iommufd + dmamem userspace NVMe driver",
			.caps = XNVME_BE_CAP_NVME_PCIE,
		},
};
