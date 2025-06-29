// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Utilities for debug-printing, static size-checks, and unused-annotation
 * =======================================================================
 *
 * @file debug.h
 * @version 0.3.2
 */

#ifdef UPCIE_DEBUG_ENABLED

#define UPCIE_DEBUG_FCALL(x) x

#define __FILENAME__ strrchr("/" __FILE__, '/') + 1

#define UPCIE_DEBUG(...)                                                                    \
	fprintf(stderr, "# DBG:%s:%s-%d: " FIRST(__VA_ARGS__) "\n", __FILENAME__, __func__, \
		__LINE__ REST(__VA_ARGS__));                                                \
	fflush(stderr);

#define FIRST(...) FIRST_HELPER(__VA_ARGS__, throwaway)
#define FIRST_HELPER(first, ...) first

#define REST(...) REST_HELPER(NUM(__VA_ARGS__), __VA_ARGS__)
#define REST_HELPER(qty, ...) REST_HELPER2(qty, __VA_ARGS__)
#define REST_HELPER2(qty, ...) REST_HELPER_##qty(__VA_ARGS__)
#define REST_HELPER_ONE(first)
#define REST_HELPER_TWOORMORE(first, ...) , __VA_ARGS__
#define NUM(...)                                                                        \
	SELECT_10TH(__VA_ARGS__, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, TWOORMORE, \
		    TWOORMORE, TWOORMORE, TWOORMORE, ONE, throwaway)
#define SELECT_10TH(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, ...) a10

#else
#define UPCIE_DEBUG(...)
#define UPCIE_DEBUG_FCALL(x)
#endif

/**
 * Verity structure size at compile-time
 */

#ifdef static_assert
#define UPCIE_STATIC_ASSERT(cond, msg) static_assert(cond, msg);
#else
#define UPCIE_STATIC_ASSERT(cond, msg)
#endif

/**
 * Macro to suppress warnings on unused arguments, thanks to stackoverflow.
 */
#ifdef __GNUC__
#define UPCIE_UNUSED(x) UNUSED_##x __attribute__((__unused__))
#else
#define UPCIE_UNUSED(x) UNUSED_##x
#endif

static inline unsigned int
upcie_util_shift_from_size(size_t page_size)
{
	unsigned int shift = 0;

	assert(page_size != 0 && (page_size & (page_size - 1)) == 0);

	while ((1UL << shift) < page_size) {
		shift++;
	}

	return shift;
}
