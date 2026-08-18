// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stdint.h>
#include <libxnvme.h>

/**
 * GPU-resident NVMe IO into a cudaMalloc'd, mem_map'd buffer
 *
 * Variant of xnvme_cuda.cu that registers an externally-allocated CUDA
 * buffer via xnvme_mem_map() instead of using xnvme_buf_alloc()'s
 * pre-built dma-buf heap. The remaining flow (GPU-resident queue,
 * CUDA-kernel submission, completion drain) is identical.
 *
 * Usage: xnvme_cuda_mem_map <pci-id>
 *   e.g: xnvme_cuda_mem_map 0000:01:00.0
 */

/*
 * upcie-cuda's cudamem_config.device_pagesize. mem_map requires vaddr and
 * nbytes to be a multiple of this; cudaMalloc does not guarantee that
 * alignment, so we over-allocate and round up.
 */
#define UPCIE_CUDA_DEVICE_PAGESIZE 65536u

__global__ static void
xnvme_cuda_io_kernel(struct xnvme_cuda_queue *qp, struct xnvme_spec_cmd *cmds, size_t batch_size,
		     int *errors)
{
	size_t tid = threadIdx.x;
	errors[tid] = xnvme_cuda_cmd_io(qp, &cmds[tid], tid, batch_size);
}

int
main(int argc, char **argv)
{
	struct xnvme_opts opts = xnvme_opts_default();
	struct xnvme_dev *dev = NULL;
	struct xnvme_spec_cmd *h_cmds = NULL, *d_cmds = NULL;
	struct xnvme_cuda_queue *gpu_queue = NULL;
	uint64_t phys;
	uint32_t nsid;
	size_t qdepth, lba_nbytes, buf_nbytes;
	void *raw = NULL;
	char *buf = NULL;
	int *h_errors = NULL, *d_errors = NULL;
	cudaError_t cerr;
	int err = 0;
	int mapped = 0;

	if (argc < 2) {
		err = -EINVAL;
		xnvme_cli_perr("Usage: %s <pci-id>", err);
		return err;
	}

	opts.be = "upcie-cuda";
	dev = xnvme_dev_open(argv[1], &opts);
	if (!dev) {
		err = -errno;
		xnvme_cli_perr("xnvme_dev_open()", err);
		return err;
	}

	qdepth = 64;
	lba_nbytes = xnvme_dev_get_geo(dev)->lba_nbytes;
	nsid = xnvme_dev_get_nsid(dev);

	/* mem_map demands a device-pagesize-aligned vaddr and length. */
	buf_nbytes = ((qdepth * lba_nbytes + UPCIE_CUDA_DEVICE_PAGESIZE - 1) /
		      UPCIE_CUDA_DEVICE_PAGESIZE) *
		     UPCIE_CUDA_DEVICE_PAGESIZE;

	/* Over-allocate by one page so we can hand mem_map an aligned start. */
	cerr = cudaMalloc(&raw, buf_nbytes + UPCIE_CUDA_DEVICE_PAGESIZE);
	if (cerr) {
		xnvme_cli_perr("cudaMalloc(buf), cudaError_t: %d", cerr);
		err = -ENOMEM;
		goto exit;
	}
	buf = (char *)(((uintptr_t)raw + UPCIE_CUDA_DEVICE_PAGESIZE - 1) &
		       ~((uintptr_t)UPCIE_CUDA_DEVICE_PAGESIZE - 1));

	err = xnvme_mem_map(dev, buf, buf_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_mem_map()", err);
		goto exit;
	}
	mapped = 1;

	h_cmds = (struct xnvme_spec_cmd *)calloc(qdepth, sizeof(*h_cmds));
	if (!h_cmds) {
		err = -errno;
		xnvme_cli_perr("calloc(h_cmds)", err);
		goto exit;
	}

	/*
	 * vtophys resolves through the mapping registry's chunk LUT; one
	 * lookup per LBA, no contiguity assumption.
	 */
	for (size_t i = 0; i < qdepth; i++) {
		err = xnvme_buf_vtophys(dev, buf + i * lba_nbytes, &phys);
		if (err) {
			xnvme_cli_perr("xnvme_buf_vtophys()", err);
			goto exit;
		}

		h_cmds[i].common.opcode = XNVME_SPEC_NVM_OPC_READ;
		h_cmds[i].nvm.slba = i;
		h_cmds[i].common.nsid = nsid;
		h_cmds[i].common.dptr.prp.prp1 = phys;
		h_cmds[i].nvm.nlb = 0;
	}

	cerr = cudaMalloc((void **)&d_cmds, qdepth * sizeof(*d_cmds));
	if (cerr) {
		xnvme_cli_perr("cudaMalloc(d_cmds), cudaError_t: %d", cerr);
		err = -ENOMEM;
		goto exit;
	}
	cudaMemcpy(d_cmds, h_cmds, qdepth * sizeof(*d_cmds), cudaMemcpyHostToDevice);

	cerr = cudaMalloc((void **)&d_errors, qdepth * sizeof(int));
	if (cerr) {
		xnvme_cli_perr("cudaMalloc(d_errors), cudaError_t: %d", cerr);
		err = -ENOMEM;
		goto exit;
	}
	cudaMemset(d_errors, 0, qdepth * sizeof(int));

	err = xnvme_cuda_queue_create(dev, qdepth, &gpu_queue);
	if (err) {
		xnvme_cli_perr("xnvme_cuda_queue_create()", err);
		goto exit;
	}

	xnvme_cuda_io_kernel<<<1, qdepth>>>(gpu_queue, d_cmds, qdepth, d_errors);
	cudaDeviceSynchronize();

	h_errors = (int *)calloc(qdepth, sizeof(int));
	if (!h_errors) {
		err = -errno;
		xnvme_cli_perr("calloc(h_errors)", err);
		goto exit;
	}
	cudaMemcpy(h_errors, d_errors, qdepth * sizeof(int), cudaMemcpyDeviceToHost);

	for (size_t i = 0; i < qdepth; i++) {
		if (h_errors[i]) {
			fprintf(stderr, "thread %zu failed: %d\n", i, h_errors[i]);
			err = h_errors[i];
		}
	}
	if (!err) {
		printf("OK: %zu GPU-submitted NVMe reads into mem_map'd cudaMalloc buffer\n",
		       qdepth);
	}

exit:
	if (gpu_queue) {
		xnvme_cuda_queue_destroy(dev, gpu_queue);
	}
	cudaFree(d_errors);
	cudaFree(d_cmds);
	free(h_errors);
	free(h_cmds);
	if (mapped) {
		xnvme_mem_unmap(dev, buf);
	}
	if (raw) {
		cudaFree(raw);
	}
	xnvme_dev_close(dev);

	return err;
}
