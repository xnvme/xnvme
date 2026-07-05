// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * dmamem constructor: hugepage-backed memfd
 * =========================================
 *
 * Creates a memfd backed by hugepages (memfd_create(MFD_HUGETLB | ...)),
 * mmaps it for CPU access, and imports the range into an iommufd IOAS
 * with a single IOMMU_IOAS_MAP_FILE call. The result is a dmamem whose
 * cpu_va is the mmap base and whose base_iova is the kernel-picked IOVA.
 *
 * Caveat: the hugepage pool must be reserved out-of-band (see the
 * `hugepages` CLI or `echo N > /proc/sys/vm/nr_hugepages`), and the
 * process must have IPC_LOCK / adequate memlock limits.
 *
 * @file dmamem_memfd.h
 * @version 0.5.2
 */

#ifndef MFD_HUGE_2MB
#define MFD_HUGE_2MB (21 << 26)
#endif
#ifndef MFD_HUGE_1GB
#define MFD_HUGE_1GB (30 << 26)
#endif

/**
 * Allocate a hugepage-backed memfd and map it into the given IOAS.
 *
 * @param dmem     Pre-allocated dmamem descriptor to fill.
 * @param iommufd  Open iommufd handle (caller-owned).
 * @param ioas_id  Target IOAS in the iommufd context.
 * @param size     Size in bytes; must be a multiple of hugepgsz.
 * @param hugepgsz Hugepage size in bytes; must be 2 MiB or 1 GiB.
 *
 * @return 0 on success, negative errno on error.
 */
static inline int
dmamem_from_memfd(struct dmamem *dmem, struct iommufd *iommufd, uint32_t ioas_id, size_t size,
		  size_t hugepgsz)
{
	unsigned int memfd_flags = MFD_HUGETLB;
	int err;

	if (!dmem || !iommufd || iommufd->fd < 0 || !size) {
		return -EINVAL;
	}

	if (size % hugepgsz) {
		UPCIE_DEBUG("FAILED: size(%zu) not a multiple of hugepgsz(%zu)", size, hugepgsz);
		return -EINVAL;
	}

	if (hugepgsz == 2ULL * 1024 * 1024) {
		memfd_flags |= MFD_HUGE_2MB;
	} else if (hugepgsz == 1024ULL * 1024 * 1024) {
		memfd_flags |= MFD_HUGE_1GB;
	} else {
		UPCIE_DEBUG("FAILED: unsupported hugepgsz(%zu)", hugepgsz);
		return -EINVAL;
	}

	memset(dmem, 0, sizeof(*dmem));
	dmem->fd = -1;
	dmem->iommufd = iommufd;
	dmem->ioas_id = ioas_id;
	dmem->size = size;
	dmem->backing = DMAMEM_BACKING_MEMFD;
	dmem->translator = DMAMEM_XLATE_ARITHMETIC;
	dmem->owned = 1;

	dmem->fd = syscall(SYS_memfd_create, "dmamem", memfd_flags);
	if (dmem->fd < 0) {
		err = -errno;
		UPCIE_DEBUG("FAILED: memfd_create(); errno(%d)", errno);
		return err;
	}

	if (ftruncate(dmem->fd, size) != 0) {
		err = -errno;
		UPCIE_DEBUG("FAILED: ftruncate(); errno(%d)", errno);
		goto err_close;
	}

	dmem->cpu_va = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dmem->fd, 0);
	if (dmem->cpu_va == MAP_FAILED) {
		err = -errno;
		dmem->cpu_va = NULL;
		UPCIE_DEBUG("FAILED: mmap(); errno(%d)", errno);
		goto err_close;
	}

	if (mlock(dmem->cpu_va, size) != 0) {
		err = -errno;
		UPCIE_DEBUG("FAILED: mlock(); errno(%d)", errno);
		goto err_munmap;
	}

	/*
	 * Zero the range to fault in and pin every backing hugepage before
	 * the IOAS map. Otherwise IOMMU_IOAS_MAP_FILE may pin a partial or
	 * still-being-materialised range.
	 */
	memset(dmem->cpu_va, 0, size);

	/*
	 * Ask the kernel to pick an IOVA (FIXED_IOVA not set). base_iova is
	 * an OUT parameter to iommufd_ioas_map_file(); the returned value is
	 * the base of the contiguous IOVA window the kernel installed.
	 */
	err = iommufd_ioas_map_file(iommufd, ioas_id, dmem->fd, 0, size,
				    IOMMU_IOAS_MAP_READABLE | IOMMU_IOAS_MAP_WRITEABLE,
				    &dmem->base_iova);
	if (err) {
		UPCIE_DEBUG("FAILED: iommufd_ioas_map_file(); err(%d)", err);
		goto err_munmap;
	}

	return 0;

err_munmap:
	munmap(dmem->cpu_va, size);
	dmem->cpu_va = NULL;
err_close:
	close(dmem->fd);
	dmem->fd = -1;
	return err;
}
