// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <cuda_runtime.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <libxnvme.h>

extern "C" {
#include "xnvmeperf.h"
}

/**
 * One CUDA block per queue; each thread owns one queue slot and advances
 * sequentially by qdepth * nlbas per round, wrapping at the device boundary.
 */
__global__ static void
xnvmeperf_cuda_kernel_seq(struct xnvme_cuda_queue **qps, struct xnvme_spec_cmd *cmds,
			  uint64_t *nblocks, uint16_t nlbas, volatile int *stop,
			  uint64_t *out_rounds, uint64_t *out_failed)
{
	struct xnvme_spec_cmd cmd;
	uint64_t cap, offset, rounds = 0, failed = 0;
	__shared__ int s_stop;
	int err;

	const size_t bid = blockIdx.x;
	const size_t tid = threadIdx.x;
	const size_t qdepth = blockDim.x;

	cap = nblocks[bid];
	cmd = cmds[bid * qdepth + tid];
	offset = (uint64_t)tid * nlbas;

	while (true) {
		/* Thread 0 samples the stop flag and broadcasts via shared memory so
		 * all threads in the block make the same exit decision.  Without this,
		 * warps that observe *stop == 1 would skip xnvme_cuda_cmd_io while
		 * others still enter it, causing the __syncthreads() inside
		 * xnvme_cuda_cmd_io to deadlock. */
		if (tid == 0) {
			s_stop = *stop;
		}

		__syncthreads();
		if (s_stop) {
			break;
		}

		cmd.nvm.slba = offset;

		offset += (uint64_t)qdepth * nlbas;
		if (offset >= cap * (uint64_t)nlbas) {
			offset = (uint64_t)tid * nlbas;
		}

		err = xnvme_cuda_cmd_io(qps[bid], &cmd, tid, qdepth);

		if (tid == 0) {
			rounds++;
		}
		if (err) {
			failed++;
		}
	}

	if (tid == 0) {
		out_rounds[bid] = rounds;
	}
	atomicAdd((unsigned long long *)&out_failed[bid], (unsigned long long)failed);
}

/**
 * One CUDA block per queue; each thread owns one queue slot and picks a random
 * LBA each round using an independent per-thread LCG seeded from seeds.
 */
__global__ static void
xnvmeperf_cuda_kernel_rand(struct xnvme_cuda_queue **qps, struct xnvme_spec_cmd *cmds,
			   uint64_t *nblocks, uint16_t nlbas, uint64_t *seeds, volatile int *stop,
			   uint64_t *out_rounds, uint64_t *out_failed)
{
	struct xnvme_spec_cmd cmd;
	uint64_t cap, slba, seed, rounds = 0, failed = 0;
	__shared__ int s_stop;
	int err;

	const size_t bid = blockIdx.x;
	const size_t tid = threadIdx.x;
	const size_t qdepth = blockDim.x;

	cap = nblocks[bid];
	cmd = cmds[bid * qdepth + tid];
	seed = seeds[bid * qdepth + tid];

	while (true) {
		if (tid == 0) {
			s_stop = *stop;
		}

		__syncthreads();
		if (s_stop) {
			break;
		}

		seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
		slba = ((seed >> 33) % cap) * (uint64_t)nlbas;

		cmd.nvm.slba = slba;

		err = xnvme_cuda_cmd_io(qps[bid], &cmd, tid, qdepth);

		if (tid == 0) {
			rounds++;
		}
		if (err) {
			failed++;
		}
	}

	if (tid == 0) {
		out_rounds[bid] = rounds;
	}
	atomicAdd((unsigned long long *)&out_failed[bid], (unsigned long long)failed);
}

static cudaError_t
cuda_sync_check(void)
{
	cudaError_t cerr = cudaGetLastError();
	if (cerr) {
		fprintf(stderr, "Failed: cudaGetLastError(): %s\n", cudaGetErrorString(cerr));
		return cerr;
	}
	cerr = cudaDeviceSynchronize();
	if (cerr) {
		fprintf(stderr, "Failed: cudaDeviceSynchronize(): %s\n", cudaGetErrorString(cerr));
	}
	return cerr;
}

