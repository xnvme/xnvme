// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Memory allocator for hugepages
 * ==============================
 *
 * These wrappers provide an encapsulated use of hugepages. That is, they work with memfd_create()
 * as well as hugetlbfs. The motivation for this library is to provide a minimal API encapsulating
 * the differences in tlbfs and memfd, making it more convenient to build things on top.
 *
 * The general motivation is to use hugepages as a way to obtain physically addressable memory
 * which is pinned and contigous.
 *
 * As these properties are required when working on user-space drivers. Additionally, then
 * hugepages and mmap regions with SHARED are very useful as a basic IPC channel. Thus, providing
 * this for the regions allocated here.
 *
 *  API: Hugepages
 *  --------------
 *
 * - hostmem_hugepage_init()
 *   - Checks environment variables and sets up config
 *
 * - hostmem_hugepage_alloc()
 *   - Allocate memory in multiples of hugepage size
 *
 * - hostmem_hugepage_import()
 *   - Import hugepage for allocated by another process
 *
 * - hostmem_hugepage_free()
 *   - De-allocate memory obtained with with hostmem_hugepage_{alloc,import}()
 *
 * Caveat: system setup
 * --------------------
 *
 * The library makes use of memfd_create(MFD_HUGETLB), however, you still need to allocate them
 * yourself. That is, have a system setup step than makes hugepages available, such as:
 *
 *    echo 128 | tee -a /proc/sys/vm/nr_hugepages
 *    ulimit -l unlimited
 *
 * Thus, a utility for this similar to devbind.py is needed. This is what we have today with
 * 'xnvme-driver', however, we want something simpler.
 *
 * Caveat: CAP_SYS_ADMIN
 * ---------------------
 *
 * Reading /proc/self/pagemap requires CAP_SYS_ADMIN, so hostmem_virt_to_phys() cannot be used by
 * non-privileged users. Therefore, any process needing DMA via this allocator must run as root.
 *
 * Possible Workaround: Since the allocator uses MAP_SHARED, a privileged "allocator-daemon" could
 * handle virt_to_phys translations and share the results via shared memory with unprivileged
 * clients. This allows integration into the heap with minimal complexity. Example:
 *
 * After heap initialization, write the heap structure into hugepage memory. Because phys_lut[]
 * resolves all physical addresses of the backing hugepages, any process that imports the hugepage
 * also gains access to those physical addresses—without needing CAP_SYS_ADMIN.
 *
 * @file hostmem_hugepage.h
 * @version 0.3.2
 */

struct hostmem_hugepage {
	int fd;
	void *virt;
	size_t size;
	uint64_t phys;
	char path[256];
	struct hostmem_config *config;
};

static inline int
hostmem_hugepage_pp(struct hostmem_hugepage *hugepage)
{
	int wrtn = 0;

	wrtn += printf("hostmem_hugepage:");

	if (!hugepage) {
		wrtn += printf(" ~\n");
		return 0;
	}

	wrtn += printf("\n");
	wrtn += printf("  fd: %d\n", hugepage->fd);
	wrtn += printf("  size: %zu\n", hugepage->size);
	wrtn += printf("  virt: 0x%" PRIx64 "\n", (uint64_t)hugepage->virt);
	wrtn += printf("  phys: 0x%" PRIx64 "\n", hugepage->phys);
	wrtn += printf("  path: '%.*s'\n", 256, hugepage->path);

	return wrtn;
};

/**
 * Deallocate a hugepage allocation
 */
static inline void
hostmem_hugepage_free(struct hostmem_hugepage *hugepage)
{
	if (!hugepage)
		return;

	if (hugepage->virt && hugepage->size) {
		munmap(hugepage->virt, hugepage->size);
	}

	if (HOSTMEM_BACKEND_HUGETLBFS == hugepage->config->backend) {
		unlink(hugepage->path);
	}

	memset(hugepage, 0, sizeof(*hugepage));
}

/**
 * Allocate a hugepage of the given 'size'
 *
 * @param size Must be a multiple of 2M
 * @param hugepage Pointer to a pre-allocated hugepage-descriptor
 *
 * @return On success, 0 is returned. On error, negative errno is returned to
 * indicate the error.
 */
static inline int
hostmem_hugepage_alloc(size_t size, struct hostmem_hugepage *hugepage,
		       struct hostmem_config *config)
{
	int err;

	if (size % (config->hugepgsz) != 0) {
		UPCIE_DEBUG("FAILED: size must be multiple of hugepgsz(%d)", config->hugepgsz);
		return -EINVAL;
	}

	hugepage->config = config;
	hugepage->size = size;

