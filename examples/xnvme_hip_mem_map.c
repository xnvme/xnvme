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
 * It then closes the device without unregistering, reopens it and registers
 * the buffer again. Given a second controller, it registers the buffer through
 * that one too, closes the first without unregistering, and reads through the
 * second.
 *
 * Usage: xnvme_hip_mem_map <pci-id> [homi-id] [second-pci-id]
 *   e.g: xnvme_hip_mem_map 0000:01:00.0
 */

static int
read_lba0(struct xnvme_dev *dev, void *buf)
{
	struct xnvme_cmd_ctx ctx = {0};
	int err;

	xnvme_dev_get_cmd_ctx(dev, &ctx);
	err = xnvme_nvm_read(&ctx, xnvme_dev_get_nsid(dev), 0x0, 0, buf, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		err = err ? err : -EIO;
		fprintf(stderr, "xnvme_nvm_read(): %s\n", strerror(-err));
		return err;
	}

	return 0;
}

/**
 * Close without unregistering, reopen, and register the buffer again
 *
 * Closing the last device takes down what was registered through it, so the
 * buffer is registered anew after reopening. A read into it tells that it was.
 */
static int
reopen_and_read(const char *uri, struct xnvme_opts *opts, struct xnvme_dev **dev, void *buf,
		size_t nbytes, int *mapped)
{
	int err;

	xnvme_dev_close(*dev);
	*mapped = 0;

	*dev = xnvme_dev_open(uri, opts);
	if (!*dev) {
		err = -errno;
		fprintf(stderr, "xnvme_dev_open(): %s\n", strerror(-err));
		return err;
	}

	err = xnvme_buf_register(*dev, buf, nbytes);
	if (err) {
		fprintf(stderr, "xnvme_buf_register(): %s\n", strerror(-err));
		return err;
	}
	*mapped = 1;

	err = read_lba0(*dev, buf);
	if (err) {
		return err;
	}

	printf("OK: read into the buffer registered again after reopening\n");

	return 0;
}

/**
 * Register the buffer through a second controller, close the first without
 * unregistering, and read through the second
 *
 * Served, each controller reaches the buffer through a registration of its
 * own, and closing one leaves the other's in place.
 */
static int
second_ctrlr_read(const char *uri, struct xnvme_opts *opts, struct xnvme_dev **dev, void *buf,
		  size_t nbytes)
{
	struct xnvme_dev *second;
	int err;

	second = xnvme_dev_open(uri, opts);
	if (!second) {
		err = -errno;
		fprintf(stderr, "xnvme_dev_open(): %s\n", strerror(-err));
		return err;
	}

	err = xnvme_buf_register(second, buf, nbytes);
	if (err) {
		fprintf(stderr, "xnvme_buf_register(): %s\n", strerror(-err));
		xnvme_dev_close(second);
		return err;
	}

	err = read_lba0(*dev, buf);
	err = err ? err : read_lba0(second, buf);
	if (err) {
		xnvme_buf_unregister(second, buf);
		xnvme_dev_close(second);
		return err;
	}

	xnvme_dev_close(*dev);
	*dev = second;

	err = read_lba0(*dev, buf);
	if (err) {
		return err;
	}

	printf("OK: read through the second controller after closing the first\n");

	return 0;
}

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
		printf("Usage: %s <pci-id> [<homi-id>] [<second-pci-id>]\n\n"
		       "Read into a hipMalloc'd buffer registered with xnvme_buf_register()\n",
		       argv[0]);
		return argc < 2;
	}

	opts.be = "upcie-hip";
	if (argc > 2) {
		opts.homi_id = (uint32_t)strtoul(argv[2], NULL, 0);
	}
	dev = xnvme_dev_open(argv[1], &opts);
	if (!dev) {
		err = -errno;
		fprintf(stderr, "xnvme_dev_open(): %s\n", strerror(-err));
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
		fprintf(stderr, "hipMalloc(): %s\n", strerror(-err));
		goto exit;
	}
	printf("hipMalloc(%zu) -> %p\n", buf_nbytes, buf);

	err = xnvme_buf_register(dev, buf, buf_nbytes);
	if (err) {
		fprintf(stderr, "xnvme_buf_register(): %s\n", strerror(-err));
		goto exit;
	}
	mapped = 1;
	printf("xnvme_buf_register(%p, %zu): registered\n", buf, buf_nbytes);

	xnvme_dev_get_cmd_ctx(dev, &ctx);
	err = xnvme_nvm_read(&ctx, xnvme_dev_get_nsid(dev), 0x0, nlb - 1, buf, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		err = err ? err : -EIO;
		fprintf(stderr, "xnvme_nvm_read(): %s\n", strerror(-err));
		goto exit;
	}

	printf("OK: read %zu LBAs into a registered hipMalloc buffer\n", nlb);

	err = reopen_and_read(argv[1], &opts, &dev, buf, buf_nbytes, &mapped);
	if (!err && (argc > 3)) {
		err = second_ctrlr_read(argv[3], &opts, &dev, buf, buf_nbytes);
	}

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