static cudaError_t
cuda_upload(void **d_ptr, const void *h_ptr, size_t nbytes)
{
	cudaError_t cerr;

	cerr = cudaMalloc(d_ptr, nbytes);
	if (cerr) {
		fprintf(stderr, "Failed: cudaMalloc(): %s\n", cudaGetErrorString(cerr));
		return cerr;
	}

	cerr = cudaMemcpy(*d_ptr, h_ptr, nbytes, cudaMemcpyHostToDevice);
	if (cerr) {
		fprintf(stderr, "Failed: cudaMemcpy(): %s\n", cudaGetErrorString(cerr));
	}
	return cerr;
}

/**
 * Allocates qdepth DMA buffers for one queue; for iosize spanning more than two
 * pages an additional PRP-list page is allocated per slot.
 */
static int
xnvmeperf_cuda_alloc_bufs(struct xnvme_dev *dev, uint32_t iosize, uint32_t qdepth,
			  void ***out_bufs, void ***out_prp_bufs)
{
	void **bufs, **prp_bufs = NULL;
	uint64_t npages;
	uint32_t pagesize;
	int err = 0;

	pagesize = (uint32_t)getpagesize();
	npages = ((uint64_t)iosize + pagesize - 1) / pagesize;

	bufs = (void **)calloc(qdepth, sizeof(*bufs));
	if (!bufs) {
		xnvme_cli_perr("Failed: calloc()", -ENOMEM);
		return -ENOMEM;
	}

	if (npages > 2) {
		prp_bufs = (void **)calloc(qdepth, sizeof(*prp_bufs));
		if (!prp_bufs) {
			xnvme_cli_perr("Failed: calloc()", -ENOMEM);
			free(bufs);
			return -ENOMEM;
		}
	}

	for (uint32_t i = 0; i < qdepth; i++) {
		bufs[i] = xnvme_buf_alloc(dev, iosize);
		if (!bufs[i]) {
			err = -ENOMEM;
			xnvme_cli_perr("Failed: xnvme_buf_alloc()", err);
			goto err_free_allocs;
		}
		if (npages > 2) {
			prp_bufs[i] = xnvme_buf_alloc(dev, pagesize);
			if (!prp_bufs[i]) {
				err = -ENOMEM;
				xnvme_cli_perr("Failed: xnvme_buf_alloc()", err);
				goto err_free_allocs;
			}
		}
	}

	*out_bufs = bufs;
	*out_prp_bufs = prp_bufs;
	return 0;

err_free_allocs:
	for (uint32_t i = 0; i < qdepth; i++) {
		if (bufs[i]) {
			xnvme_buf_free(dev, bufs[i]);
		}
		if (prp_bufs && prp_bufs[i]) {
			xnvme_buf_free(dev, prp_bufs[i]);
		}
	}
	free(prp_bufs);
	free(bufs);
	return err;
}

/**
 * Resolves physical buffer addresses and sets PRP1/PRP2 (1-page and 2-page
 * cases) or builds a PRP list in prp_bufs and points PRP2 at it (>2 pages).
 */
static int
xnvmeperf_cuda_fill_cmds(struct xnvme_dev *dev, uint32_t iosize, uint32_t qdepth, uint8_t opcode,
			 uint32_t nsid, uint16_t nlb, void **bufs, void **prp_bufs,
			 struct xnvme_spec_cmd *h_cmds)
{
	uint64_t prp1, prp2, prp_list_phys, page_phys, npages, *prp_list = NULL;
	uint32_t pagesize;
	cudaError_t cerr;
	int err = 0;

	pagesize = (uint32_t)getpagesize();
	npages = ((uint64_t)iosize + pagesize - 1) / pagesize;

	if (npages > 2) {
		/* PRP-list buffer is one page; reject iosize whose list overflows it. */
		if ((npages - 1) * sizeof(uint64_t) > pagesize) {
			fprintf(stderr,
				"Error: iosize %u requires a PRP list larger than one page\n",
				iosize);
			return -EINVAL;
		}
		prp_list = (uint64_t *)malloc((npages - 1) * sizeof(*prp_list));
		if (!prp_list) {
			xnvme_cli_perr("Failed: malloc()", -ENOMEM);
			return -ENOMEM;
		}
	}

