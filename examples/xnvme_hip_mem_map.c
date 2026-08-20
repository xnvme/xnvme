// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <hip/hip_runtime_api.h>
#include <libxnvme.h>

/**
 * NVMe I/O into a hipMalloc'd, mem_map'd buffer
 *
 * The HIP counterpart of xnvme_cuda_mem_map.cu: registers an
 * externally-allocated device buffer with xnvme_mem_map() instead of drawing
 * from the heap that xnvme_buf_alloc() serves, then reads into it. Submission
 * is host-side, since there is no HIP equivalent of the GPU-resident queue.
 *
 * Usage: xnvme_hip_mem_map <pci-id>
 *   e.g: xnvme_hip_mem_map 0000:01:00.0
 */

int
main(int argc, char **argv)
{
	struct xnvme_opts opts = xnvme_opts_default();
	struct xnvme_dev *dev = NULL;
	struct xnvme_cmd_ctx ctx = {0};
	size_t lba_nbytes, buf_nbytes, nlb;
	void *buf = NULL;
	hipError_t herr;
	int mapped = 0;
	int err = 0;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <pci-id>\n", argv[0]);
		return -EINVAL;
	}

	opts.be = "upcie-hip";
	dev = xnvme_dev_open(argv[1], &opts);
	if (!dev) {
		err = -errno;
		xnvme_cli_perr("xnvme_dev_open()", err);
		return err;
	}

	lba_nbytes = xnvme_dev_get_geo(dev)->lba_nbytes;
	nlb = 8;
	buf_nbytes = nlb * lba_nbytes;

	/* AMD's dma-buf export wants a 2 MiB-aligned range, coarser than the
	 * 4 KiB allocation granularity, so give registration something it can
	 * actually export rather than the bare payload size. */
	buf_nbytes = (buf_nbytes + (2 * 1024 * 1024ULL) - 1) & ~((2 * 1024 * 1024ULL) - 1);

	herr = hipMalloc(&buf, buf_nbytes);
	if (herr != hipSuccess) {
		err = -ENOMEM;
		xnvme_cli_perr("hipMalloc()", err);
		goto exit;
	}
	xnvme_cli_pinf("hipMalloc(%zu) -> %p", buf_nbytes, buf);

	err = xnvme_mem_map(dev, buf, buf_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_mem_map()", err);
		goto exit;
	}
	mapped = 1;
	xnvme_cli_pinf("xnvme_mem_map(%p, %zu): registered", buf, buf_nbytes);

	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_nvm_read(&ctx, xnvme_dev_get_nsid(dev), 0x0, nlb - 1, buf, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		err = err ? err : -EIO;
		xnvme_cli_perr("xnvme_nvm_read()", err);
		goto exit;
	}

	xnvme_cli_pinf("OK: read %zu LBAs into a mem_map'd hipMalloc buffer", nlb);

exit:
	if (mapped) {
		xnvme_mem_unmap(dev, buf);
	}
	if (buf) {
		hipFree(buf);
	}
	xnvme_dev_close(dev);

	return err;
}
