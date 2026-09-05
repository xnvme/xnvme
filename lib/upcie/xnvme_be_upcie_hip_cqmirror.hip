// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <hip/hip_runtime.h>
#include <xnvme_be_upcie_hip_cqmirror.h>

typedef unsigned long long cqe_t __attribute__((ext_vector_type(2)));

__global__ static void
xnvme_be_upcie_hip_cqmirror_kernel(struct xnvme_be_upcie_hip_cqmirror_table *table)
{
	const uint32_t lane = __lane_id();
	const uint32_t wave = blockIdx.x * (blockDim.x / warpSize) + threadIdx.x / warpSize;
	volatile struct xnvme_be_upcie_hip_cqmirror_slot *slot = &table->slots[wave];
	const uint64_t full = (warpSize == 64) ? ~0ull : 0xffffffffull;

	while (!*(volatile int32_t *)&table->stop) {
		const uint32_t gen = slot->gen;
		const volatile cqe_t *cq_gpu;
		cqe_t *cq_host;
		uint32_t depth, head = 0, phase = 1, spins = 0;

		if (!(gen & 1)) {
			if (gen && !lane && slot->ack != gen - 1) {
				slot->ack = gen - 1;
			}
			for (int i = 0; i < 16; ++i) {
				__builtin_amdgcn_s_sleep(127);
			}
			continue;
		}
		cq_gpu = (const volatile cqe_t *)slot->cq_gpu;
		cq_host = (cqe_t *)slot->cq_host;
		depth = slot->depth;

		while (true) {
			uint32_t idx = head + lane, ph = phase, n;
			uint64_t mask;
			cqe_t v;

			if (idx >= depth) {
				idx -= depth;
				ph ^= 1;
			}
			v = cq_gpu[lane < depth ? idx : 0];
			mask = __ballot(lane < depth && ((v.y >> 48) & 1) == ph);
			n = (mask == full) ? warpSize : __builtin_ctzll(~mask);
			if (lane < n) {
				__builtin_nontemporal_store(v, &cq_host[idx]);
			}
			head += n;
			if (head >= depth) {
				head -= depth;
				phase ^= 1;
			}
			if (!n) {
				__builtin_amdgcn_s_sleep(16);
				if (!(++spins & 1023) && slot->gen != gen) {
					break;
				}
			}
		}
		/* The host frees the CQ once it sees the ack, so the copies land first */
		__threadfence_system();
		if (!lane) {
			slot->ack = gen;
		}
	}
}

extern "C" int
xnvme_be_upcie_hip_cqmirror_launch(struct xnvme_be_upcie_hip_cqmirror_table *table,
				   unsigned int nblocks, unsigned int nthreads, void *stream)
{
	void *args[] = {&table};

	return hipLaunchKernel((const void *)xnvme_be_upcie_hip_cqmirror_kernel, dim3(nblocks),
			       dim3(nthreads), args, 0, (hipStream_t)stream);
}