	for (uint32_t i = 0; i < qdepth; i++) {
		err = xnvme_buf_vtophys(dev, bufs[i], &prp1);
		if (err) {
			xnvme_cli_perr("Failed: xnvme_buf_vtophys()", err);
			goto done;
		}

		h_cmds[i].common.opcode = opcode;
		h_cmds[i].common.nsid = nsid;
		h_cmds[i].nvm.nlb = nlb;
		h_cmds[i].common.dptr.prp.prp1 = prp1;

		if (npages == 2) {
			err = xnvme_buf_vtophys(dev, (char *)bufs[i] + pagesize, &prp2);
			if (err) {
				xnvme_cli_perr("Failed: xnvme_buf_vtophys()", err);
				goto done;
			}
			h_cmds[i].common.dptr.prp.prp2 = prp2;
		} else if (npages > 2) {
			for (uint64_t p = 1; p < npages; p++) {
				err = xnvme_buf_vtophys(dev, (char *)bufs[i] + p * pagesize,
							&page_phys);
				if (err) {
					xnvme_cli_perr("Failed: xnvme_buf_vtophys()", err);
					goto done;
				}
				prp_list[p - 1] = page_phys;
			}

			err = xnvme_buf_vtophys(dev, prp_bufs[i], &prp_list_phys);
			if (err) {
				xnvme_cli_perr("Failed: xnvme_buf_vtophys()", err);
				goto done;
			}

			cerr = cudaMemcpy(prp_bufs[i], prp_list, (npages - 1) * sizeof(*prp_list),
					  cudaMemcpyHostToDevice);
			if (cerr) {
				fprintf(stderr, "Failed: cudaMemcpy(): %s\n",
					cudaGetErrorString(cerr));
				err = (int)cerr;
				goto done;
			}

			h_cmds[i].common.dptr.prp.prp2 = prp_list_phys;
		}
	}

done:
	free(prp_list);
	return err;
}

static int
xnvmeperf_cuda_build_cmds(struct xnvme_dev **devs, int ndevs, uint32_t iosize, uint32_t qdepth,
			  uint8_t opcode, uint32_t nqueues, void ***bufs, void ***prp_bufs,
			  struct xnvme_spec_cmd *h_cmds)
{
	struct xnvme_dev *dev;
	uint32_t nsid, qi;
	uint16_t nlb;
	int err;

	for (int d = 0; d < ndevs; d++) {
		dev = devs[d];
		nsid = xnvme_dev_get_nsid(dev);
		nlb = (uint16_t)(iosize / xnvme_dev_get_geo(dev)->lba_nbytes) - 1;

		for (uint32_t q = 0; q < nqueues; q++) {
			qi = (uint32_t)d * nqueues + q;

			err = xnvmeperf_cuda_fill_cmds(dev, iosize, qdepth, opcode, nsid, nlb,
						       bufs[qi], prp_bufs[qi],
						       h_cmds + qi * qdepth);
			if (err) {
				xnvme_cli_perr("Failed: xnvmeperf_cuda_fill_cmds()", err);
				return err;
			}
		}
	}
	return 0;
}

/**
 * Creates queues and allocates I/O buffers for all devices. On partial failure
 * the successfully created resources are left in place for the caller to tear
 * down via xnvmeperf_cuda_cleanup().
 */
static int
xnvmeperf_cuda_setup(struct xnvme_dev **devs, int ndevs, uint32_t iosize, uint32_t qdepth,
		     uint32_t nqueues, struct xnvme_cuda_queue **h_qps, void ***bufs,
		     void ***prp_bufs, uint64_t *h_nblocks)
{
	struct xnvme_dev *dev;
	uint64_t nblocks;
	uint32_t qi;
	int err;

	for (int d = 0; d < ndevs; d++) {
		dev = devs[d];
		nblocks = xnvme_dev_get_geo(dev)->tbytes / iosize;
		if ((uint64_t)nqueues * qdepth > nblocks) {
			fprintf(stderr, "Error: device %d has %lu IO slots but need %u\n", d,
				(unsigned long)nblocks, nqueues * qdepth);
			return -EINVAL;
		}

		for (uint32_t q = 0; q < nqueues; q++) {
			qi = (uint32_t)d * nqueues + q;

			if (h_nblocks) {
				h_nblocks[qi] = nblocks;
			}

			err = xnvme_cuda_queue_create(dev, qdepth, &h_qps[qi]);
			if (err) {
				xnvme_cli_perr("Failed: xnvme_cuda_queue_create()", err);
				return err;
			}

			err = xnvmeperf_cuda_alloc_bufs(dev, iosize, qdepth, &bufs[qi],
							&prp_bufs[qi]);
			if (err) {
				xnvme_cli_perr("Failed: xnvmeperf_cuda_alloc_bufs()", err);
				return err;
			}
		}
	}
	return 0;
}

