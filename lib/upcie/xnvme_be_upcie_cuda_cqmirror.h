// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_UPCIE_CUDA_CQMIRROR_H
#define __INTERNAL_XNVME_BE_UPCIE_CUDA_CQMIRROR_H
#include <stddef.h>
#include <stdint.h>
#include <cuda.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Both queues zeroed, since the warp starts at entry zero expecting phase one; the host range
 * stays registered until the last detach
 */
int
xnvme_be_upcie_cuda_cqmirror_attach(CUcontext ctx, void *host_base, size_t host_nbytes,
				    const void *cq_gpu, void *cq_host, uint16_t depth);

/** Returns once the warp has let go of both queues */
void
xnvme_be_upcie_cuda_cqmirror_detach(int slot);

/** Every queue must have been detached */
void
xnvme_be_upcie_cuda_cqmirror_term(void);

int
xnvme_be_upcie_cuda_cqmirror_nactive(void);

#ifdef __cplusplus
}
#endif
#endif /* __INTERNAL_XNVME_BE_UPCIE_CUDA_CQMIRROR_H */
