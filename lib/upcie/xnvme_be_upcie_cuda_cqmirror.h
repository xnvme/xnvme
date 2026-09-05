// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Completion queues in GPU memory, mirrored into host memory
 *
 * A drive completing into DRAM what it wrote into VRAM has its completion
 * held behind the data, since both are posted writes from one requester and
 * the root complex keeps them in order. With the CQ in VRAM the two travel
 * together, and a warp resident on the GPU copies each completion into the
 * host-side CQ the CPU polls, which is none the wiser.
 *
 * One kernel serves every such queue in the process: a warp per queue, each
 * watching a slot in a table the host fills and clears. It is launched with
 * the first queue and stopped with the last. Nothing here synchronises the
 * device, since that would wait for the kernel itself.
 *
 * This is the only CUDA device code in the library, hence the separate
 * translation unit; the rest of `upcie-cuda` is C against the driver API.
 */
#ifndef __INTERNAL_XNVME_BE_UPCIE_CUDA_CQMIRROR_H
#define __INTERNAL_XNVME_BE_UPCIE_CUDA_CQMIRROR_H
#include <stddef.h>
#include <stdint.h>
#include <cuda.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Have a warp keep cq_host a copy of cq_gpu
 *
 * cq_host must lie in host memory the GPU can be given; the range it is in
 * is registered with the driver the first time it is seen and stays so until
 * xnvme_be_upcie_cuda_cqmirror_term(). Both queues must be zeroed, since the
 * warp starts at entry zero expecting phase one.
 *
 * @param ctx The context the GPU memory belongs to
 * @param host_base Start of the host region cq_host lies in
 * @param host_nbytes Size of that region
 * @param cq_gpu The CQ the controller completes into
 * @param cq_host The CQ the CPU reads
 * @param depth Entries in the CQ
 *
 * @return The slot the queue was given, or negative errno
 */
int
xnvme_be_upcie_cuda_cqmirror_attach(CUcontext ctx, void *host_base, size_t host_nbytes,
				    const void *cq_gpu, void *cq_host, uint16_t depth);

/**
 * Take a queue back from its warp
 *
 * Returns once the warp has let go of both queues, so they can be deleted
 * and their memory reused.
 *
 * @param slot What xnvme_be_upcie_cuda_cqmirror_attach() returned
 */
void
xnvme_be_upcie_cuda_cqmirror_detach(int slot);

/**
 * Stop the kernel if it is up and drop the host registrations
 *
 * For the runtime's teardown; every queue must have been detached.
 */
void
xnvme_be_upcie_cuda_cqmirror_term(void);

#ifdef __cplusplus
}
#endif
#endif /* __INTERNAL_XNVME_BE_UPCIE_CUDA_CQMIRROR_H */
