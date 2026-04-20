// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __XNVMEPERF_H
#define __XNVMEPERF_H

#include <stddef.h>
#include <stdint.h>

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

#endif /* __XNVMEPERF_H */
