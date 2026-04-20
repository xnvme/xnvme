// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

/**
 * GPU-resident NVMe IO example
 *
 * Demonstrates how to use the libxnvme_cuda API to submit NVMe read commands
 * from a CUDA kernel using a GPU-resident queue pair.
 *
 * Usage: xnvme_gpu <pci-id>
 *   e.g: xnvme_gpu 0000:01:00.0
 */

// [kernel]
__global__ static void
xnvme_cuda_io_kernel(struct xnvme_cuda_queue *qp, struct xnvme_spec_cmd *cmds, size_t batch_size,
		     int *errors)
{
	size_t tid = threadIdx.x;
	/* kernels cannot return values to the host directly, populate err array */
	errors[tid] = xnvme_cuda_cmd_io(qp, &cmds[tid], tid, batch_size);
}
// [kernel]

// [host]
int
main(int argc, char **argv)
{
	struct xnvme_opts opts = xnvme_opts_default();
	struct xnvme_dev *dev = NULL;
	struct xnvme_spec_cmd *h_cmds = NULL, *d_cmds = NULL;
	struct xnvme_cuda_queue *gpu_queue = NULL;
	uint64_t phys;
	uint32_t nsid;
	size_t qdepth, lba_nbytes;
	char *buf = NULL;
	int *h_errors = NULL, *d_errors = NULL;
	cudaError_t cerr;
	int err = 0;

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

	/*
	 * qdepth is both the CUDA block dimension and the NVMe queue depth.
	 * Pass it as batch_size to the kernel to make all threads active.
	 * To submit a partial batch, pass a smaller value as batch_size;
	 * threads with tid >= batch_size participate in barriers but do not
	 * submit or reap a command.
	 */
	qdepth = 64;
	lba_nbytes = xnvme_dev_get_geo(dev)->lba_nbytes;
	nsid = xnvme_dev_get_nsid(dev);

	buf = (char *)xnvme_buf_alloc(dev, qdepth * lba_nbytes);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	/* Prepare one command per thread with physical PRP addresses */
	h_cmds = (struct xnvme_spec_cmd *)calloc(qdepth, sizeof(*h_cmds));
	if (!h_cmds) {
		err = -errno;
		xnvme_cli_perr("calloc(h_cmds)", err);
		goto exit;
	}

	for (size_t i = 0; i < qdepth; i++) {
		xnvme_buf_vtophys(dev, buf + i * lba_nbytes, &phys);

		h_cmds[i].common.opcode = XNVME_SPEC_NVM_OPC_READ;
		h_cmds[i].nvm.slba = i;
		h_cmds[i].common.nsid = nsid;
		h_cmds[i].common.dptr.prp.prp1 = phys;
		h_cmds[i].nvm.nlb = 0; /* nlb is zero-based; 0 means 1 LBA */
	}

	/* Copy commands to device memory before passing to the kernel */
	cerr = cudaMalloc((void **)&d_cmds, qdepth * sizeof(*d_cmds));
	if (cerr) {
		xnvme_cli_perr("cudaMalloc(d_cmds), cudaError_t: %d", cerr);
		err = -ENOMEM;
		goto exit;
	}
	cudaMemcpy(d_cmds, h_cmds, qdepth * sizeof(*d_cmds), cudaMemcpyHostToDevice);

	/* Allocate a per-thread error buffer so the kernel can report failures */
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
		printf("OK: %zu GPU-submitted NVMe reads completed successfully\n", qdepth);
	}

exit:
	if (gpu_queue) {
		xnvme_cuda_queue_destroy(dev, gpu_queue);
	}
	cudaFree(d_errors);
	cudaFree(d_cmds);
	free(h_errors);
	free(h_cmds);
	xnvme_buf_free(dev, buf);
	xnvme_dev_close(dev);

	return err;
}
// [host]
