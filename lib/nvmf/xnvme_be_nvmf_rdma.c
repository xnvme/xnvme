// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <string.h>

#include <libxnvme.h>
#include <xnvme_be.h>

#ifdef XNVME_BE_NVMF_ENABLED
#include <xnvme_be_nvmf.h>
#include <xnvme_be_nvmf_rdma.h>

static struct xnvme_be_nvmf_transport_ops g_xnvme_be_nvmf_rdma_transport_ops = {
	.create_ctrlr = xnvme_be_nvmf_create_rdma_controller,
};

struct xnvme_be_nvmf_transport g_xnvme_be_nvmf_rdma_transport = {
	.name = "rdma",
	.ops =
		{
			.create_ctrlr = xnvme_be_nvmf_create_rdma_controller,
		},
};

#endif // XNVME_BE_NVMF_ENABLED
