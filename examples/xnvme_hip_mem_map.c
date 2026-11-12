// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <hip/hip_runtime_api.h>
#include <string.h>
#include <libxnvme.h>

/**
 * NVMe I/O into a hipMalloc'd, registered buffer
 *
 * The HIP counterpart of xnvme_cuda_mem_map.cu: registers an
 * externally-allocated device buffer with xnvme_buf_register() instead of drawing
 * from the heap that xnvme_buf_alloc() serves, then reads into it. Submission
 * is host-side, since there is no HIP equivalent of the GPU-resident queue.
 *
 * Usage: xnvme_hip_mem_map <pci-id>
 *   e.g: xnvme_hip_mem_map 0000:01:00.0
 */

int
main(int argc, char **argv)
{
	struct xnvme_opts opts = {0};
	struct xnvme_dev *dev = NULL;
	struct xnvme_cmd_ctx ctx = {0};
	size_t lba_nbytes, buf_nbytes, nlb;
	void *buf = NULL;
	hipError_t herr;
	int mapped = 0;
	int err = 0;

	xnvme_opts_set_defaults(&opts);

	if (argc < 2 || !strcmp(argv[1], "--help")) {
		printf("Usage: %s <pci-id> [<homi-id>]\n\nRead into a hipMalloc'd buffer "
		       "registered with xnvme_buf_register()\n",
		       argv[0]);
		return argc < 2;
	}

	snprintf(opts.be, sizeof(opts.be), "%s", "upcie-hip");
	if (argc > 2) {
		opts.homi_id = (uint32_t)strtoul(argv[2], NULL, 0);
	}
	err = xnvme_dev_open_uri(argv[1], &opts, &dev);
	if (err) {
		xnvme_cli_perr("xnvme_dev_open_uri()", err);
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

	err = xnvme_buf_register(dev, buf, buf_nbytes);
	if (err) {
		xnvme_cli_perr("xnvme_buf_register()", err);
		goto exit;
	}
	mapped = 1;
	xnvme_cli_pinf("xnvme_buf_register(%p, %zu): registered", buf, buf_nbytes);

	struct xnvme_cmd_prep_nvm_read_args args = {
		.args_nbytes = sizeof(args),
		.nsid = xnvme_dev_get_nsid(dev),
		.slba = 0x0,
		.nlb = (uint16_t)(nlb - 1),
	};

	xnvme_dev_get_cmd_ctx(dev, &ctx);
	err = xnvme_cmd_prep_nvm_read(&ctx.cmd, &args);
	if (!err) {
		// The buffer is rounded up to 2 MiB; transfer the blocks read
		err = xnvme_cmd_pass(&ctx, buf, nlb * lba_nbytes, NULL, 0);
	}
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		err = err ? err : -EIO;
		xnvme_cli_perr("read", err);
		goto exit;
	}

	xnvme_cli_pinf("OK: read %zu LBAs into a registered hipMalloc buffer", nlb);

exit:
	if (mapped) {
		xnvme_buf_unregister(dev, buf);
	}
	if (buf) {
		hipFree(buf);
	}
	xnvme_dev_close(dev);

	return err;
}