/* Safe to call with NULL per-slot entries; designed for partial-setup teardown. */
static void
xnvmeperf_cuda_cleanup(struct xnvme_dev **devs, int ndevs, uint32_t nqueues, uint32_t qdepth,
		       struct xnvme_cuda_queue **h_qps, void ***bufs, void ***prp_bufs)
{
	struct xnvme_dev *dev;
	uint32_t qi;

	for (int d = 0; d < ndevs; d++) {
		dev = devs[d];

		for (uint32_t q = 0; q < nqueues; q++) {
			qi = (uint32_t)d * nqueues + q;

			if (bufs && bufs[qi]) {
				for (uint32_t i = 0; i < qdepth; i++) {
					if (bufs[qi][i]) {
						xnvme_buf_free(dev, bufs[qi][i]);
					}
					if (prp_bufs && prp_bufs[qi] && prp_bufs[qi][i]) {
						xnvme_buf_free(dev, prp_bufs[qi][i]);
					}
				}
				free(bufs[qi]);
			}
			if (prp_bufs && prp_bufs[qi]) {
				free(prp_bufs[qi]);
			}
			if (h_qps && h_qps[qi]) {
				xnvme_cuda_queue_destroy(dev, h_qps[qi]);
			}
		}
	}
}

/**
 * Launches one CUDA block per queue and runs for runtime_secs. Selects the
 * random kernel when h_seeds is non-NULL, sequential otherwise.
 */
