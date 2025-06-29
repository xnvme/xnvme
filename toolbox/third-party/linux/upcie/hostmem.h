// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Host memory utilities
 * =====================
 *
 * @file hostmem.h
 * @version 0.3.2
 */

enum hostmem_backend {
	HOSTMEM_BACKEND_UNKNOWN = 0x0,
	HOSTMEM_BACKEND_MEMFD = 0x1,
	HOSTMEM_BACKEND_HUGETLBFS = 0x2,
};

static inline int
hostmem_internal_memfd_create(const char *name, unsigned int flags)
{
	return syscall(SYS_memfd_create, name, flags);
}

/**
 * Consult "/proc/self/pagemap" for the given va-space address
 *
 * NOTE: The implementation assumes 55bit VA-space, is this assumption safe?
 *
 * @param virt The address in the process virtual-address space to resolve the
 * physical address for
 * @param phys Pointer to where the physical address should be recorded
 *
 * @returns On success, 0 is returned. On error, negative errno is return to
 * indicate the error.
 *
 */
static inline int
hostmem_pagemap_virt_to_phys(void *virt, uint64_t *phys)
{
	const int pagemap_entry_bytes = 8;
	const uint64_t pfn_mask = ((1ULL << 55) - 1);
	const int pgsz = getpagesize();
	uint64_t entry = 0;
	uint64_t virt_pfn = (uint64_t)virt / pgsz;
	uint64_t phys_pfn;
	int fd;

	fd = open("/proc/self/pagemap", O_RDONLY);
	if (fd < 0) {
		UPCIE_DEBUG("FAILED: open(pagemap); errno(%d), fd(%d)", errno, fd);
		return -errno;
	}

	if (pread(fd, &entry, pagemap_entry_bytes, virt_pfn * pagemap_entry_bytes) !=
	    pagemap_entry_bytes) {
		UPCIE_DEBUG("FAILED: pread(pagemap); errno(%d)", errno);
		close(fd);
		return -errno;
	}

	if (!(entry & (1ULL << 63))) {
		UPCIE_DEBUG("FAILED: Page not present");
		close(fd);
		return -EINVAL;
	}

	phys_pfn = entry & pfn_mask;
	*phys = (phys_pfn * pgsz) + ((uint64_t)virt % pgsz);

	close(fd);

	return 0;
}
