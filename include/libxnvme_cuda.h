// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * GPU NVMe IO helpers
 *
 * Host-side queue management and device-side helpers for submitting NVMe
 * commands from CUDA kernels using xNVMe's NVMe spec types.
 *
 * @note This API is experimental and may change without notice.
 *
 * @headerfile libxnvme_cuda.h
 */

#ifndef __LIBXNVME_CUDA_H
#define __LIBXNVME_CUDA_H

#include <stdint.h>

#ifdef __CUDACC__
#include <errno.h>
#include <upcie/nvme/nvme_command.h>
#include <upcie/nvme/nvme_qpair_cuda.h>

/**
 * GPU-resident NVMe queue pair
 *
 * Instances live in CUDA device memory. Create one via xnvme_cuda_queue_create()
 * and pass the returned pointer as a kernel argument.
 *
 * With __CUDACC__ (nvcc compiling CUDA kernels):
 * the full definition is provided. This is required for xnvme_cuda_cmd_io().
 */
struct xnvme_cuda_queue {
	struct nvme_qpair_cuda qpair;
};
#else
/**
 * Opaque GPU-resident NVMe queue pair
 *
 * Instances live in CUDA device memory. Create one via xnvme_cuda_queue_create()
 * and pass the returned pointer as a kernel argument.
 *
 * Without __CUDACC__ (host C/C++ callers):
 * only a forward declaration is visible
 */
struct xnvme_cuda_queue;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a GPU-resident NVMe IO queue
 *
 * Allocates a queue pair in CUDA device memory and registers it with the NVMe
 * controller. The resulting pointer lives in device memory and can be passed
 * directly as a CUDA kernel argument.
 *
 * @param dev   An xnvme_dev opened on an ``upcie-cuda`` device
 * @param depth Number of IO slots in the queue
 * @param queue On success, set to a device pointer to the GPU queue
 *
 * @return 0 on success, negative error code on failure
 */
int
xnvme_cuda_queue_create(struct xnvme_dev *dev, uint16_t depth, struct xnvme_cuda_queue **queue);

/**
 * Destroy a GPU-resident NVMe IO queue
 *
 * Deletes the queue pair from the NVMe controller and frees the device memory.
 *
 * @param dev   The xnvme_dev used to create the queue
 * @param queue GPU queue pointer returned by xnvme_cuda_queue_create()
 */
void
xnvme_cuda_queue_destroy(struct xnvme_dev *dev, struct xnvme_cuda_queue *queue);

/**
 * Submit an NVMe command and reap its completion from a CUDA kernel
 *
 * Each active thread (tid < batch_size) submits one command and reaps one
 * completion. Inactive threads participate in the required barriers but do
 * not submit or reap.
 *
 * @param qp         GPU queue pair (from xnvme_cuda_queue_create())
 * @param cmd        Fully populated NVMe command
 * @param tid        Thread index within the block (threadIdx.x)
 * @param batch_size Number of IOs to submit; threads with tid >= batch_size
 *                   participate in barriers but do not submit or reap
 *
 * @return 0 on success. -EAGAIN on completion timeout. Positive NVMe status
 *         code on device error.
 */
#ifdef __CUDACC__
static inline __device__ int
xnvme_cuda_cmd_io(struct xnvme_cuda_queue *qp, struct xnvme_spec_cmd *cmd, size_t tid,
		  size_t batch_size)
{
	return nvme_qpair_cuda_io(&qp->qpair, (struct nvme_command *)cmd, tid, batch_size);
}
#endif /* __CUDACC__ */

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_CUDA_H */