static int
xnvmeperf_cuda_launch(struct xnvme_cuda_queue **h_qps, struct xnvme_spec_cmd *h_cmds,
		      uint64_t *h_seeds, uint32_t nqueues, uint64_t *h_nblocks, uint16_t nlbas,
		      uint32_t runtime_secs, unsigned int qdepth, uint64_t *h_rounds,
		      uint64_t *h_failed, float *elapsed_ms)
{
	struct xnvme_cuda_queue **d_qps = NULL;
	struct xnvme_spec_cmd *d_cmds = NULL;
	uint64_t *d_seeds = NULL, *d_nblocks = NULL, *d_rounds = NULL, *d_failed = NULL;
	void *d_stop;
	int *h_stop = NULL;
	cudaEvent_t t0 = NULL, t1 = NULL;
	cudaError_t cerr;

	cerr = cuda_upload((void **)&d_qps, h_qps, nqueues * sizeof(*d_qps));
	if (cerr) {
		goto done;
	}

	cerr = cuda_upload((void **)&d_cmds, h_cmds, nqueues * qdepth * sizeof(*d_cmds));
	if (cerr) {
		goto done;
	}

	cerr = cuda_upload((void **)&d_nblocks, h_nblocks, nqueues * sizeof(*d_nblocks));
	if (cerr) {
		goto done;
	}

	cerr = cudaMalloc((void **)&d_rounds, nqueues * sizeof(*d_rounds));
	if (cerr) {
		fprintf(stderr, "Failed: cudaMalloc(): %s\n", cudaGetErrorString(cerr));
		goto done;
	}

	cerr = cudaMalloc((void **)&d_failed, nqueues * sizeof(*d_failed));
	if (cerr) {
		fprintf(stderr, "Failed: cudaMalloc(): %s\n", cudaGetErrorString(cerr));
		goto done;
	}
	cerr = cudaMemset(d_failed, 0, nqueues * sizeof(*d_failed));
	if (cerr) {
		fprintf(stderr, "Failed: cudaMemset(): %s\n", cudaGetErrorString(cerr));
		goto done;
	}

	cerr = cudaHostAlloc((void **)&h_stop, sizeof(*h_stop), cudaHostAllocMapped);
	if (cerr) {
		fprintf(stderr, "Failed: cudaHostAlloc(): %s\n", cudaGetErrorString(cerr));
		goto done;
	}

	cerr = cudaHostGetDevicePointer(&d_stop, h_stop, 0);
	if (cerr) {
		fprintf(stderr, "Failed: cudaHostGetDevicePointer(): %s\n",
			cudaGetErrorString(cerr));
		goto done;
	}

	if (h_seeds) {
		cerr = cuda_upload((void **)&d_seeds, h_seeds,
				   nqueues * qdepth * sizeof(*d_seeds));
		if (cerr) {
			goto done;
		}
	}

	*h_stop = 0;

	cerr = cudaEventCreate(&t0);
	if (cerr) {
		fprintf(stderr, "Failed: cudaEventCreate(): %s\n", cudaGetErrorString(cerr));
		goto done;
	}
	cerr = cudaEventCreate(&t1);
	if (cerr) {
		fprintf(stderr, "Failed: cudaEventCreate(): %s\n", cudaGetErrorString(cerr));
		goto done;
	}
	cerr = cudaEventRecord(t0);
	if (cerr) {
		fprintf(stderr, "Failed: cudaEventRecord(): %s\n", cudaGetErrorString(cerr));
		goto done;
	}

	if (h_seeds) {
		xnvmeperf_cuda_kernel_rand<<<nqueues, qdepth>>>(d_qps, d_cmds, d_nblocks, nlbas,
								d_seeds, (volatile int *)d_stop,
								d_rounds, d_failed);
	} else {
		xnvmeperf_cuda_kernel_seq<<<nqueues, qdepth>>>(d_qps, d_cmds, d_nblocks, nlbas,
							       (volatile int *)d_stop, d_rounds,
							       d_failed);
	}

	sleep(runtime_secs);
	*h_stop = 1;

	cerr = cudaEventRecord(t1);
	if (cerr) {
		fprintf(stderr, "Failed: cudaEventRecord(): %s\n", cudaGetErrorString(cerr));
		goto done;
	}
	cerr = cuda_sync_check();
	if (!cerr) {
		cudaEventElapsedTime(elapsed_ms, t0, t1);
		cerr = cudaMemcpy(h_rounds, d_rounds, nqueues * sizeof(*h_rounds),
				  cudaMemcpyDeviceToHost);
		if (cerr) {
			fprintf(stderr, "Failed: cudaMemcpy(): %s\n", cudaGetErrorString(cerr));
			goto done;
		}
		cerr = cudaMemcpy(h_failed, d_failed, nqueues * sizeof(*h_failed),
				  cudaMemcpyDeviceToHost);
		if (cerr) {
			fprintf(stderr, "Failed: cudaMemcpy(): %s\n", cudaGetErrorString(cerr));
			goto done;
		}
	}

done:
	if (t0) {
		cudaEventDestroy(t0);
	}
	if (t1) {
		cudaEventDestroy(t1);
	}
	cudaFree(d_seeds);
	cudaFreeHost(h_stop);
	cudaFree(d_failed);
	cudaFree(d_rounds);
	cudaFree(d_nblocks);
	cudaFree(d_cmds);
	cudaFree(d_qps);
	return (int)cerr;
}

static int
xnvmeperf_cuda_validate_lba(struct xnvme_dev **devs, const struct xnvmeperf_args *args)
{
	for (int d = 1; d < args->ndevs; d++) {
		if (xnvme_dev_get_geo(devs[d])->lba_nbytes !=
		    xnvme_dev_get_geo(devs[0])->lba_nbytes) {
			fprintf(stderr, "Error: device %s LBA size mismatch\n", args->dev_uris[d]);
			return -EINVAL;
		}
	}

	if (args->iosize % xnvme_dev_get_geo(devs[0])->lba_nbytes) {
		fprintf(stderr, "Error: iosize %u is not a multiple of LBA size %u\n",
			args->iosize, xnvme_dev_get_geo(devs[0])->lba_nbytes);
		return -EINVAL;
	}

	return 0;
}

