// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Memory-barriers and other intrinsics
 * ====================================
 *
 * This header defines memory and instruction barriers for x86 (x86_64, i386) and ARM (aarch64,
 * arm). These are used to control the ordering of memory operations. It also provides a CPU
 * relaxation utility for use in spin-loops.
 *
 * Will not compile on other architectures.
 *
 * @file barriers.h
 * @version 0.3.2
 */

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h> // for _mm_pause()
#endif

/**
 * Compiler memory barrier
 *
 * Prevents the compiler from reordering memory accesses across this point. This does not emit any
 * CPU instruction.
 */
static inline void
barrier(void)
{
	__asm__ volatile("" ::: "memory");
}

/**
 * Read memory barrier
 *
 * Ensures that all read operations before the barrier are globally visible before any subsequent
 * reads. Enforces ordering of loads.
 */
static inline void
rmb(void)
{
#if defined(__aarch64__)
	__asm__ volatile("dsb ld" ::: "memory");
#elif defined(__x86_64__) || defined(__i386__)
	__asm__ volatile("lfence" ::: "memory");
#else
#error "rmb() not implemented for this architecture"
#endif
}

/**
 * Write memory barrier
 *
 * Ensures that all write operations before the barrier are globally visible before any subsequent
 * writes. Enforces ordering of stores.
 */
static inline void
wmb(void)
{
#if defined(__aarch64__)
	__asm__ volatile("dsb st" ::: "memory");
#elif defined(__x86_64__) || defined(__i386__)
	__asm__ volatile("sfence" ::: "memory");
#else
#error "wmb() not implemented for this architecture"
#endif
}

/**
 * Full memory barrier
 *
 * Ensures that all memory operations (loads and stores) before the barrier are globally visible
 * before any subsequent memory operations.
 */
static inline void
mb(void)
{
#if defined(__aarch64__)
	__asm__ volatile("dsb sy" ::: "memory");
#elif defined(__x86_64__) || defined(__i386__)
	__asm__ volatile("mfence" ::: "memory");
#else
#error "mb() not implemented for this architecture"
#endif
}

/**
 * DMA read memory barrier
 *
 * Ensures the CPU observes DMA writes performed by a device before any subsequent reads by the CPU
 * from the same memory region.
 *
 * Use this before reading from memory that was written by a device (e.g., completion queues).
 */
static inline void
dma_rmb(void)
{
#if defined(__aarch64__)
	__asm__ volatile("dmb oshld" ::: "memory");
#elif defined(__x86_64__) || defined(__i386__)
	barrier(); // Sufficient on x86
#else
#error "dma_rmb() not implemented for this architecture"
#endif
}

/**
 * CPU relaxation hint for spin-wait loops
 *
 * On x86, emits the `pause` instruction (rep; nop), reducing power and bus contention.
 * On ARM, emits `yield`.
 * On other architectures, defaults to a no-op.
 */
static inline void
cpu_relax(void)
{
#if defined(__x86_64__) || defined(__i386__)
	_mm_pause();
#elif defined(__aarch64__) || defined(__arm__)
	__asm__ volatile("yield");
#else
	// fallback: do nothing
#endif
}