// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_UPCIE_HIP_ENABLED
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_upcie_hip_gpuinit.h>
#endif

/**
 * GPU-initiated async engine (WORK IN PROGRESS).
 *
 * The device-resident submission path (a HIP kernel builds SQEs, rings the
 * bridged doorbell, and reaps completions from device code) can only be built
 * and validated on ROCm hardware, so it is not wired up here yet. The per-queue
 * lifecycle it will drive is already in place: xnvme_hip_gpuinit_queue_create
 * and _destroy set up the device-resident qpair plus its host doorbell/CQ
 * bridge and run the poller for the queue lifetime (see
 * xnvme_be_upcie_hip_gpuinit_gpu.c). Until the kernel-launch submit is attached
 * to init/cmd_io, the handlers report -ENOSYS so the backend registers and
 * links without claiming a working I/O path.
 */
struct xnvme_be_async g_xnvme_be_upcie_hip_gpuinit_async = {
	.id = "upcie-hip-gpuinit",
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
	.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd,
};
