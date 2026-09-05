// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Backend wiring for `upcie-cuda`
 *
 * The admin, sync and async paths are the ones implemented for `upcie`; they
 * pick up this backend's device memory through xnvme_be_upcie_state.dmem,
 * assigned by dev_open. Only dev and mem are cuda-specific, and the queue
 * setup, which is `upcie`'s unless a queue asks for its CQ in GPU memory.
 */
#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#include <xnvme_be_upcie.h>
#include <xnvme_be_upcie_cuda.h>

struct xnvme_be_upcie_cuda_rte g_upcie_cuda_rte = {0};

struct xnvme_be_async g_xnvme_be_upcie_cuda_async = {
	.id = "upcie-cuda",
	.cmd_io = xnvme_be_upcie_async_cmd_io,
	.cmd_iov = xnvme_be_upcie_async_cmd_iov,
	.poke = xnvme_be_upcie_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_upcie_cuda_queue_init,
	.term = xnvme_be_upcie_cuda_queue_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
};

const struct xnvme_be_config g_xnvme_be_upcie_cuda = {
	.async = &g_xnvme_be_upcie_cuda_async,
	.sync = &g_xnvme_be_upcie_sync,
	.admin = &g_xnvme_be_upcie_admin,
	.dev = &g_xnvme_be_upcie_cuda_dev,
	.mem = &g_xnvme_be_upcie_cuda_mem,
	.attr =
		{
			.name = "upcie-cuda",
			.descr = "CUDA-based uPCIe userspace NVMe driver",
			.caps = XNVME_BE_CAP_NVME_PCIE | XNVME_BE_CAP_MPROC,
		},
};

#endif
