// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <cuda_runtime.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <libxnvme.h>

extern "C" {
#include "xnvmeperf.h"
}

/**
 * The thread's next command: a random LBA from its own LCG when seeded,
 * otherwise a stride of the queue's depth through the device, wrapping at
 * its end, and a command identifier unique among what the queue holds.
 */
__device__ static void
xnvmeperf_cuda_next(struct xnvme_spec_cmd *cmd, int random, uint64_t *seed, uint64_t *offset,
		    uint64_t cap, uint16_t nlbas, size_t tid, size_t qdepth, uint32_t *nsubmitted)
{
	if (random) {
		*seed = *seed * 6364136223846793005ULL + 1442695040888963407ULL;
		cmd->nvm.slba = ((*seed >> 33) % cap) * (uint64_t)nlbas;
	} else {
		cmd->nvm.slba = *offset;
		*offset += (uint64_t)qdepth * nlbas;
		if (*offset >= cap * (uint64_t)nlbas) {
			*offset = (uint64_t)tid * nlbas;
		}
	}

	/* A batch resubmits after reaping the queue's oldest completions, which
	 * need not be its own, so a slot's previous command may still be in
	 * flight when the slot is reused: the identifier advances by the depth
	 * each time and stays unique across many more submissions than a
	 * command outlives, and clear of the 0xFFFF the reap treats as empty. */
	cmd->common.cid = (uint16_t)((tid + (size_t)*nsubmitted * qdepth) & 0x7FFF);
	(*nsubmitted)++;
}

/**
 * One CUDA block per queue, one thread per queue slot, the slots in nbatches
 * groups that take turns: a group reaps the queue's next batch of completions,
 * whichever slots they belong to, and refills the room they leave.
 *
 * A group's turn is what a closed loop costs the queue: the barrier, the
 * fence, the doorbell and the controller's fetch of the new entries before
 * anything new is in service. With one batch the queue drains to empty for
 * every such turn, so the round trip is paid on every depth's worth of I/O;
 * with two or more the other groups' commands are still in service while a
 * group turns, and the queue never runs empty. All threads take every barrier,
 * so a thread whose group is not up waits at them.
 *
 * Counts completed commands, not rounds, since what is in flight at the stop
 * is drained but never resubmitted.
 */