	switch (hugepage->config->backend) {
	case HOSTMEM_BACKEND_MEMFD:
		hugepage->fd =
			hostmem_internal_memfd_create("hostmem", hugepage->config->memfd_flags);
		if (hugepage->fd < 0) {
			UPCIE_DEBUG("FAILED: memfd_create(); errno(%errno)", errno);
			return -errno;
		}

		snprintf(hugepage->path, sizeof(hugepage->path), "/proc/%d/fd/%d", getpid(),
			 hugepage->fd);
		break;

	case HOSTMEM_BACKEND_HUGETLBFS:
		snprintf(hugepage->path, sizeof(hugepage->path), "%s/%d",
			 hugepage->config->hugetlb_path, hugepage->config->count);

		hugepage->fd = open(hugepage->path, O_CREAT | O_RDWR, 0600);
		if (hugepage->fd < 0) {
			UPCIE_DEBUG("FAILED: open(hugepage); ");
			return -errno;
		}

		break;
	}

	if (ftruncate(hugepage->fd, hugepage->size) != 0) {
		UPCIE_DEBUG("FAILED: ftruncate(hugepage)");
		close(hugepage->fd);
		return -ENOMEM;
	}

	hugepage->virt =
		mmap(NULL, hugepage->size, PROT_READ | PROT_WRITE, MAP_SHARED, hugepage->fd, 0);
	if (hugepage->virt == MAP_FAILED) {
		UPCIE_DEBUG("FAILED: mmap(hugepage)");
		close(hugepage->fd);
		return -ENOMEM;
	}

	err = mlock(hugepage->virt, hugepage->size);
	if (err) {
		UPCIE_DEBUG("FAILED: mlock(hugepage); err(%d)", err);
		munmap(hugepage->virt, hugepage->size);
		close(hugepage->fd);
		return -ENOMEM;
	}

	{
		volatile char *ptr = (volatile char *)hugepage->virt;
		for (size_t i = 0; i < hugepage->size; i += hugepage->config->pagesize) {
			ptr[i] = 0;
		}
	}

	///< The assumption here is that memset and mlock should lead to pinned pages
	memset(hugepage->virt, 0, hugepage->size);

	err = hostmem_pagemap_virt_to_phys(hugepage->virt, &hugepage->phys);
	if (err) {
		UPCIE_DEBUG("FAILED: hostmem_virt_to_phys(hugepage); err(%d)", err);
		munmap(hugepage->virt, hugepage->size);
		close(hugepage->fd);
		return -ENOMEM;
	}

	hugepage->config->count++;

	return 0;
}

/**
 * Import (re-map) an existing hugepage shared by another process.
 *
 * This function uses fstat() to determine the size of the shared memory region.
 *
 * @param path Path to the memfd or hugetlbfs file (e.g. /proc/<pid>/fd/<fd>)
 * @param hugepage Pre-allocated pointer to the descriptor to fill in
 *
 * @return 0 on success, negative errno on error
 */
static inline int
hostmem_hugepage_import(const char *path, struct hostmem_hugepage *hugepage,
			struct hostmem_config *config)
{
	struct stat st;
	int err;

	if (!path || !hugepage) {
		return -EINVAL;
	}

	snprintf(hugepage->path, sizeof(hugepage->path), "%s", path);
	hugepage->config = config;

	hugepage->fd = open(hugepage->path, O_RDWR);
	if (hugepage->fd < 0) {
		UPCIE_DEBUG("FAILED: open(hugepage->path); errno(%d)", errno);
		return -errno;
	}

	if (fstat(hugepage->fd, &st) != 0) {
		UPCIE_DEBUG("FAILED: fstat(hugepage_import_path); errno(%d)", errno);
		close(hugepage->fd);
		return -errno;
	}
	hugepage->size = st.st_size;

	if (hugepage->size % hugepage->config->hugepgsz != 0) {
		UPCIE_DEBUG("FAILED: mapped file size (%zu) is not hugepgsz(%d) aligned",
			    st.st_size, hugepage->config->hugepgsz);
		close(hugepage->fd);
		return -EINVAL;
	}

	hugepage->virt =
		mmap(NULL, hugepage->size, PROT_READ | PROT_WRITE, MAP_SHARED, hugepage->fd, 0);
	if (hugepage->virt == MAP_FAILED) {
		UPCIE_DEBUG("FAILED: mmap(import); errno(%d)", errno);
		close(hugepage->fd);
		return -errno;
	}

	// This is done to ensure that pages are 'paged-in', that is, the process
	// importing the hugepage will be able to resolve virtual pages to physical
	// ones, without this, then this mapping will not be done
	{
		volatile const char *p = (volatile const char *)hugepage->virt;
		volatile char sink;
		for (size_t i = 0; i < hugepage->size; i += hugepage->config->pagesize) {
			sink = p[i];
			(void)sink; ///< Avoid compiler-warnings "unused-but-set-variable"
		}
	}

	err = hostmem_pagemap_virt_to_phys(hugepage->virt, &hugepage->phys);
	if (err) {
		UPCIE_DEBUG("FAILED: hostmem_pagemap_virt_to_phys(); err(%d)", err);
		munmap(hugepage->virt, st.st_size);
		close(hugepage->fd);
		return -ENOMEM;
	}

	return 0;
}