extern "C" int
xnvmeperf_cuda_run_io(struct xnvme_dev **devs, const struct xnvmeperf_args *args,
		      uint64_t *rounds_per_dev, uint64_t *failed_per_dev, float *elapsed_ms)
{
	struct xnvme_cuda_queue **h_qps;
	struct xnvme_spec_cmd *h_cmds;
	void ***bufs, ***prp_bufs;
	uint64_t *nblocks, *rounds, *failed, *h_seeds = NULL;
	uint32_t total_queues;
	uint16_t nlbas;
	uint8_t opcode;
	int random = 0, err = 0;

	switch (args->pattern) {
	case IOPATTERN_READ:
		opcode = XNVME_SPEC_NVM_OPC_READ;
		break;
	case IOPATTERN_WRITE:
		opcode = XNVME_SPEC_NVM_OPC_WRITE;
		break;
	case IOPATTERN_RANDREAD:
		opcode = XNVME_SPEC_NVM_OPC_READ;
		random = 1;
		break;
	case IOPATTERN_RANDWRITE:
		opcode = XNVME_SPEC_NVM_OPC_WRITE;
		random = 1;
		break;
	default:
		err = -EINVAL;
		xnvme_cli_perr("Error: unsupported pattern", err);
		return err;
	}

	err = xnvmeperf_cuda_validate_lba(devs, args);
	if (err) {
		return err;
	}

	nlbas = (uint16_t)(args->iosize / xnvme_dev_get_geo(devs[0])->lba_nbytes);

	total_queues = (uint32_t)args->ndevs * args->nqueues;
	/* Flat arrays; device d owns slots [d*nqueues .. (d+1)*nqueues). */
	h_qps = (struct xnvme_cuda_queue **)calloc(total_queues, sizeof(*h_qps));
	h_cmds = (struct xnvme_spec_cmd *)calloc(total_queues * args->qdepth, sizeof(*h_cmds));
	bufs = (void ***)calloc(total_queues, sizeof(*bufs));
	prp_bufs = (void ***)calloc(total_queues, sizeof(*prp_bufs));
	nblocks = (uint64_t *)calloc(total_queues, sizeof(*nblocks));
	rounds = (uint64_t *)calloc(total_queues, sizeof(*rounds));
	failed = (uint64_t *)calloc(total_queues, sizeof(*failed));

	if (!h_qps || !h_cmds || !nblocks || !bufs || !prp_bufs || !rounds || !failed) {
		err = -ENOMEM;
		xnvme_cli_perr("Failed: calloc()", err);
		goto cleanup;
	}

	err = xnvmeperf_cuda_setup(devs, args->ndevs, args->iosize, args->qdepth, args->nqueues,
				   h_qps, bufs, prp_bufs, nblocks);
	if (err) {
		xnvme_cli_perr("Failed: xnvmeperf_cuda_setup()", err);
		goto cleanup;
	}

	err = xnvmeperf_cuda_build_cmds(devs, args->ndevs, args->iosize, args->qdepth, opcode,
					args->nqueues, bufs, prp_bufs, h_cmds);
	if (err) {
		xnvme_cli_perr("Failed: xnvmeperf_cuda_build_cmds()", err);
		goto cleanup;
	}

	if (random) {
		h_seeds = (uint64_t *)malloc(total_queues * args->qdepth * sizeof(*h_seeds));
		if (!h_seeds) {
			err = -ENOMEM;
			xnvme_cli_perr("Failed: malloc()", err);
			goto cleanup;
		}

		for (uint32_t i = 0; i < total_queues * args->qdepth; i++) {
			h_seeds[i] = 0x12345678ULL + i * 0x9e3779b97f4a7c15ULL;
		}
	}

	err = xnvmeperf_cuda_launch(h_qps, h_cmds, h_seeds, total_queues, nblocks, nlbas,
				    args->time, args->qdepth, rounds, failed, elapsed_ms);

	if (!err) {
		for (int d = 0; d < args->ndevs; d++) {
			rounds_per_dev[d] = 0;
			failed_per_dev[d] = 0;
			for (uint32_t q = 0; q < args->nqueues; q++) {
				rounds_per_dev[d] += rounds[(uint32_t)d * args->nqueues + q];
				failed_per_dev[d] += failed[(uint32_t)d * args->nqueues + q];
			}
		}
	}

cleanup:
	free(h_seeds);
	free(h_cmds);
	xnvmeperf_cuda_cleanup(devs, args->ndevs, args->nqueues, args->qdepth, h_qps, bufs,
			       prp_bufs);
	free(bufs);
	free(prp_bufs);
	free(nblocks);
	free(rounds);
	free(failed);
	free(h_qps);
	return err;
}

/**
 * Issues one NVMe I/O per thread. All threads in a block must enter together
 * because xnvme_cuda_cmd_io() contains a __syncthreads() internally.
 */
