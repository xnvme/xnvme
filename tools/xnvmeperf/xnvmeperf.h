// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __XNVMEPERF_H
#define __XNVMEPERF_H

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

#include <libxnvme.h>

enum iopattern {
	IOPATTERN_READ      = 1,
	IOPATTERN_WRITE     = 2,
	IOPATTERN_RANDREAD  = 3,
	IOPATTERN_RANDWRITE = 4,
	IOPATTERN_VERIFY    = 5, ///< Used for verify subcommand
};

struct xnvmeperf_args {
	int ndevs;
	const char **dev_uris;
	int ncpus;
	int *cpus;
	uint32_t qdepth;
	uint32_t iosize;
	uint32_t time;
	uint32_t count;
	uint32_t nqueues;
	enum iopattern pattern;
	struct xnvme_opts opts;
};

int
fill_pattern(void *buf, size_t nbytes, uint64_t slba, uint16_t nlb);

#ifdef XNVME_BE_UPCIE_CUDA_ENABLED
int
xnvmeperf_cuda_run_io(struct xnvme_dev **devs, const struct xnvmeperf_args *args,
		      uint64_t *rounds_per_dev, uint64_t *failed_per_dev, float *elapsed_ms);
int
xnvmeperf_cuda_verify_io(struct xnvme_dev **devs, const struct xnvmeperf_args *args);
#else
static inline int
xnvmeperf_cuda_run_io(struct xnvme_dev **XNVME_UNUSED(devs),
		      const struct xnvmeperf_args *XNVME_UNUSED(args),
		      uint64_t *XNVME_UNUSED(rounds_per_dev),
		      uint64_t *XNVME_UNUSED(failed_per_dev), float *XNVME_UNUSED(elapsed_ms))
{
	return -ENOSYS;
}

static inline int
xnvmeperf_cuda_verify_io(struct xnvme_dev **XNVME_UNUSED(devs),
			 const struct xnvmeperf_args *XNVME_UNUSED(args))
{
	return -ENOSYS;
}
#endif

#endif /* __XNVMEPERF_H */
