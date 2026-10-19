// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file xnvme_util.h
 * @brief Internal helpers shared by the command-line tools
 */
#ifndef __INTERNAL_XNVME_UTIL_H
#define __INTERNAL_XNVME_UTIL_H

#include <errno.h>
#include <pthread.h>
#include <stddef.h>

/**
 * Per-queue allowance for the uPCIe queue structures, overshooting the real
 * need to keep the heap math simple
 */
#define XNVME_UTIL_HEAP_QUEUE_OVERHEAD (16UL << 20)

/**
 * Estimate the uPCIe heap needed by 'nqueues' queues each using 'nbytes' of
 * buffers
 */
static inline size_t
xnvme_util_heap_size(size_t nqueues, size_t nbytes)
{
	return nqueues * (XNVME_UTIL_HEAP_QUEUE_OVERHEAD + nbytes);
}

/**
 * Pin the calling thread to 'cpu'
 *
 * The includer must define _GNU_SOURCE before its first libc header, as
 * cpu_set_t and the CPU_* macros are otherwise not declared.
 *
 * @return 0 on success, an errno value otherwise
 */
static inline int
xnvme_util_pin_to_cpu(int cpu)
{
#ifdef XNVME_PTHREAD_SETAFFINITY_NP_ENABLED
	cpu_set_t cpuset;

	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	return pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
#else
	(void)cpu;
	return ENOSYS;
#endif
}

#endif /* __INTERNAL_XNVME_UTIL_H */