__global__ static void
xnvmeperf_cuda_kernel_verify_round(struct xnvme_cuda_queue **qps, struct xnvme_spec_cmd *cmds,
				   int *out_err)
{
	const size_t bid = blockIdx.x;
	const size_t tid = threadIdx.x;
	const size_t qdepth = blockDim.x;
	struct xnvme_spec_cmd cmd = cmds[bid * qdepth + tid];

	out_err[bid * qdepth + tid] = xnvme_cuda_cmd_io(qps[bid], &cmd, tid, qdepth);
}

static int
verify_dispatch(struct xnvme_cuda_queue **d_qps, struct xnvme_spec_cmd *h_cmds,
		struct xnvme_spec_cmd *d_cmds, int *d_errs, int *h_errs, uint32_t nqueues,
		uint32_t qdepth, const char *phase)
{
	uint32_t total = nqueues * qdepth;
	cudaError_t cerr;

	cerr = cudaMemcpy(d_cmds, h_cmds, total * sizeof(*d_cmds), cudaMemcpyHostToDevice);
	if (cerr) {
		fprintf(stderr, "Failed: cudaMemcpy(): %s\n", cudaGetErrorString(cerr));
		return (int)cerr;
	}

	xnvmeperf_cuda_kernel_verify_round<<<nqueues, qdepth>>>(d_qps, d_cmds, d_errs);
	cerr = cuda_sync_check();
	if (cerr) {
		return (int)cerr;
	}

	cerr = cudaMemcpy(h_errs, d_errs, total * sizeof(*h_errs), cudaMemcpyDeviceToHost);
	if (cerr) {
		fprintf(stderr, "Failed: cudaMemcpy(): %s\n", cudaGetErrorString(cerr));
		return (int)cerr;
	}

	for (uint32_t i = 0; i < total; i++) {
		if (h_errs[i]) {
			fprintf(stderr, "%s failure at slot %u: err(%d)\n", phase, i, h_errs[i]);
			return h_errs[i];
		}
	}

	return 0;
}