__global__ static void
xnvmeperf_cuda_kernel_run(struct xnvme_cuda_queue **qps, struct xnvme_spec_cmd *cmds,
			  uint64_t *nblocks, uint16_t nlbas, uint64_t *seeds, uint32_t nbatches,
			  volatile int *stop, uint64_t *out_completed, uint64_t *out_failed, int live)
{
	struct xnvme_cuda_queue *qp;
	struct xnvme_spec_cmd cmd;
	struct xnvme_spec_cpl cpl = {0};
	uint64_t cap, offset, seed, completed = 0, failed = 0;
	uint32_t nsubmitted = 0;
	__shared__ int s_stop;
	int err;

	const size_t bid = blockIdx.x;
	const size_t tid = threadIdx.x;
	const size_t qdepth = blockDim.x;
	const size_t batch = qdepth / nbatches;
	const size_t group = tid / batch;
	const uint16_t lane = (uint16_t)(tid % batch);
	const int random = seeds != NULL;
	size_t turn = 0;

	qp = qps[bid];
	cap = nblocks[bid];
	cmd = cmds[bid * qdepth + tid];
	seed = random ? seeds[bid * qdepth + tid] : 0;
	offset = (uint64_t)tid * nlbas;

	/* Each thread fences its own entry before the barrier. The fence thread 0
	 * issues before the doorbell does not order the other warps' stores on
	 * an H100 when the controller is already fetching: with two batches in
	 * flight it then fetched entries before they were written, and executed
	 * them as what it found there, Invalid Namespace or Format on a zeroed
	 * one and Command ID Conflict on a stale one, a few per million. */
	for (size_t g = 0; g < nbatches; g++) {
		if (group == g) {
			xnvmeperf_cuda_next(&cmd, random, &seed, &offset, cap, nlbas, tid, qdepth,
					    &nsubmitted);
			xnvme_cuda_enqueue_at_i(qp, &cmd, lane);
			__threadfence_system();
		}
		__syncthreads();
		if (tid == 0) {
			xnvme_cuda_sq_update(qp, batch);
		}
	}

	while (true) {
		/* Thread 0 samples the stop flag and broadcasts it through shared
		 * memory, so every thread leaves at the same barrier. */
		if (tid == 0) {
			s_stop = *stop;
		}
		__syncthreads();
		if (s_stop) {
			break;
		}

		if (group == turn) {
			err = xnvme_cuda_reap_at_i(qp, qp->timeout_ms, &cpl, lane);
			if (!err) {
				err = cpl.status.sc;
			}
			if (err) {
				failed++;
			}
		}
		__syncthreads();
		if (tid == 0) {
			xnvme_cuda_cq_update(qp, batch);
			completed += batch;
			if (live) {
				atomicExch_system((unsigned long long *)&out_completed[bid],
						  (unsigned long long)completed);
			}
		}

		if (group == turn) {
			xnvmeperf_cuda_next(&cmd, random, &seed, &offset, cap, nlbas, tid, qdepth,
					    &nsubmitted);
			xnvme_cuda_enqueue_at_i(qp, &cmd, lane);
			__threadfence_system();
		}
		__syncthreads();
		if (tid == 0) {
			xnvme_cuda_sq_update(qp, batch);
		}

		turn = (turn + 1) % nbatches;
	}

	for (size_t g = 0; g < nbatches; g++) {
		if (group == turn) {
			err = xnvme_cuda_reap_at_i(qp, qp->timeout_ms, &cpl, lane);
			if (!err) {
				err = cpl.status.sc;
			}
			if (err) {
				failed++;
			}
		}
		__syncthreads();
		if (tid == 0) {
			xnvme_cuda_cq_update(qp, batch);
			completed += batch;
		}
		turn = (turn + 1) % nbatches;
	}

	if (tid == 0) {
		out_completed[bid] = completed;
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
		     uint32_t nqueues, int queue_opts, struct xnvme_cuda_queue **h_qps,
		     void ***bufs, void ***prp_bufs, uint64_t *h_nblocks)
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

			err = xnvme_cuda_queue_create(dev, qdepth, queue_opts, &h_qps[qi]);
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
 * Launches one CUDA block per queue and runs for runtime_secs; random LBAs
 * when h_seeds is non-NULL, sequential otherwise. h_completed receives the
 * commands each queue completed.
 */
static int
xnvmeperf_cuda_launch(struct xnvme_cuda_queue **h_qps, struct xnvme_spec_cmd *h_cmds,
		      uint64_t *h_seeds, uint32_t nqueues, uint64_t *h_nblocks, uint16_t nlbas,
		      uint32_t runtime_secs, unsigned int qdepth, uint32_t nbatches,
		      uint64_t *h_completed, uint64_t *h_failed, float *elapsed_ms,
		      double report_freq, uint32_t iosize)
{
	struct xnvme_cuda_queue **d_qps = NULL;
	struct xnvme_spec_cmd *d_cmds = NULL;
	uint64_t *d_seeds = NULL, *d_nblocks = NULL, *d_failed = NULL;
	volatile uint64_t *h_completed_live = NULL;
	void *d_stop = NULL, *d_completed = NULL;
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

	cerr = cudaHostAlloc((void **)&h_completed_live, nqueues * sizeof(*h_completed_live),
			     cudaHostAllocMapped);
	if (cerr) {
		fprintf(stderr, "Failed: cudaHostAlloc(): %s\n", cudaGetErrorString(cerr));
		goto done;
	}
	memset((void *)h_completed_live, 0, nqueues * sizeof(*h_completed_live));

	cerr = cudaHostGetDevicePointer(&d_completed, (void *)h_completed_live, 0);
	if (cerr) {
		fprintf(stderr, "Failed: cudaHostGetDevicePointer(): %s\n",
			cudaGetErrorString(cerr));
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

	xnvmeperf_cuda_kernel_run<<<nqueues, qdepth>>>(
		d_qps, d_cmds, d_nblocks, nlbas, d_seeds, nbatches, (volatile int *)d_stop,
		(uint64_t *)d_completed, d_failed, report_freq != 0.0);

	if (report_freq != 0.0) {
		uint64_t report_freq_ns = (uint64_t)(report_freq * 1000000000.0);
		uint64_t runtime_ns = (uint64_t)runtime_secs * 1000000000ULL;
		uint64_t deadline = report_freq_ns;
		uint64_t prev_completed = 0, prev_elapsed = 0;
		struct xnvme_timer timer = {0};

		xnvme_timer_start(&timer);
		print_intermediate_header();

		while (1) {
			struct timespec ts;
			uint64_t completed = 0, elapsed, wakeup;

			xnvme_timer_stop(&timer);
			elapsed = xnvme_timer_elapsed_nsecs(&timer);
			if (elapsed >= runtime_ns) {
				break;
			}
			if (elapsed >= deadline) {
				for (uint32_t q = 0; q < nqueues; q++) {
					completed += h_completed_live[q];
				}
				print_intermediate_result((double)elapsed / 1000000000.0,
							  (double)(elapsed - prev_elapsed) /
								  1000000000.0,
							  completed - prev_completed, iosize);
				prev_completed = completed;
				prev_elapsed = elapsed;
				while (deadline <= elapsed) {
					deadline += report_freq_ns;
				}
				continue;
			}

			wakeup = deadline < runtime_ns ? deadline : runtime_ns;
			ts.tv_sec = (time_t)((wakeup - elapsed) / 1000000000ULL);
			ts.tv_nsec = (long)((wakeup - elapsed) % 1000000000ULL);
			nanosleep(&ts, NULL);
		}
	} else {
		sleep(runtime_secs);
	}

	*h_stop = 1;

	cerr = cudaEventRecord(t1);
	if (cerr) {
		fprintf(stderr, "Failed: cudaEventRecord(): %s\n", cudaGetErrorString(cerr));
		goto done;
	}
	cerr = cuda_sync_check();
	if (!cerr) {
		cudaEventElapsedTime(elapsed_ms, t0, t1);
		memcpy(h_completed, (void *)h_completed_live, nqueues * sizeof(*h_completed));
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
	cudaFreeHost((void *)h_completed_live);
	cudaFreeHost(h_stop);
	cudaFree(d_failed);
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

	if (xnvme_dev_get_geo(devs[0])->lba_nbytes == 0) {
		fprintf(stderr,
			"Error: device %s reports LBA size 0 (identify likely returned zeros)\n",
			args->dev_uris[0]);
		return -EINVAL;
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
		      uint64_t *completed_per_dev, uint64_t *failed_per_dev, float *elapsed_ms)
{
	struct xnvme_cuda_queue **h_qps;
	struct xnvme_spec_cmd *h_cmds;
	void ***bufs, ***prp_bufs;
	uint64_t *nblocks, *completed, *failed, *h_seeds = NULL;
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
	completed = (uint64_t *)calloc(total_queues, sizeof(*completed));
	failed = (uint64_t *)calloc(total_queues, sizeof(*failed));

	if (!h_qps || !h_cmds || !nblocks || !bufs || !prp_bufs || !completed || !failed) {
		err = -ENOMEM;
		xnvme_cli_perr("Failed: calloc()", err);
		goto cleanup;
	}

	err = xnvmeperf_cuda_setup(devs, args->ndevs, args->iosize, args->qdepth, args->nqueues,
				   args->queue_opts, h_qps, bufs, prp_bufs, nblocks);
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
				    args->time, args->qdepth, args->nbatches, completed, failed,
				    elapsed_ms, args->report_freq, args->iosize);

	if (!err) {
		for (int d = 0; d < args->ndevs; d++) {
			completed_per_dev[d] = 0;
			failed_per_dev[d] = 0;
			for (uint32_t q = 0; q < args->nqueues; q++) {
				completed_per_dev[d] += completed[(uint32_t)d * args->nqueues + q];
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
	free(completed);
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
				   args->queue_opts, h_qps, bufs, prp_bufs, NULL);
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

/* ------------------------------------------------------------------------- *
 * Host-bounce staging and the host-to-device copy roofline
 *
 * The NVMe I/O runs on a host backend and lands in host buffers; these move
 * the payload on to the GPU with cudaMemcpyAsync, overlapping the copy of one
 * slot with the read into the next. The generic loop in xnvmeperf.c owns the
 * host buffers and hands slots here; this file owns the device buffer, one
 * copy stream and one completion event per slot.
 * ------------------------------------------------------------------------- */
#include <errno.h>
#include <string.h>
#include <time.h>

extern "C" {

struct xnvmeperf_gpu {
	uint32_t iosize;
	uint32_t nslots;
	uint8_t *dev;        ///< device buffer, nslots * iosize
	cudaStream_t stream; ///< one stream carries every slot's copy in order
	cudaEvent_t *events; ///< per-slot completion, so a slot is reused only once drained
	void **hbufs;        ///< host buffers page-locked here, unregistered at close
	uint8_t *pending;    ///< 1 while a slot's copy is enqueued and not yet observed done
};

int
xnvmeperf_gpu_set_device(uint32_t gpu_id)
{
	cudaError_t cerr = cudaSetDevice((int)gpu_id);

	if (cerr != cudaSuccess) {
		fprintf(stderr, "Failed: cudaSetDevice(%u): %s\n", gpu_id,
			cudaGetErrorString(cerr));
		return -EIO;
	}
	return 0;
}

struct xnvmeperf_gpu *
xnvmeperf_gpu_bounce_open(uint32_t gpu_id, uint32_t iosize, uint32_t nslots)
{
	struct xnvmeperf_gpu *gpu;
	cudaError_t cerr;

	if (xnvmeperf_gpu_set_device(gpu_id)) {
		return NULL;
	}
	gpu = (struct xnvmeperf_gpu *)calloc(1, sizeof(*gpu));
	if (!gpu) {
		errno = ENOMEM;
		return NULL;
	}
	gpu->iosize = iosize;
	gpu->nslots = nslots;
	gpu->events = (cudaEvent_t *)calloc(nslots, sizeof(*gpu->events));
	gpu->hbufs = (void **)calloc(nslots, sizeof(*gpu->hbufs));
	gpu->pending = (uint8_t *)calloc(nslots, sizeof(*gpu->pending));
	if (!gpu->events || !gpu->hbufs || !gpu->pending) {
		goto failed;
	}

	cerr = cudaMalloc((void **)&gpu->dev, (size_t)nslots * iosize);
	if (cerr != cudaSuccess) {
		fprintf(stderr, "Failed: cudaMalloc(%zu): %s\n", (size_t)nslots * iosize,
			cudaGetErrorString(cerr));
		goto failed;
	}
	cerr = cudaStreamCreateWithFlags(&gpu->stream, cudaStreamNonBlocking);
	if (cerr != cudaSuccess) {
		fprintf(stderr, "Failed: cudaStreamCreate(): %s\n", cudaGetErrorString(cerr));
		goto failed;
	}
	for (uint32_t i = 0; i < nslots; i++) {
		cerr = cudaEventCreateWithFlags(&gpu->events[i], cudaEventDisableTiming);
		if (cerr != cudaSuccess) {
			fprintf(stderr, "Failed: cudaEventCreate(): %s\n",
				cudaGetErrorString(cerr));
			goto failed;
		}
	}
	return gpu;

failed:
	xnvmeperf_gpu_bounce_close(gpu);
	errno = ENOMEM;
	return NULL;
}

int
xnvmeperf_gpu_bounce_register(struct xnvmeperf_gpu *gpu, uint32_t slot, void *hbuf)
{
	cudaError_t cerr;

	gpu->hbufs[slot] = hbuf;
	cerr = cudaHostRegister(hbuf, gpu->iosize, cudaHostRegisterDefault);
	if (cerr == cudaErrorHostMemoryAlreadyRegistered) {
		cudaGetLastError();
		return 0;
	}
	if (cerr != cudaSuccess) {
		/* Leave it unregistered: the copy still runs, just at the pageable
		 * rate, which the report then reflects rather than hides. */
		gpu->hbufs[slot] = NULL;
		fprintf(stderr, "Warning: cudaHostRegister(%p): %s; copy will be pageable\n", hbuf,
			cudaGetErrorString(cerr));
		cudaGetLastError();
		return -EIO;
	}
	return 0;
}

int
xnvmeperf_gpu_bounce_ready(struct xnvmeperf_gpu *gpu, uint32_t slot)
{
	if (!gpu->pending[slot]) {
		return 1;
	}
	if (cudaEventQuery(gpu->events[slot]) == cudaSuccess) {
		gpu->pending[slot] = 0;
		return 1;
	}
	return 0;
}

int
xnvmeperf_gpu_bounce_copy(struct xnvmeperf_gpu *gpu, uint32_t slot, void *hbuf)
{
	cudaError_t cerr;

	cerr = cudaMemcpyAsync(gpu->dev + (size_t)slot * gpu->iosize, hbuf, gpu->iosize,
			       cudaMemcpyHostToDevice, gpu->stream);
	if (cerr != cudaSuccess) {
		fprintf(stderr, "Failed: cudaMemcpyAsync(): %s\n", cudaGetErrorString(cerr));
		return -EIO;
	}
	cudaEventRecord(gpu->events[slot], gpu->stream);
	gpu->pending[slot] = 1;
	return 0;
}

void
xnvmeperf_gpu_bounce_drain(struct xnvmeperf_gpu *gpu)
{
	cudaStreamSynchronize(gpu->stream);
	memset(gpu->pending, 0, gpu->nslots);
}

void
xnvmeperf_gpu_bounce_close(struct xnvmeperf_gpu *gpu)
{
	if (!gpu) {
		return;
	}
	if (gpu->stream) {
		cudaStreamSynchronize(gpu->stream);
	}
	for (uint32_t i = 0; i < gpu->nslots; i++) {
		if (gpu->hbufs && gpu->hbufs[i]) {
			cudaHostUnregister(gpu->hbufs[i]);
		}
		if (gpu->events && gpu->events[i]) {
			cudaEventDestroy(gpu->events[i]);
		}
	}
	if (gpu->dev) {
		cudaFree(gpu->dev);
	}
	if (gpu->stream) {
		cudaStreamDestroy(gpu->stream);
	}
	cudaGetLastError();
	free(gpu->events);
	free(gpu->hbufs);
	free(gpu->pending);
	free(gpu);
}

static double
_now_s(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int
xnvmeperf_htod_roofline(uint32_t gpu_id, uint32_t iosize, uint32_t nslots, uint32_t seconds,
			double *gbps)
{
	uint8_t *hbuf = NULL, *dev = NULL;
	cudaStream_t stream = NULL;
	cudaEvent_t *events = NULL;
	uint8_t *pending = NULL;
	uint64_t done = 0;
	double t0, elapsed;
	cudaError_t cerr;
	int err = 0;

	if (xnvmeperf_gpu_set_device(gpu_id)) {
		return -EIO;
	}
	events = (cudaEvent_t *)calloc(nslots, sizeof(*events));
	pending = (uint8_t *)calloc(nslots, sizeof(*pending));
	if (!events || !pending) {
		err = -ENOMEM;
		goto out;
	}
	if ((cerr = cudaHostAlloc((void **)&hbuf, iosize, cudaHostAllocDefault)) != cudaSuccess ||
	    (cerr = cudaMalloc((void **)&dev, (size_t)nslots * iosize)) != cudaSuccess ||
	    (cerr = cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking)) != cudaSuccess) {
		fprintf(stderr, "Failed: roofline setup: %s\n", cudaGetErrorString(cerr));
		err = -EIO;
		goto out;
	}
	for (uint32_t i = 0; i < nslots; i++) {
		if ((cerr = cudaEventCreateWithFlags(&events[i], cudaEventDisableTiming)) !=
		    cudaSuccess) {
			fprintf(stderr, "Failed: cudaEventCreate(): %s\n",
				cudaGetErrorString(cerr));
			err = -EIO;
			goto out;
		}
	}

	t0 = _now_s();
	while (_now_s() - t0 < (double)seconds) {
		for (uint32_t s = 0; s < nslots; s++) {
			if (pending[s] && cudaEventQuery(events[s]) != cudaSuccess) {
				continue;
			}
			cudaMemcpyAsync(dev + (size_t)s * iosize, hbuf, iosize,
					cudaMemcpyHostToDevice, stream);
			cudaEventRecord(events[s], stream);
			pending[s] = 1;
			done++;
		}
	}
	cudaStreamSynchronize(stream);
	elapsed = _now_s() - t0;
	*gbps = (double)done * (double)iosize / elapsed / 1e9;

out:
	if (events) {
		for (uint32_t i = 0; i < nslots; i++) {
			if (events[i]) {
				cudaEventDestroy(events[i]);
			}
		}
	}
	if (stream) {
		cudaStreamDestroy(stream);
	}
	if (dev) {
		cudaFree(dev);
	}
	if (hbuf) {
		cudaFreeHost(hbuf);
	}
	cudaGetLastError();
	free(events);
	free(pending);
	return err;
}

} /* extern "C" */