extern "C" int
xnvmeperf_cuda_verify_io(struct xnvme_dev **devs, const struct xnvmeperf_args *args)
{
	struct xnvme_cuda_queue **h_qps, **d_qps = NULL;
	struct xnvme_spec_cmd *h_cmds, *d_cmds = NULL;
	void ***bufs, ***prp_bufs;
	void *cmp_buf;
	uint32_t threads_per_dev, total_threads, total_queues;
	uint16_t nlbas;
	int *h_errs, *d_errs = NULL;
	cudaError_t cerr;
	int err = 0;

	err = xnvmeperf_cuda_validate_lba(devs, args);
	if (err) {
		return err;
	}

	nlbas = (uint16_t)(args->iosize / xnvme_dev_get_geo(devs[0])->lba_nbytes);
	threads_per_dev = args->nqueues * args->qdepth;
	total_threads = (uint32_t)args->ndevs * threads_per_dev;
	total_queues = (uint32_t)args->ndevs * args->nqueues;

	h_qps = (struct xnvme_cuda_queue **)calloc(total_queues, sizeof(*h_qps));
	h_cmds = (struct xnvme_spec_cmd *)calloc(total_threads, sizeof(*h_cmds));
	bufs = (void ***)calloc(total_queues, sizeof(*bufs));
	prp_bufs = (void ***)calloc(total_queues, sizeof(*prp_bufs));
	h_errs = (int *)malloc(total_threads * sizeof(*h_errs));
	cmp_buf = malloc(args->iosize);

	if (!h_qps || !h_cmds || !bufs || !prp_bufs || !h_errs || !cmp_buf) {
		err = -ENOMEM;
		xnvme_cli_perr("Failed: calloc()", err);
		goto cleanup;
	}

	err = xnvmeperf_cuda_setup(devs, args->ndevs, args->iosize, args->qdepth, args->nqueues,
				   h_qps, bufs, prp_bufs, NULL);
	if (err) {
		xnvme_cli_perr("Failed: xnvmeperf_cuda_setup()", err);
		goto cleanup;
	}

	err = xnvmeperf_cuda_build_cmds(devs, args->ndevs, args->iosize, args->qdepth,
					XNVME_SPEC_NVM_OPC_WRITE, args->nqueues, bufs, prp_bufs,
					h_cmds);
	if (err) {
		xnvme_cli_perr("Failed: xnvmeperf_cuda_build_cmds()", err);
		goto cleanup;
	}

	cerr = cuda_upload((void **)&d_qps, h_qps, total_queues * sizeof(*d_qps));
	if (cerr) {
		err = (int)cerr;
		goto cleanup;
	}

	cerr = cudaMalloc(&d_cmds, total_threads * sizeof(*d_cmds));
	if (cerr) {
		err = (int)cerr;
		fprintf(stderr, "Failed: cudaMalloc(): %s\n", cudaGetErrorString(cerr));
		goto cleanup;
	}

	cerr = cudaMalloc(&d_errs, total_threads * sizeof(*d_errs));
	if (cerr) {
		err = (int)cerr;
		fprintf(stderr, "Failed: cudaMalloc(): %s\n", cudaGetErrorString(cerr));
		goto cleanup;
	}

	/* Write: stamp each thread's LBA range with a unique pattern */
	for (uint32_t idx = 0; idx < total_threads; idx++) {
		h_cmds[idx].nvm.slba = (uint64_t)idx * nlbas;
		err = fill_pattern(bufs[idx / args->qdepth][idx % args->qdepth], args->iosize,
				   h_cmds[idx].nvm.slba, nlbas);
		if (err) {
			xnvme_cli_perr("Failed: fill_pattern()", err);
			goto cleanup;
		}
	}

	err = verify_dispatch(d_qps, h_cmds, d_cmds, d_errs, h_errs, total_queues, args->qdepth,
			      "write");
	if (err) {
		xnvme_cli_perr("Failed: verify_dispatch(write)", err);
		goto cleanup;
	}

	/* Read back through the same GPU queue path */
	for (uint32_t idx = 0; idx < total_threads; idx++)
		h_cmds[idx].common.opcode = XNVME_SPEC_NVM_OPC_READ;

	err = verify_dispatch(d_qps, h_cmds, d_cmds, d_errs, h_errs, total_queues, args->qdepth,
			      "read");
	if (err) {
		xnvme_cli_perr("Failed: verify_dispatch(read)", err);
		goto cleanup;
	}

	/* Compare read-back data against expected pattern, report per device */
	for (int d = 0; d < args->ndevs; d++) {
		uint64_t mismatches = 0;
		uint32_t base = (uint32_t)d * threads_per_dev;

		for (uint32_t i = 0; i < threads_per_dev; i++) {
			uint32_t idx = base + i;
			uint32_t q = i / args->qdepth;
			uint32_t t = i % args->qdepth;
			size_t diff = 0;

			err = fill_pattern(cmp_buf, args->iosize, h_cmds[idx].nvm.slba, nlbas);
			if (err) {
				fprintf(stderr,
					"Failed: fill_pattern() for dev=%s q=%u t=%u, err: %d\n",
					args->dev_uris[d], q, t, err);
				goto cleanup;
			}
			err = xnvme_buf_diff(cmp_buf, bufs[(uint32_t)d * args->nqueues + q][t],
					     args->iosize, &diff);
			if (err) {
				fprintf(stderr,
					"Failed: xnvme_buf_diff() for dev=%s q=%u t=%u, err: %d\n",
					args->dev_uris[d], q, t, err);
				goto cleanup;
			}

			if (diff) {
				fprintf(stderr, "  MISMATCH at dev=%s q=%u t=%u slba=%lu\n",
					args->dev_uris[d], q, t,
					(unsigned long)h_cmds[idx].nvm.slba);
				mismatches++;
			}
		}

		printf(" %-20s  verified %u IOs, %lu mismatches\n", args->dev_uris[d],
		       threads_per_dev, mismatches);

		if (mismatches) {
			err = -EIO;
		}
	}

cleanup:
	cudaFree(d_errs);
	cudaFree(d_cmds);
	cudaFree(d_qps);
	xnvmeperf_cuda_cleanup(devs, args->ndevs, args->nqueues, args->qdepth, h_qps, bufs,
			       prp_bufs);
	free(bufs);
	free(prp_bufs);
	free(h_cmds);
	free(h_qps);
	free(h_errs);
	free(cmp_buf);
	return err;
}
